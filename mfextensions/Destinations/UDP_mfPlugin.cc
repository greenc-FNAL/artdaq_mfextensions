#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#ifdef NO_MF_UTILITIES
#include "messagefacility/MessageLogger/ELseverityLevel.h"
#else
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/MessageService/ELcontextSupplier.h"
#endif
#include "messagefacility/MessageLogger/MessageDrop.h"
#include "messagefacility/Utilities/exception.h"

// C/C++ includes
#include <iostream>
#include <memory>
#include <algorithm>

// Boost includes
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

namespace mfplugins {

  using mf::service::ELdestination;
  using mf::ELseverityLevel;
  using mf::ErrorObj;
#ifndef NO_MF_UTILITIES
  using mf::service::ELcontextSupplier;
#endif
  using boost::asio::ip::udp;

  //======================================================================
  //
  // UDP destination plugin
  //
  //======================================================================

  class ELUDP : public ELdestination {
  public:

    ELUDP( const fhicl::ParameterSet& pset );

    virtual void fillPrefix  (       std::ostringstream&, const ErrorObj&
#ifndef NO_MF_UTILITIES
							   , const ELcontextSupplier&
#endif
 ) override;
    virtual void fillUsrMsg  (       std::ostringstream&, const ErrorObj& ) override;
    virtual void fillSuffix  (       std::ostringstream&, const ErrorObj& ) override {}
    virtual void routePayload( const std::ostringstream&, const ErrorObj&
#ifndef NO_MF_UTILITIES
							   , const ELcontextSupplier&
#endif
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
  };

  // END DECLARATION
  //======================================================================
  // BEGIN IMPLEMENTATION


  //======================================================================
  // ELUDP c'tor
  //======================================================================

  ELUDP::ELUDP( const fhicl::ParameterSet& pset )
    : ELdestination( pset )
    , io_service_()
    , socket_(io_service_)
    , remote_endpoint_()
    , consecutive_success_count_(0)
    , error_count_(0)
    , next_error_report_(1)
    , error_report_backoff_factor_(pset.get<int>("error_report_backoff_factor", 10))
    , error_max_(pset.get<int>("error_turnoff_threshold", 1))
    , host_(pset.get<std::string>("host", "227.128.12.27"))
    , port_(pset.get<int>("port",5140))
	  , seqNum_(0)
  {
    reconnect_();
  }

  void ELUDP::reconnect_(bool quiet)
  {
    boost::system::error_code ec;
    socket_.open(udp::v4());
    socket_.set_option(boost::asio::socket_base::reuse_address(true),ec);
    if(ec && !quiet) {
      std::cerr << "An error occurred setting reuse_address to true: "
                << ec.message() << std::endl;
    }

    if(boost::iequals(host_, "Broadcast") || host_ == "255.255.255.255") {
      socket_.set_option(boost::asio::socket_base::broadcast(true),ec);
      remote_endpoint_ = udp::endpoint(boost::asio::ip::address_v4::broadcast(), port_);
    }
    else
    {
      udp::resolver resolver(io_service_);
      udp::resolver::query query(udp::v4(), host_, std::to_string(port_));
	remote_endpoint_ = *resolver.resolve(query);
    }

    socket_.connect(remote_endpoint_, ec);
    if(ec) { 
      if(!quiet) {
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
  void ELUDP::fillPrefix( std::ostringstream& oss,const ErrorObj & msg
#ifndef NO_MF_UTILITIES
							   , ELcontextSupplier const&
#endif
						  ) {
    const auto& xid = msg.xid();

    auto id = xid.id;
    auto app = xid.application;
    auto process = xid.process;
    auto module = xid.module; 
    std::replace(id.begin(), id.end(), '|', '!');
    std::replace(app.begin(), app.end(), '|', '!');
    std::replace(process.begin(), process.end(), '|', '!');
    std::replace(module.begin(), module.end(), '|', '!');

    oss << format.timestamp( msg.timestamp() )+"|";   // timestamp
	oss << std::to_string(++seqNum_) + "|";	          // sequence number
    oss << xid.hostname+"|";                          // host name
    oss << xid.hostaddr+"|";                          // host address
    oss << xid.severity.getName()+"|";                // severity
    oss << id+"|";                                // category
    oss << app+"|";                       // application
    oss << process+"|";
    oss << xid.pid<<"|";                              // process id
    oss << mf::MessageDrop::instance()->runEvent+"|"; // run/event no
    oss << module+"|";                            // module name
  }

  //======================================================================
  // Message filler ( overriddes ELdestination::fillUsrMsg )
  //======================================================================
  void ELUDP::fillUsrMsg( std::ostringstream& oss,const ErrorObj & msg ) {

    std::ostringstream tmposs;
    ELdestination::fillUsrMsg( tmposs, msg );

    // remove leading "\n" if present
    const std::string& usrMsg = !tmposs.str().compare(0,1,"\n") ? tmposs.str().erase(0,1) : tmposs.str();

    oss << usrMsg;
  }

  //======================================================================
  // Message router ( overriddes ELdestination::routePayload )
  //======================================================================
  void ELUDP::routePayload( const std::ostringstream& oss, const ErrorObj& msg
#ifndef NO_MF_UTILITIES
							   , ELcontextSupplier const&
#endif
) {
    if(error_count_ < error_max_) {
    auto pid = msg.xid().pid;
	//std::cout << oss.str() << std::endl;
	auto string = "UDPMFMESSAGE" + std::to_string(pid) + "|" + oss.str();
	//std::cout << string << std::endl;
    auto message = boost::asio::buffer(string);
    bool error = true;
    try {
      socket_.send_to(message, remote_endpoint_);
      ++consecutive_success_count_;
      if (consecutive_success_count_ >= 5) {
        error_count_ = 0;
        next_error_report_ = 1;
      }
      error = false;
    }
    catch (boost::system::system_error& err) {
      consecutive_success_count_ = 0;
      ++error_count_;
      if (error_count_ == next_error_report_) {
        std::cerr << "An exception occurred when trying to send message " << std::to_string(seqNum_) << " to "
                  << remote_endpoint_ << std::endl
                  << "  message = " << oss.str() << std::endl
                  << "  exception = " << err.what() << std::endl;
        next_error_report_ *= error_report_backoff_factor_;
      }
    }
    if(error) {
      try{
	reconnect_(true);
      }
      catch(...)
	{}
    }
  }
  }
} // end namespace mfplugins

//======================================================================
//
// makePlugin function
//
//======================================================================

extern "C" {

  auto makePlugin( const std::string&,
                   const fhicl::ParameterSet& pset) {

    return std::make_unique<mfplugins::ELUDP>( pset );

  }

}
DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
