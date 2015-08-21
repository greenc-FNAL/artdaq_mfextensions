#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/MessageLogger/ELseverityLevel.h"
#include "messagefacility/MessageLogger/MessageDrop.h"
#include "messagefacility/Utilities/exception.h"

// C/C++ includes
#include <iostream>
#include <memory>

// Boost includes
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>

namespace mfplugins {

  using mf::service::ELdestination;
  using mf::ELseverityLevel;
  using mf::ELstring;
  using mf::ErrorObj;
  using boost::asio::ip::udp;

  //======================================================================
  //
  // UDP destination plugin
  //
  //======================================================================

  class ELUDP : public ELdestination {
  public:

    ELUDP( const fhicl::ParameterSet& pset );

    virtual void fillPrefix  (       std::ostringstream&, const ErrorObj& ) override;
    virtual void fillUsrMsg  (       std::ostringstream&, const ErrorObj& ) override;
    virtual void fillSuffix  (       std::ostringstream&, const ErrorObj& ) override {}
    virtual void routePayload( const std::ostringstream&, const ErrorObj& ) override;

  private:
    boost::asio::io_service io_service_;
    udp::socket socket_;
    udp::endpoint remote_endpoint_;
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
  {
    boost::system::error_code ec;
    socket_.open(udp::v4());
    int port = pset.get<int>("port", 5140);
    socket_.set_option(boost::asio::socket_base::reuse_address(true),ec);
    if(ec) {
      std::cerr << "An error occurred setting reuse_address to true: "
                << ec.message() << std::endl;
    }
    std::string host = pset.get<std::string>("host", "227.128.12.27");

    if(boost::iequals(host, "Broadcast") || host == "255.255.255.255") {
      socket_.set_option(boost::asio::socket_base::broadcast(true),ec);
      remote_endpoint_ = udp::endpoint(boost::asio::ip::address_v4::broadcast(), port);
    }
    else
    {
      remote_endpoint_ = udp::endpoint(boost::asio::ip::address_v4::from_string(host), port);
    }

    socket_.connect(remote_endpoint_, ec);
    if(ec) { 
      std::cerr << "An Error occurred in connect(): " << ec.message() << std::endl
                << "  endpoint = " << remote_endpoint_ << std::endl;
    }
    //else {
    //  std::cout << "Successfully connected to remote endpoint = " << remote_endpoint_
    //            << std::endl;
    //}
  }

  //======================================================================
  // Message prefix filler ( overriddes ELdestination::fillPrefix )
  //======================================================================
  void ELUDP::fillPrefix( std::ostringstream& oss,const ErrorObj & msg ) {
    const auto& xid = msg.xid();

    oss << format.timestamp( msg.timestamp() )+ELstring("|");   // timestamp
    oss << xid.hostname+ELstring("|");                          // host name
    oss << xid.hostaddr+ELstring("|");                          // host address
    oss << xid.severity.getName()+ELstring("|");                // severity
    oss << xid.id+ELstring("|");                                // category
    oss << xid.application+ELstring("|");                       // application
    oss << xid.process+ELstring("|");
    oss << xid.pid<<ELstring("|");                              // process id
    oss << mf::MessageDrop::instance()->runEvent+ELstring("|"); // run/event no
    oss << xid.module+ELstring("|");                            // module name
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
  void ELUDP::routePayload( const std::ostringstream& oss, const ErrorObj& msg) {
    auto pid = msg.xid().pid;
    auto message = boost::asio::buffer("UDPMFMESSAGE" + std::to_string(pid) + "|" + oss.str());
    try {
      socket_.send_to(message, remote_endpoint_);
    }
    catch (boost::system::system_error& err) {
      std::cerr << "An exception occurred when trying to send a message to "
                << remote_endpoint_ << std::endl
                << "  message = " << oss.str() << std::endl
                << "  exception = " << err.what() << std::endl;
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
