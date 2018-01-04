#include "cetlib/PluginTypeDeducer.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
# include "messagefacility/MessageService/ELcontextSupplier.h"
# include "messagefacility/MessageLogger/MessageDrop.h"
#endif
#include "messagefacility/Utilities/exception.h"

#define TRACE_NAME "MessageFacility"
#include "trace.h"


namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	using mf::service::ELcontextSupplier;
# endif
	using mf::ErrorObj;

	/// <summary>
	/// Message Facility destination which logs messages to a TRACE buffer
	/// </summary>
	class ELTRACE : public ELdestination
	{
	public:

		ELTRACE(const fhicl::ParameterSet& pset);

		virtual void fillPrefix(std::ostringstream&, const ErrorObj&
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
								, const ELcontextSupplier&
# endif
		) override;

		virtual void fillUsrMsg(std::ostringstream&, const ErrorObj&) override;

		virtual void fillSuffix(std::ostringstream&, const ErrorObj&) override {}

		virtual void routePayload(const std::ostringstream&, const ErrorObj&
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
								  , const ELcontextSupplier&
# endif
		) override;

	private:
		int consecutive_success_count_;
		int error_count_;
		int next_error_report_;
		int error_report_backoff_factor_;
	};

	// END DECLARATION
	//======================================================================
	// BEGIN IMPLEMENTATION


	//======================================================================
	// ELTRACE c'tor
	//======================================================================

	ELTRACE::ELTRACE(const fhicl::ParameterSet& pset)
		: ELdestination(pset)
		, consecutive_success_count_(0)
		, error_count_(0)
		, next_error_report_(1)
		, error_report_backoff_factor_()
	{
		error_report_backoff_factor_ = pset.get<int>("error_report_backoff_factor", 10);
		TRACE(3, "ELTRACE MessageLogger destination plugin initialized.");
	}

	//======================================================================
	// Message prefix filler ( overriddes ELdestination::fillPrefix )
	//======================================================================
	void ELTRACE::fillPrefix(std::ostringstream& oss, const ErrorObj& msg
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
							 , ELcontextSupplier const&
# endif
	)
	{
		const auto& xid = msg.xid();

# if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		oss << xid.application() << ", "; // application
		oss << xid.id() << ": "; // category
# else
		oss << xid.application << ", "; // application
		oss << xid.id << ": "; // category
# endif
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
	void ELTRACE::routePayload(const std::ostringstream& oss, const ErrorObj& msg
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
							   , ELcontextSupplier const&
# endif
	)
	{
		const auto& xid = msg.xid();
		auto message = oss.str();

# if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		auto level = xid.severity().getLevel();
# else
		auto level = xid.severity.getLevel();
# endif
		auto lvlNum = 0;

		switch (level)
		{
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		case mf::ELseverityLevel::ELsev_incidental:
# endif
		case mf::ELseverityLevel::ELsev_success:
		case mf::ELseverityLevel::ELsev_zeroSeverity:
		case mf::ELseverityLevel::ELsev_unspecified:
			lvlNum = 3;
			break;

		case mf::ELseverityLevel::ELsev_info:
			lvlNum = 2;
			break;

		case mf::ELseverityLevel::ELsev_warning:
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		case mf::ELseverityLevel::ELsev_warning2:
# endif
			lvlNum = 1;
			break;
		}
		TRACE(lvlNum, message);
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
			return std::make_unique<mfplugins::ELTRACE>(pset);
		}
	}

	DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
