#define TRACE_NAME "UDP_Receiver"

#include "mfextensions/Receivers/UDP_receiver.hh"
#include <sys/poll.h>
#include <sstream>
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "mfextensions/Receivers/ReceiverMacros.hh"
#include "mfextensions/Receivers/detail/TCPConnect.hh"

mfviewer::UDPReceiver::UDPReceiver(fhicl::ParameterSet const& pset)
    : MVReceiver(pset)
    , message_port_(pset.get<int>("port", 5140))
    , message_addr_(pset.get<std::string>("message_address", "227.128.12.27"))
    , multicast_enable_(pset.get<bool>("multicast_enable", false))
    , multicast_out_addr_(pset.get<std::string>("multicast_interface_ip", "0.0.0.0"))
    , message_socket_(-1)
{
	TLOG(TLVL_TRACE) << "UDPReceiver Constructor";
	this->setObjectName("UDPReceiver");
}

void mfviewer::UDPReceiver::setupMessageListener_()
{
	TLOG(TLVL_INFO) << "Setting up message listen socket, address=" << message_addr_ << ":" << message_port_;
	message_socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (message_socket_ < 0)
	{
		TLOG(TLVL_ERROR) << "Error creating socket for receiving messages! err=" << strerror(errno);
		exit(1);
	}

	struct sockaddr_in si_me_request;

	int yes = 1;
	if (setsockopt(message_socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
	{
		TLOG(TLVL_ERROR) << "Unable to enable port reuse on message socket, err=" << strerror(errno);
		exit(1);
	}
	memset(&si_me_request, 0, sizeof(si_me_request));
	si_me_request.sin_family = AF_INET;
	si_me_request.sin_port = htons(message_port_);
	si_me_request.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(message_socket_, reinterpret_cast<struct sockaddr*>(&si_me_request), sizeof(si_me_request)) == -1)  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
	{
		TLOG(TLVL_ERROR) << "Cannot bind message socket to port " << message_port_ << ", err=" << strerror(errno);
		exit(1);
	}

	if (message_addr_ != "localhost" && multicast_enable_)
	{
		struct ip_mreq mreq;
		int sts = ResolveHost(message_addr_.c_str(), mreq.imr_multiaddr);
		if (sts == -1)
		{
			TLOG(TLVL_ERROR) << "Unable to resolve multicast message address, err=" << strerror(errno);
			exit(1);
		}
		sts = GetInterfaceForNetwork(multicast_out_addr_.c_str(), mreq.imr_interface);
		if (sts == -1)
		{
			TLOG(TLVL_ERROR) << "Unable to resolve hostname for " << multicast_out_addr_;
			exit(1);
		}
		if (setsockopt(message_socket_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
		{
			TLOG(TLVL_ERROR) << "Unable to join multicast group, err=" << strerror(errno);
			exit(1);
		}
	}
	TLOG(TLVL_INFO) << "Done setting up message receive socket";
}

mfviewer::UDPReceiver::~UDPReceiver()
{
	TLOG(TLVL_DEBUG) << "Closing message receive socket";
	close(message_socket_);
	message_socket_ = -1;
}

void mfviewer::UDPReceiver::run()
{
	while (!stopRequested_)
	{
		if (message_socket_ == -1) setupMessageListener_();

		int ms_to_wait = 10;
		struct pollfd ufds[1];
		ufds[0].fd = message_socket_;
		ufds[0].events = POLLIN | POLLPRI | POLLERR;
		int rv = poll(ufds, 1, ms_to_wait);

		// Continue loop if no message received or message does not have correct event ID
		if (rv <= 0 || (ufds[0].revents != POLLIN && ufds[0].revents != POLLPRI))
		{
			if (rv == 1 && (ufds[0].revents & (POLLNVAL | POLLERR | POLLHUP)))
			{
				close(message_socket_);
				message_socket_ = -1;
			}
			if (stopRequested_)
			{
				break;
			}
			continue;
		}

		char buffer[TRACE_STREAMER_MSGMAX + 1];
		auto packetSize = read(message_socket_, &buffer, TRACE_STREAMER_MSGMAX);
		if (packetSize < 0)
		{
			TLOG(TLVL_ERROR) << "Error receiving message, errno=" << errno << " (" << strerror(errno) << ")";
		}
		else
		{
			TLOG(TLVL_TRACE) << "Recieved message; validating...(packetSize=" << packetSize << ")";
			std::string message(buffer, buffer + packetSize);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
			if (validate_packet(message))
			{
				TLOG(TLVL_TRACE) << "Valid UDP Message received! Sending to GUI!";
				emit NewMessage(read_msg(message));
			}
		}
	}
	TLOG(TLVL_INFO) << "UDPReceiver shutting down!";
}

std::list<std::string> mfviewer::UDPReceiver::tokenize_(std::string const& input)
{
	size_t pos = 0;
	std::list<std::string> output;

	while (pos != std::string::npos && pos < input.size())
	{
		auto newpos = input.find('|', pos);
		if (newpos != std::string::npos)
		{
			output.emplace_back(input, pos, newpos - pos);
			//TLOG(TLVL_TRACE) << "tokenize_: " << output.back();
			pos = newpos + 1;
		}
		else
		{
			output.emplace_back(input, pos);
			//TLOG(TLVL_TRACE) << "tokenize_: " << output.back();
			pos = newpos;
		}
	}
	return output;
}

msg_ptr_t mfviewer::UDPReceiver::read_msg(std::string const& input)
{
	std::string hostname, category, application, message, hostaddr, file, line, module, eventID;
	mf::ELseverityLevel sev;
	timeval tv = {0, 0};
	int pid = 0;
	int seqNum = 0;

	TLOG(TLVL_TRACE) << "Recieved MF/Syslog message with contents: " << input;

	auto tokens = tokenize_(input);
	auto it = tokens.begin();

	if (it != tokens.end())
	{
		bool timestamp_found = false;
		struct tm tm;
		time_t t;
		while (it != tokens.end() && !timestamp_found)
		{
			std::string thisString = *it;
			while (!thisString.empty() && !timestamp_found)
			{
				auto pos = thisString.find_first_of("0123456789");
				if (pos != std::string::npos)
				{
					thisString = thisString.erase(0, pos);
					//TLOG(TLVL_TRACE) << "thisString: " << thisString;

					if (strptime(thisString.c_str(), "%d-%b-%Y %H:%M:%S", &tm) != nullptr)
					{
						timestamp_found = true;
						break;
					}

					if (!thisString.empty())
						thisString = thisString.erase(0, 1);
				}
			}
			++it;
		}

		tm.tm_isdst = -1;
		t = mktime(&tm);
		tv.tv_sec = t;
		tv.tv_usec = 0;

		auto prevIt = it;
		try
		{
			if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
			{
				seqNum = std::stoi(*it);
			}
		}
		catch (const std::invalid_argument& e)
		{
			it = prevIt;
		}
		if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
		{
			hostname = *it;
		}
		if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
		{
			hostaddr = *it;
		}
		if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
		{
			sev = mf::ELseverityLevel(*it);
		}
		if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
		{
			category = *it;
		}
		if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
		{
			application = *it;
		}
		prevIt = it;
		try
		{
			if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
			{
				pid = std::stol(*it);
			}
		}
		catch (const std::invalid_argument& e)
		{
			it = prevIt;
		}
		if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
		{
			eventID = *it;
		}
		if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
		{
			module = *it;
		}
		if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
		{
			file = *it;
		}
		if (it != tokens.end() && ++it != tokens.end() /* Advances it */)
		{
			line = *it;
		}
		std::ostringstream oss;
		bool first = true;
		while (it != tokens.end() && ++it != tokens.end() /* Advances it */)
		{
			if (!first)
			{
				oss << "|";
			}
			else
			{
				first = false;
			}
			oss << *it;
		}
		TLOG(TLVL_TRACE) << "Message content: " << oss.str();
		message = oss.str();
	}

	auto msg = std::make_shared<qt_mf_msg>(hostname, category, application, pid, tv);
	msg->setSeverity(sev);
	msg->setMessage("UDPMessage", seqNum, message);
	msg->setHostAddr(hostaddr);
	msg->setFileName(file);
	msg->setLineNumber(line);
	msg->setModule(module);
	msg->setEventID(eventID);
	msg->updateText();

	return msg;
}

bool mfviewer::UDPReceiver::validate_packet(std::string const& input)
{
	// Run some checks on the input packet
	if (input.find("MF") == std::string::npos)
	{
		TLOG(TLVL_WARNING) << "Failed to find \"MF\" in message: " << input;
		return false;
	}
	if (input.find("|") == std::string::npos)
	{
		TLOG(TLVL_WARNING) << "Failed to find | separator character in message: " << input;
		return false;
	}
	return true;
}

#include "moc_UDP_receiver.cpp"

DEFINE_MFVIEWER_RECEIVER(mfviewer::UDPReceiver)
