#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"
#if MESSAGEFACILITY_HEX_VERSION >= 0x20106 // v2_01_06 => cetlib v3_02_00 => new clang support
#include "cetlib/ProvideMakePluginMacros.h"
#endif

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/Utilities/exception.h"
#include "cetlib/compiler_macros.h"

#define TRACE_NAME "MessageFacility"

#if GCC_VERSION >= 701000 || defined(__clang__) 
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#include "trace.h"

#if GCC_VERSION >= 701000 || defined(__clang__) 
#pragma GCC diagnostic pop 
#endif


namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
	using mf::ErrorObj;

	/// <summary>
	/// Message Facility destination which logs messages to a TRACE buffer
	/// </summary>
	class ELTRACE : public ELdestination
	{

#if MESSAGEFACILITY_HEX_VERSION >= 0x20103
		struct Config
		{
			fhicl::TableFragment<ELdestination::Config> elDestConfig;
			fhicl::Atom<size_t> lvls{ fhicl::Name{ "lvls" },fhicl::Comment{ "TRACE level mask for Slow output" },0 };
			fhicl::Atom<size_t> lvlm{ fhicl::Name{ "lvlm" },fhicl::Comment{ "TRACE level mask for Memory output" },0 };
		};
		using Parameters = fhicl::WrappedTable<Config>;
#endif

	public:

		/// <summary>
		/// ELTRACE Constructor
		/// </summary>
		/// <param name="pset">ParameterSet used to configure ELTRACE</param>
#if MESSAGEFACILITY_HEX_VERSION < 0x20103 // v2_01_03 is s58, pre v2_01_03 is s50
		ELTRACE(const fhicl::ParameterSet& pset);
#else
		ELTRACE(Parameters const& pset);
#endif

		/**
		* \brief Fill the "Prefix" portion of the message
		* \param o Output stringstream
		* \param e MessageFacility object containing header information
		*/
		virtual void fillPrefix(std::ostringstream& o, const ErrorObj& e) override;

		/**
		* \brief Fill the "User Message" portion of the message
		* \param o Output stringstream
		* \param e MessageFacility object containing header information
		*/
		virtual void fillUsrMsg(std::ostringstream& o, const ErrorObj& e) override;

		/**
		* \brief Fill the "Suffix" portion of the message (Unused)
		*/
		virtual void fillSuffix(std::ostringstream&, const ErrorObj&) override {}

		/**
		* \brief Serialize a MessageFacility message to the output
		* \param o Stringstream object containing message data
		* \param e MessageFacility object containing header information
		*/
		virtual void routePayload(const std::ostringstream& o, const ErrorObj& e) override;

	};

	// END DECLARATION
	//======================================================================
	// BEGIN IMPLEMENTATION


	//======================================================================
	// ELTRACE c'tor
	//======================================================================
#if MESSAGEFACILITY_HEX_VERSION < 0x20103
	ELTRACE::ELTRACE(const fhicl::ParameterSet& pset)
		: ELdestination(pset)
	{
		size_t msk;

		if (pset.get_if_present<size_t>("lvls", msk))
			TRACE_CNTL("lvlmskS", msk); // the S mask for TRACE_NAME

		if (pset.get_if_present<size_t>("lvlm", msk))
			TRACE_CNTL("lvlmskM", msk); // the M mask for TRACE_NAME

		TLOG(TLVL_INFO) << "ELTRACE MessageLogger destination plugin initialized.";
	}
#else
	ELTRACE::ELTRACE(Parameters const& pset)
		: ELdestination(pset().elDestConfig())
	{
		size_t msk;

		if (pset().lvls() != 0)
		{
			msk = pset().lvls();
			TRACE_CNTL("lvlmskS", msk); // the S mask for TRACE_NAME
		}
		if (pset().lvlm() != 0)
		{
			msk = pset().lvlm();
			TRACE_CNTL("lvlmskM", msk); // the M mask for TRACE_NAME
		}

		TLOG(TLVL_INFO) << "ELTRACE MessageLogger destination plugin initialized.";
	}

#endif

	//======================================================================
	// Message prefix filler ( overriddes ELdestination::fillPrefix )
	//======================================================================
	void ELTRACE::fillPrefix(std::ostringstream& oss, const ErrorObj& msg)
	{
		const auto& xid = msg.xid();

		oss << xid.application() << ", "; // application
		oss << xid.id() << ": "; // category
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
		const std::string& usrMsg = !tmposs.str().compare(0, 1, "\n") ? tmposs.str().erase(0, 1) : tmposs.str();

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
		auto lvlNum = 0;

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
		}
		TRACE(lvlNum, message); // this is the TRACE -- direct the message to memory and/or stdout
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
	return std::make_unique<mfplugins::ELTRACE>(pset);
}
	}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
