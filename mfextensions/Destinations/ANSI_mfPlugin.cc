#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#ifdef NO_MF_UTILITIES
#include "messagefacility/MessageLogger/ELseverityLevel.h"
#else
#include "messagefacility/MessageService/ELcontextSupplier.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#endif
#include "messagefacility/MessageLogger/MessageDrop.h"
#include "messagefacility/Utilities/exception.h"
#include "messagefacility/Utilities/formatTime.h"

namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
#ifndef NO_MF_UTILITIES
	using mf::service::ELcontextSupplier;
#endif
	using mf::ErrorObj;

	//======================================================================
	//
	// ANSI destination plugin
	//
	//======================================================================

	class ELANSI : public ELdestination
	{
	public:

		ELANSI(const fhicl::ParameterSet& pset);

		virtual void routePayload(const std::ostringstream&, const ErrorObj&
#ifndef NO_MF_UTILITIES
		                          , const ELcontextSupplier&
#endif
		) override;

	private:
		bool bellError_;
		bool blinkError_;
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
	{
		//std::cout << "ANSI Plugin configured with ParameterSet: " << pset.to_string() << std::endl;
	}

	//======================================================================
	// Message router ( overriddes ELdestination::routePayload )
	//======================================================================
	void ELANSI::routePayload(const std::ostringstream& oss, const ErrorObj& msg
#ifndef NO_MF_UTILITIES
	                          , ELcontextSupplier const&
#endif
	)
	{
		const auto& xid = msg.xid();
		auto level = xid.severity.getLevel();

		switch (level)
		{
		case mf::ELseverityLevel::ELsev_incidental:
		case mf::ELseverityLevel::ELsev_success:
		case mf::ELseverityLevel::ELsev_zeroSeverity:
		case mf::ELseverityLevel::ELsev_unspecified:
			std::cout << "\033[39m";
			break;

		case mf::ELseverityLevel::ELsev_info:
			std::cout << "\033[92m";
			break;

		case mf::ELseverityLevel::ELsev_warning:
		case mf::ELseverityLevel::ELsev_warning2:
			std::cout << "\033[1m\033[93m";
			break;

		case mf::ELseverityLevel::ELsev_error:
		case mf::ELseverityLevel::ELsev_error2:
		case mf::ELseverityLevel::ELsev_next:
		case mf::ELseverityLevel::ELsev_severe:
		case mf::ELseverityLevel::ELsev_severe2:
		case mf::ELseverityLevel::ELsev_abort:
		case mf::ELseverityLevel::ELsev_fatal:
		case mf::ELseverityLevel::ELsev_highestSeverity:
			if (bellError_) { std::cout << "\007"; }
			if (blinkError_) { std::cout << "\033[5m"; }
			std::cout << "\033[1m\033[91m";
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
