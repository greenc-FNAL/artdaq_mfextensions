#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/MessageLogger/ELseverityLevel.h"
#include "messagefacility/MessageLogger/MessageDrop.h"
#include "messagefacility/Utilities/exception.h"

// C/C++ includes
#include <iostream>
#include <memory>

// Boost asio
#include <boost/asio.hpp>

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
    int port = pset.get<int>("port", 5140);
    std::string host = pset.get<std::string>("host", "NULL");
    if(host == "NULL") {
    socket_.set_option(boost::asio::socket_base::reuse_address(true));
    socket_.set_option(boost::asio::socket_base::broadcast(true));
    remote_endpoint_ = udp::endpoint(boost::asio::ip::address_v4::broadcast(), port);
    }
    else
    {
      remote_endpoint_ = udp::endpoint(boost::asio::ip::address_v4::from_string(host), port);
    }
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
    auto message = boost::asio::buffer(std::to_string(pid) + oss.str());
    socket_.send_to(message, remote_endpoint_);
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
