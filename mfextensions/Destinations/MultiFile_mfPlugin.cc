#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ostream_handle.h"
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

#include <fstream>

#define CET_DETAIL 1
#if CET_DETAIL
#define CET_NS cet::detail
#else
#define CET_NS cet
#endif

namespace mfplugins {

  using mf::service::ELdestination;
  using mf::ELseverityLevel;
  using mf::ErrorObj;
#ifndef NO_MF_UTILITIES
  using mf::service::ELcontextSupplier;
#endif

  //======================================================================
  //
  // TRACE destination plugin
  //
  //======================================================================

  class ELMultiFileOutput : public ELdestination {
  public:

    ELMultiFileOutput( const fhicl::ParameterSet& pset );
    virtual ~ELMultiFileOutput() {}

    virtual void routePayload( const std::ostringstream&, const ErrorObj&
#ifndef NO_MF_UTILITIES
							   , const ELcontextSupplier&
#endif
 ) override;
    virtual void flush(
#ifndef NO_MF_UTILITIES
							   const ELcontextSupplier&
#endif
) override;

  private:
    std::string baseDir_;
    bool append_;
    std::unordered_map<std::string, std::unique_ptr<cet::ostream_handle> > outputs_;

    bool useHost_;
    bool useApplication_;
    bool useCategory_;
    bool useModule_;
  };

  // END DECLARATION
  //======================================================================
  // BEGIN IMPLEMENTATION


  //======================================================================
  // ELMultiFileOutput c'tor
  //======================================================================

  ELMultiFileOutput::ELMultiFileOutput( const fhicl::ParameterSet& pset )
    : ELdestination( pset )
    , baseDir_(pset.get<std::string>("base_directory","/tmp"))
    , append_(pset.get<bool>("append",true))
    , useHost_(pset.get<bool>("use_hostname",true))
    , useApplication_(pset.get<bool>("use_application", true))
    , useCategory_(pset.get<bool>("use_category", false))
    , useModule_(pset.get<bool>("use_module",false))
  {
  }

  //======================================================================
  // Message router ( overriddes ELdestination::routePayload )
  //======================================================================
  void ELMultiFileOutput::routePayload( const std::ostringstream& oss, const ErrorObj& msg
#ifndef NO_MF_UTILITIES
							   , ELcontextSupplier const& sup
#endif
) {
	const auto& xid = msg.xid();
	std::string fileName = baseDir_ + "/";
	if(useModule_) { fileName += xid.module + "-"; }
        if(useCategory_) { fileName += xid.id + "-"; }
        if(useApplication_) {fileName += xid.application + "-"; }
	if(useHost_) { fileName += xid.hostname +"-";}
        fileName += std::to_string(xid.pid) + ".log";
	if(outputs_.count(fileName) == 0) {
	  outputs_[fileName] = std::make_unique<CET_NS::ostream_owner>(fileName.c_str(), append_ ? std::ios::app : std::ios::trunc);
	}
	*outputs_[fileName] << oss.str();
	flush(
#ifndef NO_MF_UTILITIES
							   sup
#endif
);
  }

  void ELMultiFileOutput::flush(
#ifndef NO_MF_UTILITIES
							   ELcontextSupplier const&
#endif
) {
    for(auto i = outputs_.begin(); i != outputs_.end(); ++i) {
      (*i).second->stream().flush();
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

    return std::make_unique<mfplugins::ELMultiFileOutput>( pset );

  }

}
DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
