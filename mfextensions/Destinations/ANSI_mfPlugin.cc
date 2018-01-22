#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
# include "messagefacility/MessageLogger/MessageDrop.h"
# include "messagefacility/MessageService/ELcontextSupplier.h"
#endif
#include "messagefacility/Utilities/exception.h"
#include "messagefacility/Utilities/formatTime.h"
#include <iostream>

namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	using mf::service::ELcontextSupplier;
#endif
	using mf::ErrorObj;

	/// <summary>
	/// Message Facility destination which colorizes the console output
	/// </summary>
	class ELANSI : public ELdestination
	{
	public:

		ELANSI(const fhicl::ParameterSet& pset);

#  if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		virtual void routePayload(const std::ostringstream&, const ErrorObj&) override;
#  else
		virtual void routePayload(const std::ostringstream&, const ErrorObj&, const ELcontextSupplier&) override;
#  endif

	private:
		bool bellError_;
		bool blinkError_;
		std::string errorColor_;
		std::string warningColor_;
		std::string infoColor_;
		std::string debugColor_;
	};

	// END DECLARATION
	//======================================================================
	// BEGIN IMPLEMENTATION


	//======================================================================
	// ELANSI c'tor
	//======================================================================

	ELANSI::ELANSI(const fhicl::ParameterSet& pset)
		: ELdestination(pset)
		, bellError_(pset.get<bool>("bell_on_error", true))
		, blinkError_(pset.get<bool>("blink_error_messages", false))
		, errorColor_(pset.get<std::string>("error_ansi_color", "\033[1m\033[91m"))
		, warningColor_(pset.get<std::string>("warning_ansi_color", "\033[1m\033[93m"))
		, infoColor_(pset.get<std::string>("info_ansi_color", "\033[92m"))
		, debugColor_(pset.get<std::string>("debug_ansi_color", "\033[39m"))
	{
		//std::cout << "ANSI Plugin configured with ParameterSet: " << pset.to_string() << std::endl;
	}

	//======================================================================
	// Message router ( overriddes ELdestination::routePayload )
	//======================================================================
	void ELANSI::routePayload(const std::ostringstream& oss, const ErrorObj& msg
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
							  , ELcontextSupplier const&
# endif
	)
	{
		const auto& xid = msg.xid();
# if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		auto level = xid.severity().getLevel();
# else
		auto level = xid.severity.getLevel();
# endif

		switch (level)
		{
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		case mf::ELseverityLevel::ELsev_incidental:
# endif
		case mf::ELseverityLevel::ELsev_success:
		case mf::ELseverityLevel::ELsev_zeroSeverity:
		case mf::ELseverityLevel::ELsev_unspecified:
			std::cout << debugColor_;
			break;

		case mf::ELseverityLevel::ELsev_info:
			std::cout << infoColor_;
			break;

		case mf::ELseverityLevel::ELsev_warning:
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		case mf::ELseverityLevel::ELsev_warning2:
# endif
			std::cout << warningColor_;
			break;

		case mf::ELseverityLevel::ELsev_error:
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		case mf::ELseverityLevel::ELsev_error2:
		case mf::ELseverityLevel::ELsev_next:
		case mf::ELseverityLevel::ELsev_severe2:
		case mf::ELseverityLevel::ELsev_abort:
		case mf::ELseverityLevel::ELsev_fatal:
# endif
		case mf::ELseverityLevel::ELsev_severe:
		case mf::ELseverityLevel::ELsev_highestSeverity:
			if (bellError_) { std::cout << "\007"; }
			if (blinkError_) { std::cout << "\033[5m"; }
			std::cout << errorColor_;
			break;

		default: break;
		}
		std::cout << oss.str();
		std::cout << "\033[0m" << std::endl;
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
		return std::make_unique<mfplugins::ELANSI>(pset);
	}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
