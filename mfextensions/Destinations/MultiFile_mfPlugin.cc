#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ostream_handle.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/ConfigurationTable.h"

#include "cetlib/compiler_macros.h"
#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/Utilities/exception.h"

#include <fstream>

namespace mfplugins {
using mf::ErrorObj;
using mf::service::ELdestination;

/// <summary>
/// Message Facility Destination which automatically opens files and sorts messages into them based on given criteria
/// </summary>
class ELMultiFileOutput : public ELdestination
{
public:
	/**
	 * \brief Configuration parameters for ELMultiFileOutput
	 */
	struct Config
	{
		/// ELdestination common config parameters
		fhicl::TableFragment<ELdestination::Config> elDestConfig;
		/// "base_directory" (Default: "/tmp"): Directory where log files will be created
		fhicl::Atom<std::string> baseDir = fhicl::Atom<std::string>{
		    fhicl::Name{"base_directory"}, fhicl::Comment{"Directory where log files will be created"}, "/tmp"};
		/// "append" (Default: true): Append to existing log files
		fhicl::Atom<bool> append =
		    fhicl::Atom<bool>{fhicl::Name{"append"}, fhicl::Comment{"Append to existing log files"}, true};
		/// "use_hostname" (Default: true): Use the hostname when generating log file names
		fhicl::Atom<bool> useHostname = fhicl::Atom<bool>{
		    fhicl::Name{"use_hostname"}, fhicl::Comment{"Use the hostname when generating log file names"}, true};
		/// "use_application" (Default: true): Use the application field when generating log file names
		fhicl::Atom<bool> useApplication =
		    fhicl::Atom<bool>{fhicl::Name{"use_application"},
		                      fhicl::Comment{"Use the application field when generating log file names"}, true};
		/// "use_category" (Default: false): Use the category field when generating log file names
		fhicl::Atom<bool> useCategory = fhicl::Atom<bool>{
		    fhicl::Name{"use_category"}, fhicl::Comment{"Use the category field when generating log file names"}, false};
		/// "use_module" (Default: false): Use the module field when generating log file names
		fhicl::Atom<bool> useModule = fhicl::Atom<bool>{
		    fhicl::Name{"use_module"}, fhicl::Comment{"Use the module field when generating log file names"}, false};
	};
	/// Used for ParameterSet validation
	using Parameters = fhicl::WrappedTable<Config>;

public:
	/// <summary>
	/// ELMultiFileOutput Constructor
	/// </summary>
	/// <param name="pset">ParameterSet used to configure ELMultiFileOutput</param>
	explicit ELMultiFileOutput(Parameters const& pset);

	/// <summary>
	/// Default virtual Destructor
	/// </summary>
	~ELMultiFileOutput() override = default;

	/**
	 * \brief Serialize a MessageFacility message to the output
	 * \param oss Stringstream object containing message data
	 * \param msg MessageFacility object containing header information
	 */
	void routePayload(const std::ostringstream& oss, const ErrorObj& msg) override;

	/**
	 * \brief Flush any text in the ostream buffer to disk
	 */
	void flush() override;

private:
	ELMultiFileOutput(ELMultiFileOutput const&) = delete;
	ELMultiFileOutput(ELMultiFileOutput&&) = delete;
	ELMultiFileOutput& operator=(ELMultiFileOutput const&) = delete;
	ELMultiFileOutput& operator=(ELMultiFileOutput&&) = delete;

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
    : ELdestination(pset().elDestConfig()), baseDir_(pset().baseDir()), append_(pset().append()), useHost_(pset().useHostname()), useApplication_(pset().useApplication()), useCategory_(pset().useCategory()), useModule_(pset().useModule()) {}

//======================================================================
// Message router ( overriddes ELdestination::routePayload )
//======================================================================
void ELMultiFileOutput::routePayload(const std::ostringstream& oss, const ErrorObj& msg)
{
	const auto& xid = msg.xid();
	std::string fileName = baseDir_ + "/";
	if (useModule_)
	{
		fileName += xid.module() + "-";
	}
	if (useCategory_)
	{
		fileName += xid.id() + "-";
	}
	if (useApplication_)
	{
		fileName += xid.application() + "-";
	}
	if (useHost_)
	{
		fileName += xid.hostname() + "-";
	}
	fileName += std::to_string(xid.pid()) + ".log";
	if (outputs_.count(fileName) == 0)
	{
		outputs_[fileName] =
		    std::make_unique<cet::ostream_handle>(fileName.c_str(), append_ ? std::ios::app : std::ios::trunc);
	}
	*outputs_[fileName] << oss.str();
	flush();
}

void ELMultiFileOutput::flush()
{
	for (auto& output : outputs_)
	{
		output.second->flush();
	}
}
}  // end namespace mfplugins

//======================================================================
//
// makePlugin function
//
//======================================================================

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START extern "C" {
#endif

EXTERN_C_FUNC_DECLARE_START
auto makePlugin(const std::string& /*unused*/, const fhicl::ParameterSet& pset)
{
	return std::make_unique<mfplugins::ELMultiFileOutput>(pset);
}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
