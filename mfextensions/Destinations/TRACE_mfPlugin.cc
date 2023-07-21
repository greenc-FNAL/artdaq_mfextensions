/**
 * @file TRACE_mfPlugin.cc
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ProvideMakePluginMacros.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/types/ConfigurationTable.h"

#include "cetlib/compiler_macros.h"
#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/Utilities/exception.h"

#define TRACE_NAME "MessageFacility"

#if GCC_VERSION >= 701000 || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#include "TRACE/trace.h"

#if GCC_VERSION >= 701000 || defined(__clang__)
#pragma GCC diagnostic pop
#endif

namespace mfplugins {
using mf::ELseverityLevel;
using mf::ErrorObj;
using mf::service::ELdestination;

/// <summary>
/// Message Facility destination which logs messages to a TRACE buffer
/// </summary>
class ELTRACE : public ELdestination
{
public:
	/**
	 * \brief Configuration Parameters for ELTRACE
	 */
	struct Config
	{
		/// ELDestination common parameters
		fhicl::TableFragment<ELdestination::Config> elDestConfig;
		/// "lvls" (Default: 0): TRACE level mask for Slow output
		fhicl::Atom<size_t> lvls =
		    fhicl::Atom<size_t>{fhicl::Name{"lvls"}, fhicl::Comment{"TRACE level mask for Slow output"}, 0};
		/// "lvlm" (Default: 0): TRACE level mask for Memory output
		fhicl::Atom<size_t> lvlm =
		    fhicl::Atom<size_t>{fhicl::Name{"lvlm"}, fhicl::Comment{"TRACE level mask for Memory output"}, 0};
	};
	/// Used for ParameterSet validation
	using Parameters = fhicl::WrappedTable<Config>;

public:
	/// <summary>
	/// ELTRACE Constructor
	/// </summary>
	/// <param name="pset">ParameterSet used to configure ELTRACE</param>
	explicit ELTRACE(Parameters const& pset);

	/**
	 * \brief Fill the "Prefix" portion of the message
	 * \param o Output stringstream
	 * \param msg MessageFacility object containing header information
	 */
	void fillPrefix(std::ostringstream& o, const ErrorObj& msg) override;

	/**
	 * \brief Fill the "User Message" portion of the message
	 * \param o Output stringstream
	 * \param msg MessageFacility object containing header information
	 */
	void fillUsrMsg(std::ostringstream& o, const ErrorObj& msg) override;

	/**
	 * \brief Fill the "Suffix" portion of the message (Unused)
	 */
	void fillSuffix(std::ostringstream& /*unused*/, const ErrorObj& /*msg*/) override {}

	/**
	 * \brief Serialize a MessageFacility message to the output
	 * \param o Stringstream object containing message data
	 * \param msg MessageFacility object containing header information
	 */
	void routePayload(const std::ostringstream& o, const ErrorObj& msg) override;
};

// END DECLARATION
//======================================================================
// BEGIN IMPLEMENTATION

//======================================================================
// ELTRACE c'tor
//======================================================================
ELTRACE::ELTRACE(Parameters const& pset)
    : ELdestination(pset().elDestConfig())
{
	size_t msk;

	if (pset().lvls() != 0)
	{
		msk = pset().lvls();
		TRACE_CNTL("lvlmskS", msk);  // the S mask for TRACE_NAME
	}
	if (pset().lvlm() != 0)
	{
		msk = pset().lvlm();
		TRACE_CNTL("lvlmskM", msk);  // the M mask for TRACE_NAME
	}

	TLOG(TLVL_INFO) << "ELTRACE MessageLogger destination plugin initialized.";
}

//======================================================================
// Message prefix filler ( overriddes ELdestination::fillPrefix )
//======================================================================
void ELTRACE::fillPrefix(std::ostringstream& oss, const ErrorObj& msg)
{
	const auto& xid = msg.xid();

	oss << xid.application() << ", ";  // application
	oss << xid.id() << ": ";           // category
	                                   // oss << mf::MessageDrop::instance()->runEvent + ELstring(" "); // run/event no
	                                   // oss << xid.module+ELstring(": ");                            // module name
}

//======================================================================
// Message filler ( overriddes ELdestination::fillUsrMsg )
//======================================================================
void ELTRACE::fillUsrMsg(std::ostringstream& oss, const ErrorObj& msg)
{
	std::ostringstream tmposs;
	ELdestination::fillUsrMsg(tmposs, msg);

	// remove leading "\n" if present
	const std::string& usrMsg = tmposs.str().compare(0, 1, "\n") == 0 ? tmposs.str().erase(0, 1) : tmposs.str();

	oss << usrMsg;
}

//======================================================================
// Message router ( overriddes ELdestination::routePayload )
//======================================================================
void ELTRACE::routePayload(const std::ostringstream& oss, const ErrorObj& msg)
{
	const auto& xid = msg.xid();
	auto message = oss.str();

	auto level = xid.severity().getLevel();
	int lvlNum;

	switch (level)
	{
		case mf::ELseverityLevel::ELsev_success:
		case mf::ELseverityLevel::ELsev_zeroSeverity:
		case mf::ELseverityLevel::ELsev_unspecified:
			lvlNum = 3;
			break;

		case mf::ELseverityLevel::ELsev_info:
			lvlNum = 2;
			break;

		case mf::ELseverityLevel::ELsev_warning:
			lvlNum = 1;
			break;
		default:
			lvlNum = 0;
	}
	TRACE(lvlNum, message);  // NOLINT this is the TRACE -- direct the message to memory and/or stdout
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
	return std::make_unique<mfplugins::ELTRACE>(pset);
}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
