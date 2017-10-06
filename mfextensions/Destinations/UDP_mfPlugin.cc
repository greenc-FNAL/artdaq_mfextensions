#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
# include "messagefacility/MessageService/ELcontextSupplier.h"
# include "messagefacility/MessageLogger/MessageDrop.h"
#else
# include "messagefacility/MessageService/MessageDrop.h"
#endif
#include "messagefacility/Utilities/exception.h"

// C/C++ includes
#include <iostream>
#include <fstream>
#include <memory>
#include <algorithm>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <thread>


// Boost includes
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
	using mf::ErrorObj;
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	using mf::service::ELcontextSupplier;
# endif
	using boost::asio::ip::udp;

	//======================================================================
	//
	// UDP destination plugin
	//
	//======================================================================

	class ELUDP : public ELdestination
	{
	public:

		ELUDP(const fhicl::ParameterSet& pset);

		virtual void fillPrefix(std::ostringstream&, const ErrorObj&
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
								, const ELcontextSupplier&
# endif
		) override;

		virtual void fillUsrMsg(std::ostringstream&, const ErrorObj&) override;

		virtual void fillSuffix(std::ostringstream&, const ErrorObj&) override {}

		virtual void routePayload(const std::ostringstream&, const ErrorObj&
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
								  , const ELcontextSupplier&
# endif
		) override;

	private:
		void reconnect_(bool quiet = false);

		boost::asio::io_service io_service_;
		udp::socket socket_;
		udp::endpoint remote_endpoint_;
		int consecutive_success_count_;
		int error_count_;
		int next_error_report_;
		int error_report_backoff_factor_;
		int error_max_;
		std::string host_;
		int port_;
		int seqNum_;


		long pid_;
		std::string hostname_;
		std::string hostaddr_;
		std::string app_;
	};

	// END DECLARATION
	//======================================================================
	// BEGIN IMPLEMENTATION


	//======================================================================
	// ELUDP c'tor
	//======================================================================

	ELUDP::ELUDP(const fhicl::ParameterSet& pset)
		: ELdestination(pset)
		, io_service_()
		, socket_(io_service_)
		, remote_endpoint_()
		, consecutive_success_count_(0)
		, error_count_(0)
		, next_error_report_(1)
		, error_report_backoff_factor_(pset.get<int>("error_report_backoff_factor", 10))
		, error_max_(pset.get<int>("error_turnoff_threshold", 1))
		, host_(pset.get<std::string>("host", "227.128.12.27"))
		, port_(pset.get<int>("port", 5140))
		, seqNum_(0)
		, pid_(static_cast<long>(getpid()))
	{
		reconnect_();

		// hostname
		char hostname_c[1024];
		hostname_ = (gethostname(hostname_c, 1023) == 0) ? hostname_c : "Unkonwn Host";
		
		// host ip address
		hostent* host = nullptr;
		host = gethostbyname(hostname_c);

		if (host != nullptr)
		{
			// ip address from hostname if the entry exists in /etc/hosts
			char* ip = inet_ntoa(*(struct in_addr *)host->h_addr);
			hostaddr_ = ip;
		}
		else
		{
			// enumerate all network interfaces
			struct ifaddrs* ifAddrStruct = nullptr;
			struct ifaddrs* ifa = nullptr;
			void* tmpAddrPtr = nullptr;

			if (getifaddrs(&ifAddrStruct))
			{
				// failed to get addr struct
				hostaddr_ = "127.0.0.1";
			}
			else
			{
				// iterate through all interfaces
				for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next)
				{
					if (ifa->ifa_addr->sa_family == AF_INET)
					{
						// a valid IPv4 addres
						tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
						char addressBuffer[INET_ADDRSTRLEN];
						inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
						hostaddr_ = addressBuffer;
					}

					else if (ifa->ifa_addr->sa_family == AF_INET6)
					{
						// a valid IPv6 address
						tmpAddrPtr = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
						char addressBuffer[INET6_ADDRSTRLEN];
						inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
						hostaddr_ = addressBuffer;
					}

					// find first non-local address
					if (!hostaddr_.empty()
						&& hostaddr_.compare("127.0.0.1")
						&& hostaddr_.compare("::1"))
						break;
				}

				if (hostaddr_.empty()) // failed to find anything
					hostaddr_ = "127.0.0.1";
			}
		}

#if 0
		// get process name from '/proc/pid/exe'
		std::string exe;
		std::ostringstream pid_ostr;
		pid_ostr << "/proc/" << pid_ << "/exe";
		exe = realpath(pid_ostr.str().c_str(), NULL);

		size_t end = exe.find('\0');
		size_t start = exe.find_last_of('/', end);

		app_ = exe.substr(start + 1, end - start - 1);
#else
		// get process name from '/proc/pid/cmdline'
		std::stringstream ss;
		ss << "//proc//" << pid_ << "//cmdline";
		std::ifstream procfile{ ss.str().c_str() };

		std::string procinfo;

		if (procfile.is_open())
		{
			procfile >> procinfo;
			procfile.close();
		}

		size_t end = procinfo.find('\0');
		size_t start = procinfo.find_last_of('/', end);

		app_ = procinfo.substr(start + 1, end - start - 1);
#endif
	}

	void ELUDP::reconnect_(bool quiet)
	{
		boost::system::error_code ec;
		socket_.open(udp::v4());
		socket_.set_option(boost::asio::socket_base::reuse_address(true), ec);
		if (ec && !quiet)
		{
			std::cerr << "An error occurred setting reuse_address to true: "
				<< ec.message() << std::endl;
		}

		if (boost::iequals(host_, "Broadcast") || host_ == "255.255.255.255")
		{
			socket_.set_option(boost::asio::socket_base::broadcast(true), ec);
			remote_endpoint_ = udp::endpoint(boost::asio::ip::address_v4::broadcast(), port_);
		}
		else
		{
			udp::resolver resolver(io_service_);
			udp::resolver::query query(udp::v4(), host_, std::to_string(port_));
			remote_endpoint_ = *resolver.resolve(query);
		}

		socket_.connect(remote_endpoint_, ec);
		if (ec)
		{
			if (!quiet)
			{
				std::cerr << "An Error occurred in connect(): " << ec.message() << std::endl
					<< "  endpoint = " << remote_endpoint_ << std::endl;
			}
			error_count_++;
		}
		//else {
		//  std::cout << "Successfully connected to remote endpoint = " << remote_endpoint_
		//            << std::endl;
		//}
	}

	//======================================================================
	// Message prefix filler ( overriddes ELdestination::fillPrefix )
	//======================================================================
	void ELUDP::fillPrefix(std::ostringstream& oss, const ErrorObj& msg
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
						   , ELcontextSupplier const&
# endif
	)
	{
		const auto& xid = msg.xid();

#      if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		auto id = xid.id();
		auto module = xid.module();
		auto app = app_;
#      else
		auto id = xid.id;
		auto app = xid.application;
		auto process = xid.process;
		auto module = xid.module;
#      endif
		std::replace(id.begin(), id.end(), '|', '!');
		std::replace(app.begin(), app.end(), '|', '!');
#      if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		std::replace(process.begin(), process.end(), '|', '!');
#      endif
		std::replace(module.begin(), module.end(), '|', '!');

		oss << format.timestamp(msg.timestamp()) << "|"; // timestamp
		oss << std::to_string(++seqNum_) << "|"; // sequence number
#      if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		oss << hostname_ << "|"; // host name
		oss << hostaddr_ << "|"; // host address
		oss << xid.severity().getName() << "|"; // severity
#      else
		oss << xid.hostname << "|"; // host name
		oss << xid.hostaddr << "|"; // host address
		oss << xid.severity.getName() << "|"; // severity
#      endif
		oss << id << "|"; // category
		oss << app << "|"; // application
#      if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		oss << pid_ << "|"; // process id
		oss << mf::MessageDrop::instance()->iteration << "|"; // run/event no
#      else
		oss << process << "|";
		oss << xid.pid << "|"; // process id
		oss << mf::MessageDrop::instance()->runEvent << "|"; // run/event no
#      endif
		oss << module << "|"; // module name
	}

	//======================================================================
	// Message filler ( overriddes ELdestination::fillUsrMsg )
	//======================================================================
	void ELUDP::fillUsrMsg(std::ostringstream& oss, const ErrorObj& msg)
	{
		std::ostringstream tmposs;
		ELdestination::fillUsrMsg(tmposs, msg);

		// remove leading "\n" if present
		const std::string& usrMsg = !tmposs.str().compare(0, 1, "\n") ? tmposs.str().erase(0, 1) : tmposs.str();

		oss << usrMsg;
	}

	//======================================================================
	// Message router ( overriddes ELdestination::routePayload )
	//======================================================================
	void ELUDP::routePayload(const std::ostringstream& oss
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
							 , const ErrorObj& msg, ELcontextSupplier const&
#else
							 , const ErrorObj&
#endif
	)
	{
		if (error_count_ < error_max_)
		{
#          if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
			auto pid = pid_;
#          else
			auto pid = msg.xid().pid;
#          endif
			//std::cout << oss.str() << std::endl;
			auto string = "UDPMFMESSAGE" + std::to_string(pid) + "|" + oss.str();
			//std::cout << string << std::endl;
			auto message = boost::asio::buffer(string);
			bool error = true;
			try
			{
				socket_.send_to(message, remote_endpoint_);
				++consecutive_success_count_;
				if (consecutive_success_count_ >= 5)
				{
					error_count_ = 0;
					next_error_report_ = 1;
				}
				error = false;
			}
			catch (boost::system::system_error& err)
			{
				consecutive_success_count_ = 0;
				++error_count_;
				if (error_count_ == next_error_report_)
				{
					std::cerr << "An exception occurred when trying to send message " << std::to_string(seqNum_) << " to "
						<< remote_endpoint_ << std::endl
						<< "  message = " << oss.str() << std::endl
						<< "  exception = " << err.what() << std::endl;
					next_error_report_ *= error_report_backoff_factor_;
				}
			}
			if (error)
			{
				try
				{
					reconnect_(true);
				}
				catch (...) {}
			}
		}
	}
} // end namespace mfplugins

//======================================================================
//
// makePlugin function
//
//======================================================================

extern "C"
{
	auto makePlugin(const std::string&,
					const fhicl::ParameterSet& pset)
	{
		return std::make_unique<mfplugins::ELUDP>(pset);
	}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
