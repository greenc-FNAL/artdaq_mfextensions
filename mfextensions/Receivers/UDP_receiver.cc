#include "mfextensions/Receivers/UDP_receiver.hh"
#include "mfextensions/Receivers/ReceiverMacros.hh"
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>

mfviewer::UDPReceiver::UDPReceiver(fhicl::ParameterSet pset) : MVReceiver(pset)
							     , port_(pset.get<int>("port",5140))
							     , io_service_()
							     , socket_(io_service_)
                                                             , count_(0)
{
  std::cout << "UDPReceiver Constructor" << std::endl;
  boost::system::error_code ec;
  socket_.open(udp::v4());
  boost::asio::socket_base::reuse_address option(true);
  socket_.set_option(option, ec);
  if(ec) {
    std::cout << "An error occurred in set_option(): " << ec.message() << std::endl;
  }
  socket_.bind(udp::endpoint(udp::v4(), port_),ec);

  if(ec) {
    std::cout << "An error occurred in bind(): " << ec.message() << std::endl;
  }
  std::cout << "UDPReceiver Constructor Done" << std::endl;
}

mfviewer::UDPReceiver::~UDPReceiver()
{
  socket_.close();
}

void mfviewer::UDPReceiver::run()
{
  while(!stopRequested_)
    {
      while(socket_.available() > 0) 
      {
        usleep(500000);
        udp::endpoint remote_endpoint;
	boost::system::error_code ec;
        size_t packetSize = socket_.receive_from(boost::asio::buffer(buffer_), remote_endpoint, 0, ec);
	std::cout << "Recieved message; validating...(packetSize=" << packetSize << ")" << std::endl;
	std::string message(buffer_, buffer_ + packetSize);
        if(ec) {
	  std::cout << "Recieved error code: " << ec.message() << std::endl;
        }
        else if(packetSize > 0 && validate_packet(message))
	{
	  std::cout << "Valid UDP Message received! Sending to GUI!" << std::endl;
	    emit NewMessage(read_msg(message));
	}
      }
    }
  std::cout << "UDPReceiver shutting down!" << std::endl;
}

mf::MessageFacilityMsg mfviewer::UDPReceiver::read_msg(std::string input)
{
  mf::MessageFacilityMsg msg;
  std::cout << "Recieved MF/Syslog message with contents: " << input << std::endl;

  boost::char_separator<char> sep("|");
  typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
  tokenizer tokens(input, sep);
  tokenizer::iterator it = tokens.begin();

  //There may be syslog garbage in the first token before the timestamp...
  boost::regex timestamp(".*?(\\d{2}-[^-]*-\\d{4}\\s\\d{2}:\\d{2}:\\d{2})");
  boost::smatch res;
  while(it != tokens.end() && !boost::regex_search( *it, res, timestamp) )
    { ++it; }

  if(it != tokens.end()) {
    struct tm tm;
    time_t t;
    timeval tv;
    std::string value(res[1].first, res[1].second);
    strptime(value.c_str(), "%d-%b-%Y %H:%M:%S", &tm);
    tm.tm_isdst = -1;
    t = mktime(&tm);
    tv.tv_sec = t;
    tv.tv_usec = 0;
    msg.setTimestamp( tv );
  
    if(++it != tokens.end()) { msg.setHostname(*it); }
    if(++it != tokens.end()) { msg.setHostaddr(*it); }
    if(++it != tokens.end()) { msg.setSeverity(*it); }
    if(++it != tokens.end()) { msg.setCategory(*it); }
    if(++it != tokens.end()) { msg.setApplication(*it); }
    if(++it != tokens.end()) { msg.setProcess(*it); }
    if(++it != tokens.end()) { msg.setContext(*it); }
    if(++it != tokens.end()) { msg.setModule(*it); }
    if(++it != tokens.end()) { msg.setMessage(std::string("UDPMessage"), std::to_string(count_), *it); }
    ++count_;
  }

  return msg;
}

bool mfviewer::UDPReceiver::validate_packet(std::string input)
{
  // Run some checks on the input packet
  if(input.find("MF") == std::string::npos){ 
    std::cout << "Failed to find \"MF\" in message: " << input << std::endl;
   return false; 
  }
  if(input.find("|") == std::string::npos){ 
    std::cout << "Failed to find | separator character in message: "<< input << std::endl;
     return false; 
  }
  return true;
}

#include "moc_UDP_receiver.cpp"

DEFINE_MFVIEWER_RECEIVER(mfviewer::UDPReceiver)
