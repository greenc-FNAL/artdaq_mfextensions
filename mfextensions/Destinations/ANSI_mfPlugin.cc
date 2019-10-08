#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/Utilities/exception.h"
//#include "messagefacility/Utilities/formatTime.h"
#include <iostream>
#include "cetlib/compiler_macros.h"

namespace mfplugins {
using mf::ELseverityLevel;
using mf::ErrorObj;
using mf::service::ELdestination;

/// <summary>
/// Message Facility destination which colorizes the console output
/// </summary>
class ELANSI : public ELdestination
{
public:
	/**
   * \brief Configuration parameters for ELANSI
   */
	struct Config
	{
		/// ELdestination common config parameters
		fhicl::TableFragment<ELdestination::Config> elDestConfig;
		/// "bell_on_error" (Default: true): Whether to ring the system bell on error messages
		fhicl::Atom<bool> bellOnError = fhicl::Atom<bool>{
		    fhicl::Name{"bell_on_error"}, fhicl::Comment{"Whether to ring the system bell on error messages"}, true};
		/// "blink_error_messages" (Default: false): Whether to print error messages with blinking text
		fhicl::Atom<bool> blinkOnError =
		    fhicl::Atom<bool>{fhicl::Name{"blink_error_messages"},
		                      fhicl::Comment{"Whether to print error messages with blinking text"}, false};
		/// "error_ansi_color" (Default: "\033[1m\033[91m"): ANSI Color string for Error Messages
		fhicl::Atom<std::string> errorColor = fhicl::Atom<std::string>{
		    fhicl::Name{"error_ansi_color"}, fhicl::Comment{"ANSI Color string for Error Messages"}, "\033[1m\033[91m"};
		/// "warning_ansi_color" (Default: "\033[1m\033[93m"): ANSI Color string for Warning Messages
		fhicl::Atom<std::string> warningColor = fhicl::Atom<std::string>{
		    fhicl::Name{"warning_ansi_color"}, fhicl::Comment{"ANSI Color string for Warning Messages"}, "\033[1m\033[93m"};
		/// "info_ansi_color" (Default: "\033[92m"): ANSI Color string for Info Messages
		fhicl::Atom<std::string> infoColor = fhicl::Atom<std::string>{
		    fhicl::Name{"info_ansi_color"}, fhicl::Comment{"ANSI Color string for Info Messages"}, "\033[92m"};
		/// "debug_ansi_color" (Default: "\033[39m"): ANSI Color string for ErrDebugor Messages
		fhicl::Atom<std::string> debugColor = fhicl::Atom<std::string>{
		    fhicl::Name{"debug_ansi_color"}, fhicl::Comment{"ANSI Color string for Debug Messages"}, "\033[39m"};
	};
	/// Used for ParameterSet validation
	using Parameters = fhicl::WrappedTable<Config>;

public:
	/// <summary>
	/// ELANSI Constructor
	/// </summary>
	/// <param name="pset">ParameterSet used to configure ELANSI</param>
	ELANSI(Parameters const& pset);

	/**
   * \brief Serialize a MessageFacility message to the output
   * \param o Stringstream object containing message data
   * \param e MessageFacility object containing header information
   */
	virtual void routePayload(const std::ostringstream& o, const ErrorObj& e) override;

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

ELANSI::ELANSI(Parameters const& pset)
    : ELdestination(pset().elDestConfig()), bellError_(pset().bellOnError()), blinkError_(pset().blinkOnError()), errorColor_(pset().errorColor()), warningColor_(pset().warningColor()), infoColor_(pset().infoColor()), debugColor_(pset().debugColor())
{
	// std::cout << "ANSI Plugin configured with ParameterSet: " << pset.to_string() << std::endl;
}

//======================================================================
// Message router ( overriddes ELdestination::routePayload )
//======================================================================
void ELANSI::routePayload(const std::ostringstream& oss, const ErrorObj& msg)
{
	const auto& xid = msg.xid();
	auto level = xid.severity().getLevel();

	switch (level)
	{
		case mf::ELseverityLevel::ELsev_success:
		case mf::ELseverityLevel::ELsev_zeroSeverity:
		case mf::ELseverityLevel::ELsev_unspecified:
			std::cout << debugColor_;
			break;

		case mf::ELseverityLevel::ELsev_info:
			std::cout << infoColor_;
			break;

		case mf::ELseverityLevel::ELsev_warning:
			std::cout << warningColor_;
			break;

		case mf::ELseverityLevel::ELsev_error:
		case mf::ELseverityLevel::ELsev_severe:
		case mf::ELseverityLevel::ELsev_highestSeverity:
			if (bellError_)
			{
				std::cout << "\007";
			}
			if (blinkError_)
			{
				std::cout << "\033[5m";
			}
			std::cout << errorColor_;
			break;

		default:
			break;
	}
	std::cout << oss.str();
	std::cout << "\033[0m" << std::endl;
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
auto makePlugin(const std::string&, const fhicl::ParameterSet& pset)
{
	return std::make_unique<mfplugins::ELANSI>(pset);
}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
