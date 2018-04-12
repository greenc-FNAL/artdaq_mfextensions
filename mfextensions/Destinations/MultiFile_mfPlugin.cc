#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ostream_handle.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
# include "messagefacility/Utilities/ELseverityLevel.h"
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
# include "messagefacility/MessageService/ELcontextSupplier.h"
# include "messagefacility/MessageLogger/MessageDrop.h"
#endif
#include "messagefacility/Utilities/exception.h"
#include "cetlib/compiler_macros.h"

#include <fstream>

namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
	using mf::ErrorObj;
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	using mf::service::ELcontextSupplier;
#endif

	/// <summary>
	/// Message Facility Destination which automatically opens files and sorts messages into them based on given criteria
	/// </summary>
	class ELMultiFileOutput : public ELdestination
	{
#if MESSAGEFACILITY_HEX_VERSION >= 0x20103
		struct Config
		{
			fhicl::TableFragment<ELdestination::Config> elDestConfig;
			fhicl::Atom<std::string> baseDir{ fhicl::Name{ "base_directory" },fhicl::Comment{ "Directory where log files will be created" },"/tmp" };
			fhicl::Atom<bool> append{ fhicl::Name{ "append" },fhicl::Comment{ "Append to existing log files" },true };
			fhicl::Atom<bool> useHostname{ fhicl::Name{ "use_hostname" },fhicl::Comment{ "Use the hostname when generating log file names" },true };
			fhicl::Atom<bool> useApplication{ fhicl::Name{ "use_application" },fhicl::Comment{ "Use the application field when generating log file names" },true };
			fhicl::Atom<bool> useCategory{ fhicl::Name{ "use_category" },fhicl::Comment{ "Use the category field when generating log file names" },false };
			fhicl::Atom<bool> useModule{ fhicl::Name{ "use_module" },fhicl::Comment{ "Use the module field when generating log file names" },false };
	};
		using Parameters = fhicl::WrappedTable<Config>;
#endif
	public:
#if MESSAGEFACILITY_HEX_VERSION < 0x20103 // v2_01_03 is s58, pre v2_01_03 is s50
		ELMultiFileOutput(const fhicl::ParameterSet& pset);
#else
		ELMultiFileOutput(Parameters const& pset);
#endif

		virtual ~ELMultiFileOutput() {}

		virtual void routePayload(const std::ostringstream&, const ErrorObj&
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
								  , const ELcontextSupplier&
#endif
		) override;

		virtual void flush(
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
			const ELcontextSupplier&
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

#if MESSAGEFACILITY_HEX_VERSION < 0x20103 // v2_01_03 is s58, pre v2_01_03 is s50
	ELMultiFileOutput::ELMultiFileOutput(const fhicl::ParameterSet& pset)
		: ELdestination(pset)
		, baseDir_(pset.get<std::string>("base_directory", "/tmp"))
		, append_(pset.get<bool>("append", true))
		, useHost_(pset.get<bool>("use_hostname", true))
		, useApplication_(pset.get<bool>("use_application", true))
		, useCategory_(pset.get<bool>("use_category", false))
		, useModule_(pset.get<bool>("use_module", false)) { }
#else
	ELMultiFileOutput::ELMultiFileOutput(Parameters const& pset)
		: ELdestination(pset().elDestConfig())
		, baseDir_(pset().baseDir())
		, append_(pset().append())
		, useHost_(pset().useHostname())
		, useApplication_(pset().useApplication())
		, useCategory_(pset().useCategory())
		, useModule_(pset().useModule()) { }
#endif

	//======================================================================
	// Message router ( overriddes ELdestination::routePayload )
	//======================================================================
	void ELMultiFileOutput::routePayload(const std::ostringstream& oss, const ErrorObj& msg
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
										 , ELcontextSupplier const& sup
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
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
			sup
#endif
		);
	}

	void ELMultiFileOutput::flush(
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		ELcontextSupplier const&
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

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START extern "C" {
#endif

EXTERN_C_FUNC_DECLARE_START
	auto makePlugin(const std::string&,
					const fhicl::ParameterSet& pset)
	{
		return std::make_unique<mfplugins::ELMultiFileOutput>(pset);
	}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
