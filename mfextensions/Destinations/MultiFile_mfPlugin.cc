#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ostream_handle.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#ifdef NO_MF_UTILITIES
# include "messagefacility/MessageLogger/ELseverityLevel.h"
#else
# include "messagefacility/Utilities/ELseverityLevel.h"
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
#  include "messagefacility/MessageService/ELcontextSupplier.h"
# endif
#endif
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
# include "messagefacility/MessageLogger/MessageDrop.h"
#endif
#include "messagefacility/Utilities/exception.h"

#include <fstream>

namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
	using mf::ErrorObj;
#ifndef NO_MF_UTILITIES
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	using mf::service::ELcontextSupplier;
# endif
#endif

	//======================================================================
	//
	// TRACE destination plugin
	//
	//======================================================================

	class ELMultiFileOutput : public ELdestination
	{
	public:

		ELMultiFileOutput(const fhicl::ParameterSet& pset);

		virtual ~ELMultiFileOutput() {}

		virtual void routePayload(const std::ostringstream&, const ErrorObj&
#ifndef NO_MF_UTILITIES
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		                          , const ELcontextSupplier&
# endif
#endif
		) override;

		virtual void flush(
#ifndef NO_MF_UTILITIES
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
			const ELcontextSupplier&
# endif
#endif
		) override;

	private:
		std::string baseDir_;
		bool append_;
		std::unordered_map<std::string, std::unique_ptr<cet::ostream_handle>> outputs_;

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

	ELMultiFileOutput::ELMultiFileOutput(const fhicl::ParameterSet& pset)
		: ELdestination(pset)
		, baseDir_(pset.get<std::string>("base_directory", "/tmp"))
		, append_(pset.get<bool>("append", true))
		, useHost_(pset.get<bool>("use_hostname", true))
		, useApplication_(pset.get<bool>("use_application", true))
		, useCategory_(pset.get<bool>("use_category", false))
		, useModule_(pset.get<bool>("use_module", false)) { }

	//======================================================================
	// Message router ( overriddes ELdestination::routePayload )
	//======================================================================
	void ELMultiFileOutput::routePayload(const std::ostringstream& oss, const ErrorObj& msg
#ifndef NO_MF_UTILITIES
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	                                     , ELcontextSupplier const& sup
# endif
#endif
	)
	{
		const auto& xid = msg.xid();
		std::string fileName = baseDir_ + "/";
#      if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		if (useModule_) { fileName += xid.module() + "-"; }
		if (useCategory_) { fileName += xid.id() + "-"; }
		if (useApplication_) { fileName += xid.application() + "-"; }
		if (useHost_) { fileName += xid.hostname() + "-"; }
		fileName += std::to_string(xid.pid()) + ".log";
#      else
		if (useModule_) { fileName += xid.module + "-"; }
		if (useCategory_) { fileName += xid.id + "-"; }
		if (useApplication_) { fileName += xid.application + "-"; }
		if (useHost_) { fileName += xid.hostname + "-"; }
		fileName += std::to_string(xid.pid) + ".log";
#      endif
		if (outputs_.count(fileName) == 0)
		{
#ifndef CETLIB_EXPOSES_OSTREAM_OWNER // New cetlib
			outputs_[fileName] = std::make_unique<cet::ostream_handle>(fileName.c_str(), append_ ? std::ios::app : std::ios::trunc);
#else // Old cetlib
	  outputs_[fileName] = std::make_unique<cet::ostream_owner>(fileName.c_str(), append_ ? std::ios::app : std::ios::trunc);
#endif
		}
		*outputs_[fileName] << oss.str();
		flush(
#ifndef NO_MF_UTILITIES
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
			sup
# endif
#endif
		);
	}

	void ELMultiFileOutput::flush(
#ifndef NO_MF_UTILITIES
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		ELcontextSupplier const&
# endif
#endif
	)
	{
		for (auto i = outputs_.begin(); i != outputs_.end(); ++i)
		{
#ifndef CETLIB_EXPOSES_OSTREAM_OWNER // New cetlib
			(*i).second->flush();
#else // Old cetlib
      (*i).second->stream().flush();
#endif
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
		return std::make_unique<mfplugins::ELMultiFileOutput>(pset);
	}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
