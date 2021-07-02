#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/ConfigurationTable.h"

#include "cetlib/compiler_macros.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/Utilities/exception.h"

// C/C++ includes
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include "mfextensions/Receivers/detail/TCPConnect.hh"

#define TRACE_NAME "OTS_mfPlugin"
#include "trace.h"

// Boost includes
#include <boost/algorithm/string.hpp>

namespace mfplugins {
using mf::ErrorObj;
using mf::service::ELdestination;

/// <summary>
/// Message Facility OTS Console Destination
/// Formats messages into Ryan's favorite format for OTS
/// </summary>
class ELOTS : public ELdestination
{
public:
	/**
   * \brief Configuration Parameters for ELOTS
   */
	struct Config
	{
		/// ELDestination common config parameters
		fhicl::TableFragment<ELdestination::Config> elDestConfig;
		/// format_string (Default: "%L:%N:%f [%u]	%m"): Format specifier for printing to console. %% => '%' ...
		fhicl::Atom<std::string> format_string = fhicl::Atom<std::string>{
		    fhicl::Name{"format_string"}, fhicl::Comment{"Format specifier for printing to console. %% => '%' ... "},
		    "%L:%N:%f [%u]	%m"};
		/// filename_delimit (Default: "/"): Grab path after this. "/srcs/" /x/srcs/y/z.cc => y/z.cc
		fhicl::Atom<std::string> filename_delimit =
		    fhicl::Atom<std::string>{fhicl::Name{"filename_delimit"},
		                             fhicl::Comment{"Grab path after this. \"/srcs/\" /x/srcs/y/z.cc => y/z.cc"}, "/"};
	};
	/// Used for ParameterSet validation
	using Parameters = fhicl::WrappedTable<Config>;

public:
	/// <summary>
	/// ELOTS Constructor
	/// </summary>
	/// <param name="pset">ParameterSet used to configure ELOTS</param>
	ELOTS(Parameters const& pset);

	/**
   * \brief Fill the "Prefix" portion of the message
   * \param o Output stringstream
   * \param msg MessageFacility object containing header information
   */
	void fillPrefix(std::ostringstream& o, const ErrorObj& msg) override;

	/**
   * \brief Fill the "User Message" portion of the message
   * \param o Output stringstream
   * \param msg MessageFacility object containing header information
   */
	void fillUsrMsg(std::ostringstream& o, const ErrorObj& msg) override;

	/**
   * \brief Fill the "Suffix" portion of the message (Unused)
   */
	void fillSuffix(std::ostringstream& /*unused*/, const ErrorObj& /*msg*/) override {}

	/**
   * \brief Serialize a MessageFacility message to the output
   * \param o Stringstream object containing message data
   * \param e MessageFacility object containing header information
   */
	void routePayload(const std::ostringstream& o, const ErrorObj& e) override;

private:
	// Other stuff
	int64_t pid_;
	std::string hostname_;
	std::string hostaddr_;
	std::string app_;
	std::string format_string_;
	std::string filename_delimit_;
};

// END DECLARATION
//======================================================================
// BEGIN IMPLEMENTATION

//======================================================================
// ELOTS c'tor
//======================================================================

ELOTS::ELOTS(Parameters const& pset)
    : ELdestination(pset().elDestConfig()), pid_(static_cast<int64_t>(getpid())), format_string_(pset().format_string()), filename_delimit_(pset().filename_delimit())
{
	// hostname
	char hostname_c[1024];
	hostname_ = (gethostname(hostname_c, 1023) == 0) ? hostname_c : "Unkonwn Host";

	// host ip address
	hostent* host = nullptr;
	host = gethostbyname(hostname_c);

	if (host != nullptr)
	{
		// ip address from hostname if the entry exists in /etc/hosts
		char* ip = inet_ntoa(*reinterpret_cast<struct in_addr*>(host->h_addr));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-pointer-arithmetic)
		hostaddr_ = ip;
	}
	else
	{
		// enumerate all network interfaces
		struct ifaddrs* ifAddrStruct = nullptr;
		struct ifaddrs* ifa = nullptr;
		void* tmpAddrPtr = nullptr;

		if (getifaddrs(&ifAddrStruct) != 0)
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
					tmpAddrPtr = &(reinterpret_cast<struct sockaddr_in*>(ifa->ifa_addr))->sin_addr;  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
					char addressBuffer[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
					hostaddr_ = addressBuffer;
				}

				else if (ifa->ifa_addr->sa_family == AF_INET6)
				{
					// a valid IPv6 address
					tmpAddrPtr = &(reinterpret_cast<struct sockaddr_in6*>(ifa->ifa_addr))->sin6_addr;  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
					char addressBuffer[INET6_ADDRSTRLEN];
					inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
					hostaddr_ = addressBuffer;
				}

				// find first non-local address
				if (!hostaddr_.empty() && (hostaddr_ != "127.0.0.1") && (hostaddr_ != "::1"))
				{
					break;
				}
			}

			if (hostaddr_.empty())
			{  // failed to find anything
				hostaddr_ = "127.0.0.1";
			}
		}
	}

	// get process name from '/proc/pid/cmdline'
	std::stringstream ss;
	ss << "//proc//" << pid_ << "//cmdline";
	std::ifstream procfile{ss.str().c_str()};

	std::string procinfo;

	if (procfile.is_open())
	{
		procfile >> procinfo;
		procfile.close();
	}

	size_t end = procinfo.find('\0');
	size_t start = procinfo.find_last_of('/', end);

	app_ = procinfo.substr(start + 1, end - start - 1);
}

//======================================================================
// Message prefix filler ( overriddes ELdestination::fillPrefix )
//======================================================================
void ELOTS::fillPrefix(std::ostringstream& oss, const ErrorObj& msg)
{
	const auto& xid = msg.xid();

	const auto& id = xid.id();
	const auto& module = xid.module();
	auto app = app_;
	char* cp = &format_string_[0];  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	char sev;
	bool msg_printed = false;
	std::string ossstr;
	// ossstr.reserve(100);

	for (; *cp != 0; ++cp)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
	{
		if (*cp != '%')
		{
			oss << *cp;
			continue;
		}
		if (*++cp == '\0')  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
		{                   // inc pas '%' and check if end
			// ending '%' gets printed
			oss << *cp;
			break;  // done
		}
		switch (*cp)
		{
			case 'A':
				oss << app;
				break;  // application
			case 'a':
				oss << hostaddr_;
				break;  // host address
			case 'd':
				oss << module;
				break;  // module name # Early
			case 'f':   // filename
				if (filename_delimit_.empty())
				{
					oss << msg.filename();
				}
				else if (filename_delimit_.size() == 1)
				{
					oss << (strrchr(&msg.filename()[0], filename_delimit_[0]) != nullptr  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
					            ? strrchr(&msg.filename()[0], filename_delimit_[0]) + 1   // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
					            : msg.filename());
				}
				else
				{
					const char* cp = strstr(&msg.filename()[0], &filename_delimit_[0]);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
					if (cp != nullptr)
					{
						// make sure to remove a part that ends with '/'
						cp += filename_delimit_.size() - 1;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
						while (*cp && *cp != '/') ++cp;
						++cp;  // increment past '/'
						oss << cp;
					}
					else
						oss << msg.filename();
				}
				break;
			case 'h':
				oss << hostname_;
				break;  // host name
			case 'L':
				oss << xid.severity().getName();
				break;  // severity
			case 'm':   // message
				// ossstr.clear();						 // incase message is repeated
				for (auto const& val : msg.items())
				{
					ossstr += val;  // Print the contents.
				}
				if (!ossstr.empty())
				{  // allow/check for "no message"
					if (ossstr.compare(0, 1, "\n") == 0)
					{
						ossstr.erase(0, 1);  // remove leading "\n" if present
					}
					if (ossstr.compare(ossstr.size() - 1, 1, "\n") == 0)
					{
						ossstr.erase(ossstr.size() - 1, 1);  // remove trailing "\n" if present
					}
					oss << ossstr;
				}
				msg_printed = true;
				break;
			case 'N':
				oss << id;
				break;  // category
			case 'P':
				oss << pid_;
				break;  // processID
			case 'r':
				oss << mf::GetIteration();
				break;  // run/iteration/event no #pre-events
			case 's':
				sev = xid.severity().getName()[0] | 0x20;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
				oss << sev;
				break;  // severity lower case
			case 'T':
				oss << format_.timestamp(msg.timestamp());
				break;  // timestamp
			case 'u':
				oss << std::to_string(msg.lineNumber());
				break;  // linenumber
			case '%':
				oss << '%';
				break;  // a '%' character
			default:
				oss << '%' << *cp;
				break;  // unknown - just print it w/ it's '%'
		}
	}
	if (!msg_printed)
	{
		for (auto const& val : msg.items())
		{
			ossstr += val;  // Print the contents.
		}
		if (ossstr.compare(0, 1, "\n") == 0)
		{
			ossstr.erase(0, 1);  // remove leading "\n" if present
		}
		if (ossstr.compare(ossstr.size() - 1, 1, "\n") == 0)
		{
			ossstr.erase(ossstr.size() - 1, 1);  // remove trailing "\n" if present
		}
		oss << ossstr;
	}
}

//======================================================================
// Message filler ( overriddes ELdestination::fillUsrMsg )
//======================================================================
void ELOTS::fillUsrMsg(std::ostringstream& oss __attribute__((__unused__)),
                       const ErrorObj& msg __attribute__((__unused__)))
{
	// UsrMsg filled above
}

//======================================================================
// Message router ( overriddes ELdestination::routePayload )
//======================================================================
void ELOTS::routePayload(const std::ostringstream& oss, const ErrorObj& /*msg*/) { std::cout << oss.str() << std::endl; }
}  // end namespace mfplugins
//======================================================================
//
// makePlugin function
//
//======================================================================

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START extern "C" {
#endif

EXTERN_C_FUNC_DECLARE_START auto makePlugin(const std::string& /*unused*/, const fhicl::ParameterSet& pset)
{
	return std::make_unique<mfplugins::ELOTS>(pset);
}
}  // namespace mfplugins

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
