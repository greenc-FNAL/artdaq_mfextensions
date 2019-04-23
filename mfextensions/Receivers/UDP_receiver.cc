#define TRACE_NAME "UDP_Receiver"

#include "mfextensions/Receivers/UDP_receiver.hh"
#include <sys/poll.h>
#include <boost/tokenizer.hpp>
#include <sstream>
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "mfextensions/Receivers/ReceiverMacros.hh"
#include "mfextensions/Receivers/detail/TCPConnect.hh"

mfviewer::UDPReceiver::UDPReceiver(fhicl::ParameterSet pset)
    : MVReceiver(pset),
      message_port_(pset.get<int>("port", 5140)),
      message_addr_(pset.get<std::string>("message_address", "227.128.12.27")),
      multicast_enable_(pset.get<bool>("multicast_enable", false)),
      multicast_out_addr_(pset.get<std::string>("multicast_interface_ip", "0.0.0.0")),
      message_socket_(-1),
      timestamp_regex_("(\\d{2}-[^-]*-\\d{4}\\s\\d{2}:\\d{2}:\\d{2})"),
      file_line_regex_("^\\s*([^:]*\\.[^:]{1,3}):(\\d+)(.*)") {
  TLOG(TLVL_TRACE) << "UDPReceiver Constructor";
}

void mfviewer::UDPReceiver::setupMessageListener_() {
  TLOG(TLVL_INFO) << "Setting up message listen socket, address=" << message_addr_ << ":" << message_port_;
  message_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (message_socket_ < 0) {
    TLOG(TLVL_ERROR) << "Error creating socket for receiving messages! err=" << strerror(errno);
    exit(1);
  }

  struct sockaddr_in si_me_request;

  int yes = 1;
  if (setsockopt(message_socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
    TLOG(TLVL_ERROR) << "Unable to enable port reuse on message socket, err=" << strerror(errno);
    exit(1);
  }
  memset(&si_me_request, 0, sizeof(si_me_request));
  si_me_request.sin_family = AF_INET;
  si_me_request.sin_port = htons(message_port_);
  si_me_request.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(message_socket_, (struct sockaddr *)&si_me_request, sizeof(si_me_request)) == -1) {
    TLOG(TLVL_ERROR) << "Cannot bind message socket to port " << message_port_ << ", err=" << strerror(errno);
    exit(1);
  }

  if (message_addr_ != "localhost" && multicast_enable_) {
    struct ip_mreq mreq;
    int sts = ResolveHost(message_addr_.c_str(), mreq.imr_multiaddr);
    if (sts == -1) {
      TLOG(TLVL_ERROR) << "Unable to resolve multicast message address, err=" << strerror(errno);
      exit(1);
    }
    sts = GetInterfaceForNetwork(multicast_out_addr_.c_str(), mreq.imr_interface);
    if (sts == -1) {
      TLOG(TLVL_ERROR) << "Unable to resolve hostname for " << multicast_out_addr_;
      exit(1);
    }
    if (setsockopt(message_socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
      TLOG(TLVL_ERROR) << "Unable to join multicast group, err=" << strerror(errno);
      exit(1);
    }
  }
  TLOG(TLVL_INFO) << "Done setting up message receive socket";
}

mfviewer::UDPReceiver::~UDPReceiver() {
  TLOG(TLVL_DEBUG) << "Closing message receive socket";
  close(message_socket_);
  message_socket_ = -1;
}

void mfviewer::UDPReceiver::run() {
  while (!stopRequested_) {
    if (message_socket_ == -1) setupMessageListener_();

    int ms_to_wait = 10;
    struct pollfd ufds[1];
    ufds[0].fd = message_socket_;
    ufds[0].events = POLLIN | POLLPRI | POLLERR;
    int rv = poll(ufds, 1, ms_to_wait);

    // Continue loop if no message received or message does not have correct event ID
    if (rv <= 0 || (ufds[0].revents != POLLIN && ufds[0].revents != POLLPRI)) {
      if (rv == 1 && (ufds[0].revents & (POLLNVAL | POLLERR | POLLHUP))) {
        close(message_socket_);
        message_socket_ = -1;
      }
      if (stopRequested_) {
        break;
      }
      continue;
    }

    char buffer[TRACE_STREAMER_MSGMAX + 1];
    auto packetSize = read(message_socket_, &buffer, TRACE_STREAMER_MSGMAX);
    if (packetSize < 0) {
      TLOG(TLVL_ERROR) << "Error receiving message, errno=" << errno << " (" << strerror(errno) << ")";
    } else {
      TLOG(TLVL_TRACE) << "Recieved message; validating...(packetSize=" << packetSize << ")";
      std::string message(buffer, buffer + packetSize);
      if (validate_packet(message)) {
        TLOG(TLVL_TRACE) << "Valid UDP Message received! Sending to GUI!";
        emit NewMessage(read_msg(message));
      }
    }
  }
  TLOG(TLVL_INFO) << "UDPReceiver shutting down!";
}

qt_mf_msg mfviewer::UDPReceiver::read_msg(std::string input) {
  std::string hostname, category, application, message, hostaddr, file, line, module, eventID;
  mf::ELseverityLevel sev;
  timeval tv = {0, 0};
  int pid = 0;
  int seqNum = 0;

  TLOG(TLVL_TRACE) << "Recieved MF/Syslog message with contents: " << input;

  boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
  typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
  tokenizer tokens(input, sep);
  tokenizer::iterator it = tokens.begin();

  // There may be syslog garbage in the first token before the timestamp...
  boost::smatch res;
  while (it != tokens.end() && !boost::regex_search(*it, res, timestamp_regex_)) {
    ++it;
  }

  if (it != tokens.end()) {
    struct tm tm;
    time_t t;
    std::string value(res[1].first, res[1].second);
    strptime(value.c_str(), "%d-%b-%Y %H:%M:%S", &tm);
    tm.tm_isdst = -1;
    t = mktime(&tm);
    tv.tv_sec = t;
    tv.tv_usec = 0;

    auto prevIt = it;
    try {
      if (++it != tokens.end()) {
        seqNum = std::stoi(*it);
      }
    } catch (const std::invalid_argument& e) {
      it = prevIt;
    }
    if (++it != tokens.end()) {
      hostname = *it;
    }
    if (++it != tokens.end()) {
      hostaddr = *it;
    }
    if (++it != tokens.end()) {
      sev = mf::ELseverityLevel(*it);
    }
    if (++it != tokens.end()) {
      category = *it;
    }
    if (++it != tokens.end()) {
      application = *it;
    }
    prevIt = it;
    try {
      if (++it != tokens.end()) {
        pid = std::stol(*it);
      }
    } catch (const std::invalid_argument& e) {
      it = prevIt;
    }
    if (++it != tokens.end()) {
      eventID = *it;
    }
    if (++it != tokens.end()) {
      module = *it;
    }
#if MESSAGEFACILITY_HEX_VERSION >= 0x20201  // Sender and receiver version must match!
    if (++it != tokens.end()) {
      file = *it;
    }
    if (++it != tokens.end()) {
      line = *it;
    }
#endif
    std::ostringstream oss;
    bool first = true;
    while (++it != tokens.end()) {
      if (!first) {
        oss << "|";
      } else {
        first = false;
      }
      oss << *it;
    }
    TLOG(TLVL_TRACE) << "Message content: " << oss.str();
    message = oss.str();
#if MESSAGEFACILITY_HEX_VERSION < 0x20201  // Sender and receiver version must match!
    if (boost::regex_search(message, res, file_line_regex_)) {
      file = std::string(res[1].first, res[1].second);
      line = std::string(res[2].first, res[2].second);
      std::string messagetmp = std::string(res[3].first, res[3].second);
      message = messagetmp;
    }
#endif
  }

  qt_mf_msg msg(hostname, category, application, pid, tv);
  msg.setSeverity(sev);
  msg.setMessage("UDPMessage", seqNum, message);
  msg.setHostAddr(hostaddr);
  msg.setFileName(file);
  msg.setLineNumber(line);
  msg.setModule(module);
  msg.setEventID(eventID);
  msg.updateText();

  return msg;
}

bool mfviewer::UDPReceiver::validate_packet(std::string input) {
  // Run some checks on the input packet
  if (input.find("MF") == std::string::npos) {
    TLOG(TLVL_WARNING) << "Failed to find \"MF\" in message: " << input;
    return false;
  }
  if (input.find("|") == std::string::npos) {
    TLOG(TLVL_WARNING) << "Failed to find | separator character in message: " << input;
    return false;
  }
  return true;
}

#include "moc_UDP_receiver.cpp"

DEFINE_MFVIEWER_RECEIVER(mfviewer::UDPReceiver)
