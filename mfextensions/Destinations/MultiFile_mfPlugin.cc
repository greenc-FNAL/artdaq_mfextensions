#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ostream_handle.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/MessageLogger/ELseverityLevel.h"
#include "messagefacility/MessageLogger/MessageDrop.h"
#include "messagefacility/Utilities/exception.h"

#include <fstream>

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

  class ELMultiFileOutput : public ELdestination {
  public:

    ELMultiFileOutput( const fhicl::ParameterSet& pset );
    virtual ~ELMultiFileOutput() {}

    virtual void routePayload( const std::ostringstream&, const ErrorObj& ) override;
    virtual void flush() override;

  private:
    std::string baseDir_;
    bool append_;
    std::unordered_map<std::string, std::unique_ptr<cet::ostream_handle> > outputs_;
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
  {
  }

  //======================================================================
  // Message router ( overriddes ELdestination::routePayload )
  //======================================================================
  void ELMultiFileOutput::routePayload( const std::ostringstream& oss, const ErrorObj& msg) {
	const auto& xid = msg.xid();
	std::string fileName = baseDir_ + "/" + xid.application + "-" + std::to_string(xid.pid) + ".log";
	if(outputs_.count(fileName) == 0) {
	  outputs_[fileName] = std::make_unique<cet::ostream_owner>(fileName.c_str(), append_ ? std::ios::app : std::ios::trunc);
	}
	*outputs_[fileName] << oss.str();
	flush();
  }

  void ELMultiFileOutput::flush() {
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
