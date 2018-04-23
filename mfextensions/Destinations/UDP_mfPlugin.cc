#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#if MESSAGEFACILITY_HEX_VERSION < 0x20201 // v2_02_01 is s67
# include "messagefacility/MessageService/MessageDrop.h"
#else
# include "messagefacility/MessageLogger/MessageLogger.h"
#endif
#include "messagefacility/Utilities/exception.h"
#include "cetlib/compiler_macros.h"

// C/C++ includes
#include <iostream>
#include <fstream>
#include <memory>
#include <algorithm>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>


// Boost includes
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

#if MESSAGEFACILITY_HEX_VERSION < 0x20201 // format changed to format_ for s67
#define format_ format
#endif

namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
	using mf::ErrorObj;
	using boost::asio::ip::udp;

	/// <summary>
	/// Message Facility UDP Streamer Destination
	/// Formats messages into a delimited string and sends via UDP
	/// </summary>
	class ELUDP : public ELdestination
	{
#if MESSAGEFACILITY_HEX_VERSION >= 0x20103
		struct Config
		{
			fhicl::TableFragment<ELdestination::Config> elDestConfig;
			fhicl::Atom<int> error_max{ fhicl::Name{ "error_turnoff_threshold" },fhicl::Comment{ "Number of errors before turning off destination (default: 0, don't turn off)" },0 };
			fhicl::Atom<int> error_report{ fhicl::Name{ "error_report_backoff_factor" },fhicl::Comment{ "Print an error message every N errors" },100 };
			fhicl::Atom<std::string> host{ fhicl::Name{ "host" },fhicl::Comment{ "Address to send messages to" }, "227.128.12.27" };
			fhicl::Atom<int> port{ fhicl::Name{ "port" },fhicl::Comment{ "Port to send messages to" },5140 };
		};
		using Parameters = fhicl::WrappedTable<Config>;
#endif
	public:
#if MESSAGEFACILITY_HEX_VERSION < 0x20103 // v2_01_03 is s58, pre v2_01_03 is s50
		ELUDP(const fhicl::ParameterSet& pset);
#else
		ELUDP(Parameters const& pset);
#endif

		virtual void fillPrefix(std::ostringstream&, const ErrorObj&) override;

		virtual void fillUsrMsg(std::ostringstream&, const ErrorObj&) override;

		virtual void fillSuffix(std::ostringstream&, const ErrorObj&) override {}

		virtual void routePayload(const std::ostringstream&, const ErrorObj&) override;

	private:
		void reconnect_(bool quiet = false);

		// Parameters
		int error_report_backoff_factor_;
		int error_max_;
		std::string host_;
		int port_;

		// Other stuff
		boost::asio::io_service io_service_;
		udp::socket socket_;
		udp::endpoint remote_endpoint_;
		int consecutive_success_count_;
		int error_count_;
		int next_error_report_;
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

#if MESSAGEFACILITY_HEX_VERSION < 0x20103 // v2_01_03 is s58, pre v2_01_03 is s50
	ELUDP::ELUDP(const fhicl::ParameterSet& pset)
		: ELdestination(pset)
		, error_report_backoff_factor_(pset.get<int>("error_report_backoff_factor", 100))
		, error_max_(pset.get<int>("error_turnoff_threshold", 0))
		, host_(pset.get<std::string>("host", "227.128.12.27"))
		, port_(pset.get<int>("port", 5140))
#else
	ELUDP::ELUDP(Parameters const& pset)
		: ELdestination(pset().elDestConfig())
		, error_report_backoff_factor_(pset().error_report())
		, error_max_(pset().error_max())
		, host_(pset().host())
		, port_(pset().port())
#endif
		, io_service_()
		, socket_(io_service_)
		, remote_endpoint_()
		, consecutive_success_count_(0)
		, error_count_(0)
		, next_error_report_(1)
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
	void ELUDP::fillPrefix(std::ostringstream& oss, const ErrorObj& msg)
	{
		const auto& xid = msg.xid();

		auto id = xid.id();
		auto module = xid.module();
		auto app = app_;
		std::replace(id.begin(), id.end(), '|', '!');
		std::replace(app.begin(), app.end(), '|', '!');
		std::replace(module.begin(), module.end(), '|', '!');

		oss << format_.timestamp(msg.timestamp()) << "|"; // timestamp
		oss << std::to_string(++seqNum_) << "|"; // sequence number
		oss << hostname_ << "|"; // host name
		oss << hostaddr_ << "|"; // host address
		oss << xid.severity().getName() << "|"; // severity
		oss << id << "|"; // category
		oss << app << "|"; // application
#      if MESSAGEFACILITY_HEX_VERSION >= 0x20201 // an indication of s67
		oss << pid_ << "|";
		oss << mf::GetIteration() << "|"; // run/event no
#else
		oss << pid_ << "|"; // process id
		oss << mf::MessageDrop::instance()->iteration << "|"; // run/event no
#endif
		oss << module << "|"; // module name
#if MESSAGEFACILITY_HEX_VERSION >= 0x20201
		oss << msg.filename() << "|" << std::to_string(msg.lineNumber()) << "|";
#endif
	}

	//======================================================================
	// Message filler ( overriddes ELdestination::fillUsrMsg )
	//======================================================================
	void ELUDP::fillUsrMsg(std::ostringstream& oss, const ErrorObj& msg)
	{
		std::ostringstream tmposs;
		// Print the contents.
		for (auto const& val : msg.items())
		{
			tmposs << val;
		}

		// remove leading "\n" if present
		const std::string& usrMsg = !tmposs.str().compare(0, 1, "\n") ? tmposs.str().erase(0, 1) : tmposs.str();

		oss << usrMsg;
	}

	//======================================================================
	// Message router ( overriddes ELdestination::routePayload )
	//======================================================================
	void ELUDP::routePayload(const std::ostringstream& oss, const ErrorObj&)
	{
		if (error_count_ < error_max_ || error_max_ == 0)
		{
			auto pid = pid_;
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

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START extern "C" {
#endif

EXTERN_C_FUNC_DECLARE_START
auto makePlugin(const std::string&,
				const fhicl::ParameterSet& pset)
{
	return std::make_unique<mfplugins::ELUDP>(pset);
}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
