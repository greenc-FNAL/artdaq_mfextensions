#include "mfextensions/Receivers/UDP_receiver.hh"
#include "mfextensions/Receivers/ReceiverMacros.hh"
#include "messagefacility/Utilities/ELseverityLevel.h"
#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>
#include <sstream>

mfviewer::UDPReceiver::UDPReceiver(fhicl::ParameterSet pset) : MVReceiver(pset)
, port_(pset.get<int>("port", 5140))
, io_service_()
, socket_(io_service_)
, debug_(pset.get<bool>("debug_mode", false))
{
	//std::cout << "UDPReceiver Constructor" << std::endl;
	boost::system::error_code ec;
	udp::endpoint listen_endpoint(boost::asio::ip::address::from_string("0.0.0.0"), port_);
	socket_.open(listen_endpoint.protocol());
	boost::asio::socket_base::reuse_address option(true);
	socket_.set_option(option, ec);
	if (ec)
	{
		std::cerr << "An error occurred setting reuse_address: " << ec.message() << std::endl;
	}
	socket_.bind(listen_endpoint, ec);

	if (ec)
	{
		std::cerr << "An error occurred in bind(): " << ec.message() << std::endl;
	}

	if (pset.get<bool>("multicast_enable", false))
	{
		std::string multicast_address_string = pset.get<std::string>("multicast_address", "227.128.12.27");
		auto multicast_address = boost::asio::ip::address::from_string(multicast_address_string);
		boost::asio::ip::multicast::join_group group_option(multicast_address);
		socket_.set_option(group_option, ec);
		if (ec)
		{
			std::cerr << "An error occurred joining the multicast group " << multicast_address_string
				<< ": " << ec.message() << std::endl;
		}

		boost::asio::ip::multicast::enable_loopback loopback_option(true);
		socket_.set_option(loopback_option, ec);

		if (ec)
		{
			std::cerr << "An error occurred setting the multicast loopback option: " << ec.message() << std::endl;
		}
	}
	if (debug_) { std::cout << "UDPReceiver Constructor Done" << std::endl; }
}

mfviewer::UDPReceiver::~UDPReceiver()
{
	if (debug_) { std::cout << "Closing UDP Socket" << std::endl; }
	socket_.close();
}

void mfviewer::UDPReceiver::run()
{
	while (!stopRequested_)
	{
		if (socket_.available() <= 0)
		{
			usleep(10000);
		}
		else
		{
			//usleep(500000);
			udp::endpoint remote_endpoint;
			boost::system::error_code ec;
			size_t packetSize = socket_.receive_from(boost::asio::buffer(buffer_), remote_endpoint, 0, ec);
			if (debug_) { std::cout << "Recieved message; validating...(packetSize=" << packetSize << ")" << std::endl; }
			std::string message(buffer_, buffer_ + packetSize);
			if (ec)
			{
				std::cerr << "Recieved error code: " << ec.message() << std::endl;
			}
			else if (packetSize > 0 && validate_packet(message))
			{
				if (debug_) { std::cout << "Valid UDP Message received! Sending to GUI!" << std::endl; }
				emit NewMessage(read_msg(message));
				//std::cout << std::endl << std::endl;
			}
		}
	}
	std::cout << "UDPReceiver shutting down!" << std::endl;
}

qt_mf_msg mfviewer::UDPReceiver::read_msg(std::string input)
{
	std::string hostname, category, application, message, hostaddr, file, line, module, eventID;
	mf::ELseverityLevel sev;
	timeval tv;
	int pid = 0;
	int seqNum = 0;

	if (debug_) { std::cout << "Recieved MF/Syslog message with contents: " << input << std::endl; }

	boost::char_separator<char> sep("|", "", boost::keep_empty_tokens);
	typedef boost::tokenizer<boost::char_separator<char>> tokenizer;
	tokenizer tokens(input, sep);
	tokenizer::iterator it = tokens.begin();

	//There may be syslog garbage in the first token before the timestamp...
	boost::regex timestamp(".*?(\\d{2}-[^-]*-\\d{4}\\s\\d{2}:\\d{2}:\\d{2})");
	boost::smatch res;
	while (it != tokens.end() && !boost::regex_search(*it, res, timestamp))
	{
		++it;
	}

	if (it != tokens.end())
	{
		struct tm tm;
		time_t t;
		std::string value(res[1].first, res[1].second);
		strptime(value.c_str(), "%d-%b-%Y %H:%M:%S", &tm);
		tm.tm_isdst = -1;
		t = mktime(&tm);
		tv.tv_sec = t;
		tv.tv_usec = 0;

		auto prevIt = it;
		try
		{
			if (++it != tokens.end()) { seqNum = std::stoi(*it); }
		}
		catch (std::invalid_argument e) { it = prevIt; }
		if (++it != tokens.end()) { hostname = *it; }
		if (++it != tokens.end()) { hostaddr = *it; }
		if (++it != tokens.end()) { sev = mf::ELseverityLevel(*it); }
		if (++it != tokens.end()) { category = *it; }
		if (++it != tokens.end()) { application = *it; }
		prevIt = it;
		try
		{
			if (++it != tokens.end()) { pid = std::stol(*it); }
		}
		catch (std::invalid_argument e) { it = prevIt; }
		if (++it != tokens.end()) { eventID = *it; }
		if (++it != tokens.end()) { module = *it; }
#if MESSAGEFACILITY_HEX_VERSION >= 0x20201 // Sender and receiver version must match!
		if (++it != tokens.end()) { file = *it; }
		if (++it != tokens.end()) { line = *it; }
#endif
		std::ostringstream oss;
		bool first = true;
		while (++it != tokens.end())
		{
			if (!first) { oss << "|"; }
			else { first = false; }
			oss << *it;
		}
		if (debug_) { std::cout << "Message content: " << oss.str() << std::endl; }
		message = oss.str();
#if MESSAGEFACILITY_HEX_VERSION < 0x20201 // Sender and receiver version must match!
		boost::regex fileLine("^\\s*([^:]*):(\\d+)(.*)");
		if (boost::regex_search(message, res, fileLine))
		{
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

bool mfviewer::UDPReceiver::validate_packet(std::string input)
{
	// Run some checks on the input packet
	if (input.find("MF") == std::string::npos)
	{
		std::cout << "Failed to find \"MF\" in message: " << input << std::endl;
		return false;
	}
	if (input.find("|") == std::string::npos)
	{
		std::cout << "Failed to find | separator character in message: " << input << std::endl;
		return false;
	}
	return true;
}

#include "moc_UDP_receiver.cpp"

DEFINE_MFVIEWER_RECEIVER(mfviewer::UDPReceiver)
