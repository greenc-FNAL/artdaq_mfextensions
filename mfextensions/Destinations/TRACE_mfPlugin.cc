#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/MessageLogger/ELseverityLevel.h"
#include "messagefacility/MessageLogger/MessageDrop.h"
#include "messagefacility/Utilities/exception.h"

#include "tracelib.h"

#define TRACE_NAME "MessageFacility"

namespace mfplugins {

  using mf::service::ELdestination;
  using mf::ELseverityLevel;
  using mf::ELstring;
  using mf::ErrorObj;

  //======================================================================
  //
  // TRACE destination plugin
  //
  //======================================================================

  class ELTRACE : public ELdestination {
  public:

    ELTRACE( const fhicl::ParameterSet& pset );

    virtual void fillPrefix  (       std::ostringstream&, const ErrorObj& ) override;
    virtual void fillUsrMsg  (       std::ostringstream&, const ErrorObj& ) override;
    virtual void fillSuffix  (       std::ostringstream&, const ErrorObj& ) override {}
    virtual void routePayload( const std::ostringstream&, const ErrorObj& ) override;

  private:
	int trace_level_offset_;
    int consecutive_success_count_;
    int error_count_;
    int next_error_report_;
    int error_report_backoff_factor_;
  };

  // END DECLARATION
  //======================================================================
  // BEGIN IMPLEMENTATION


  //======================================================================
  // ELTRACE c'tor
  //======================================================================

  ELTRACE::ELTRACE( const fhicl::ParameterSet& pset )
    : ELdestination( pset )
	, trace_level_offset_(pset.get<int>("level_offset", 0))
    , consecutive_success_count_(0)
    , error_count_(0)
    , next_error_report_(1)
    , error_report_backoff_factor_()
  {
    error_report_backoff_factor_ = pset.get<int>("error_report_backoff_factor", 10);
	TRACE(trace_level_offset_, "ELTRACE MessageLogger destination plugin initialized.");
  }

  //======================================================================
  // Message prefix filler ( overriddes ELdestination::fillPrefix )
  //======================================================================
  void ELTRACE::fillPrefix( std::ostringstream& oss,const ErrorObj & msg ) {
    const auto& xid = msg.xid();

    oss << xid.application + ELstring(", ");                       // application
    oss << xid.id + ELstring(": ");                                // category
	// oss << mf::MessageDrop::instance()->runEvent + ELstring(" "); // run/event no
    // oss << xid.module+ELstring(": ");                            // module name
  }

  //======================================================================
  // Message filler ( overriddes ELdestination::fillUsrMsg )
  //======================================================================
  void ELTRACE::fillUsrMsg( std::ostringstream& oss,const ErrorObj & msg ) {

    std::ostringstream tmposs;
    ELdestination::fillUsrMsg( tmposs, msg );

    // remove leading "\n" if present
    const std::string& usrMsg = !tmposs.str().compare(0,1,"\n") ? tmposs.str().erase(0,1) : tmposs.str();

    oss << usrMsg;
  }

  //======================================================================
  // Message router ( overriddes ELdestination::routePayload )
  //======================================================================
  void ELTRACE::routePayload( const std::ostringstream& oss, const ErrorObj& msg) {
    auto message = oss.str();
	const auto& xid = msg.xid();
	auto level = trace_level_offset_ + xid.severity.getLevel();
	TRACE(level, message);
  
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

    return std::make_unique<mfplugins::ELTRACE>( pset );

  }

}
DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
