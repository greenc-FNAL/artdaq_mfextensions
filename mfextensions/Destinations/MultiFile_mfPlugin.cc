#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ostream_handle.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
# include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/Utilities/exception.h"
#include "cetlib/compiler_macros.h"

#include <fstream>

namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
	using mf::ErrorObj;

	/// <summary>
	/// Message Facility Destination which automatically opens files and sorts messages into them based on given criteria
	/// </summary>
	class ELMultiFileOutput : public ELdestination
	{
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

	public:
		/// <summary>
		/// ELMultiFileOutput Constructor
		/// </summary>
		/// <param name="pset">ParameterSet used to configure ELMultiFileOutput</param>
		ELMultiFileOutput(Parameters const& pset);

		/// <summary>
		/// Default virtual Destructor
		/// </summary>
		virtual ~ELMultiFileOutput() {}

		/**
		* \brief Serialize a MessageFacility message to the output
		* \param oss Stringstream object containing message data
		* \param msg MessageFacility object containing header information
		*/
		virtual void routePayload(const std::ostringstream& oss, const ErrorObj& msg) override;

		/**
		* \brief Flush any text in the ostream buffer to disk
		*/
		virtual void flush() override;

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
	ELMultiFileOutput::ELMultiFileOutput(Parameters const& pset)
		: ELdestination(pset().elDestConfig())
		, baseDir_(pset().baseDir())
		, append_(pset().append())
		, useHost_(pset().useHostname())
		, useApplication_(pset().useApplication())
		, useCategory_(pset().useCategory())
		, useModule_(pset().useModule())
	{}

	//======================================================================
	// Message router ( overriddes ELdestination::routePayload )
	//======================================================================
	void ELMultiFileOutput::routePayload(const std::ostringstream& oss, const ErrorObj& msg)
	{
		const auto& xid = msg.xid();
		std::string fileName = baseDir_ + "/";
		if (useModule_) { fileName += xid.module() + "-"; }
		if (useCategory_) { fileName += xid.id() + "-"; }
		if (useApplication_) { fileName += xid.application() + "-"; }
		if (useHost_) { fileName += xid.hostname() + "-"; }
		fileName += std::to_string(xid.pid()) + ".log";
		if (outputs_.count(fileName) == 0)
		{
			outputs_[fileName] = std::make_unique<cet::ostream_handle>(fileName.c_str(), append_ ? std::ios::app : std::ios::trunc);
		}
		*outputs_[fileName] << oss.str();
		flush();
	}

	void ELMultiFileOutput::flush()
	{
		for (auto i = outputs_.begin(); i != outputs_.end(); ++i)
		{
			(*i).second->flush();
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
