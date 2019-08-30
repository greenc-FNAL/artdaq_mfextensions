#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

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
using mf::ELseverityLevel;
using mf::ErrorObj;
using mf::service::ELdestination;

/// <summary>
/// Message Facility OTS Console Destination
/// Formats messages into Ryan's favorite format for OTS
/// </summary>
class ELOTS : public ELdestination {
 public:
  /**
   * \brief Configuration Parameters for ELOTS
   */
  struct Config {
    /// ELDestination common config parameters
    fhicl::TableFragment<ELdestination::Config> elDestConfig;
    fhicl::Atom<std::string> format_string = fhicl::Atom<std::string>{
        fhicl::Name{"format_string"}, fhicl::Comment{"Format specifier for printing to console. %% => '%' ... "}, ""};
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
   * \param e MessageFacility object containing header information
   */
  virtual void fillPrefix(std::ostringstream& o, const ErrorObj& e) override;

  /**
   * \brief Fill the "User Message" portion of the message
   * \param o Output stringstream
   * \param e MessageFacility object containing header information
   */
  virtual void fillUsrMsg(std::ostringstream& o, const ErrorObj& e) override;

  /**
   * \brief Fill the "Suffix" portion of the message (Unused)
   */
  virtual void fillSuffix(std::ostringstream&, const ErrorObj&) override {}

  /**
   * \brief Serialize a MessageFacility message to the output
   * \param o Stringstream object containing message data
   * \param e MessageFacility object containing header information
   */
  virtual void routePayload(const std::ostringstream& o, const ErrorObj& e) override;

 private:
  // Other stuff
  long pid_;
  std::string hostname_;
  std::string hostaddr_;
  std::string app_;
};

// END DECLARATION
//======================================================================
// BEGIN IMPLEMENTATION

//======================================================================
// ELOTS c'tor
//======================================================================

ELOTS::ELOTS(Parameters const& pset) : ELdestination(pset().elDestConfig()), pid_(static_cast<long>(getpid())) {
  // hostname
  char hostname_c[1024];
  hostname_ = (gethostname(hostname_c, 1023) == 0) ? hostname_c : "Unkonwn Host";

  // host ip address
  hostent* host = nullptr;
  host = gethostbyname(hostname_c);

  if (host != nullptr) {
    // ip address from hostname if the entry exists in /etc/hosts
    char* ip = inet_ntoa(*(struct in_addr*)host->h_addr);
    hostaddr_ = ip;
  } else {
    // enumerate all network interfaces
    struct ifaddrs* ifAddrStruct = nullptr;
    struct ifaddrs* ifa = nullptr;
    void* tmpAddrPtr = nullptr;

    if (getifaddrs(&ifAddrStruct)) {
      // failed to get addr struct
      hostaddr_ = "127.0.0.1";
    } else {
      // iterate through all interfaces
      for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) {
          // a valid IPv4 addres
          tmpAddrPtr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
          char addressBuffer[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
          hostaddr_ = addressBuffer;
        }

        else if (ifa->ifa_addr->sa_family == AF_INET6) {
          // a valid IPv6 address
          tmpAddrPtr = &((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
          char addressBuffer[INET6_ADDRSTRLEN];
          inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
          hostaddr_ = addressBuffer;
        }

        // find first non-local address
        if (!hostaddr_.empty() && hostaddr_.compare("127.0.0.1") && hostaddr_.compare("::1")) break;
      }

      if (hostaddr_.empty())  // failed to find anything
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
  std::ifstream procfile{ss.str().c_str()};

  std::string procinfo;

  if (procfile.is_open()) {
    procfile >> procinfo;
    procfile.close();
  }

  size_t end = procinfo.find('\0');
  size_t start = procinfo.find_last_of('/', end);

  app_ = procinfo.substr(start + 1, end - start - 1);
#endif
}

//======================================================================
// Message prefix filler ( overriddes ELdestination::fillPrefix )
//======================================================================
void ELOTS::fillPrefix(std::ostringstream& oss, const ErrorObj& msg) {
  const auto& xid = msg.xid();

  auto id = xid.id();
  auto module = xid.module();
  auto app = app_;

  oss << format_.timestamp(msg.timestamp()) << ": ";  // timestamp
  oss << app << " (";                                 // application
  oss << pid_ << ") ";
  oss << hostname_ << " (";                // host name
  oss << hostaddr_ << "): ";               // host address
  oss << xid.severity().getName() << "|";  // severity
  oss << id << "|";                        // category
                                           // oss << mf::GetIteration() << "|";  // run/event no #pre-events
                                           // oss << module << "|";              // module name # Early
  oss << msg.filename() << ":[" << std::to_string(msg.lineNumber()) << "]: ";
}

//======================================================================
// Message filler ( overriddes ELdestination::fillUsrMsg )
//======================================================================
void ELOTS::fillUsrMsg(std::ostringstream& oss, const ErrorObj& msg) {
  std::ostringstream tmposs;
  // Print the contents.
  for (auto const& val : msg.items()) {
    tmposs << val;
  }

  // remove leading "\n" if present
  const std::string& usrMsg = !tmposs.str().compare(0, 1, "\n") ? tmposs.str().erase(0, 1) : tmposs.str();

  oss << usrMsg;
}

//======================================================================
// Message router ( overriddes ELdestination::routePayload )
//======================================================================
void ELOTS::routePayload(const std::ostringstream& oss, const ErrorObj&) { std::cout << oss.str() << std::endl; }
}  // end namespace mfplugins
//======================================================================
//
// makePlugin function
//
//======================================================================

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START extern "C" {
#endif

EXTERN_C_FUNC_DECLARE_START auto makePlugin(const std::string&, const fhicl::ParameterSet& pset) {
  return std::make_unique<mfplugins::ELOTS>(pset);
}
}  // namespace mfplugins

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
