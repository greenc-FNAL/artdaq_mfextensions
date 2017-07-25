#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ostream_handle.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/MessageService/ELostreamOutput.h"
#ifdef NO_MF_UTILITIES
#include "messagefacility/MessageLogger/ELseverityLevel.h"
#else
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/MessageService/ELcontextSupplier.h"
#endif
#include "messagefacility/MessageLogger/MessageDrop.h"
#include "messagefacility/Utilities/exception.h"

// C/C++ includes
#include <iostream>
#include <memory>
#include <algorithm>


namespace mfplugins
{
	using namespace mf::service;
	using mf::ELseverityLevel;
	using mf::ErrorObj;

	//======================================================================
	//
	// Parser-Friendly destination plugin
	//
	//======================================================================

	class ELFriendly : public ELostreamOutput
	{
	public:

		ELFriendly(const fhicl::ParameterSet& pset);

		virtual void fillPrefix(std::ostringstream&, const ErrorObj&, const ELcontextSupplier&) override;
		virtual void fillUsrMsg(std::ostringstream&, const ErrorObj&) override;
		virtual void fillSuffix(std::ostringstream&, const ErrorObj&) override;

	private:
		std::string delimeter_;
	};

	// END DECLARATION
	//======================================================================
	// BEGIN IMPLEMENTATION


	//======================================================================
	// ELFriendly c'tor
	//======================================================================

	ELFriendly::ELFriendly(const fhicl::ParameterSet& pset)
		: ELostreamOutput(pset, cet::ostream_handle{ std::cout }, false)
		, delimeter_(pset.get<std::string>("field_delimeter", "  "))
	{
	}

	//======================================================================
	// Message prefix filler ( overriddes ELdestination::fillPrefix )
	//======================================================================
	void ELFriendly::fillPrefix(std::ostringstream& oss, const ErrorObj& msg, ELcontextSupplier const& contextSupplier)
	{
		//if (msg.is_verbatim()) return;

		// Output the prologue:
		//
		format.preambleMode = true;

		auto const& xid = msg.xid();

		auto id = xid.id;
		auto app = xid.application;
		auto process = xid.process;
		auto module = xid.module;
		auto subroutine = xid.subroutine;
		std::replace(id.begin(), id.end(), ' ', '-');
		std::replace(app.begin(), app.end(), ' ', '-');
		std::replace(process.begin(), process.end(), ' ', '-');
		std::replace(module.begin(), module.end(), ' ', '-');
		std::replace(subroutine.begin(), subroutine.end(), ' ', '-');

		emit(oss, "%MSG");
		emit(oss, xid.severity.getSymbol());
		emit(oss, delimeter_);
		emit(oss, id);
		emit(oss, msg.idOverflow());
		emit(oss, ":");
		emit(oss, delimeter_);

		// Output serial number of message:
		//
		if (format.want(SERIAL)) {
			std::ostringstream s;
			s << msg.serial();
			emit(oss, "[serial #" + s.str() + "]");
			emit(oss, delimeter_);
		}

		// Provide further identification:
		//
		bool needAspace = true;
		if (format.want(EPILOGUE_SEPARATE)) {
			if (module.length() + subroutine.length() > 0) {
				emit(oss, "\n");
				needAspace = false;
			}
			else if (format.want(TIMESTAMP) && !format.want(TIME_SEPARATE)) {
				emit(oss, "\n");
				needAspace = false;
			}
		}
		if (format.want(MODULE) && (module.length() > 0)) {
			if (needAspace) {
				emit(oss, delimeter_);
				needAspace = false;
			}
			emit(oss, module + "  ");
		}
		if (format.want(SUBROUTINE) && (subroutine.length() > 0)) {
			if (needAspace) {
				emit(oss, delimeter_);
				needAspace = false;
			}
			emit(oss, subroutine + "()");
			emit(oss, delimeter_);
		}

		// Provide time stamp:
		//
		if (format.want(TIMESTAMP)) {
			if (format.want(TIME_SEPARATE)) {
				emit(oss, "\n");
				needAspace = false;
			}
			if (needAspace) {
				emit(oss, delimeter_);
				needAspace = false;
			}
			emit(oss, format.timestamp(msg.timestamp()));
			emit(oss, delimeter_);
		}

		// Provide the context information:
		//
		if (format.want(SOME_CONTEXT)) {
			if (needAspace) {
				emit(oss, delimeter_);
				needAspace = false;
			}
			if (format.want(FULL_CONTEXT)) {
				emit(oss, contextSupplier.fullContext());
			}
			else {
				emit(oss, contextSupplier.context());
			}
		}

	}


	//=============================================================================
    void ELFriendly::fillUsrMsg(std::ostringstream& oss, ErrorObj const& msg)
    {
      if (!format.want(TEXT)) return;

      format.preambleMode = false;
      auto const usrMsgStart = std::next(msg.items().cbegin(), 4);
      auto it = msg.items().cbegin();

      // Determine if file and line should be included
      if (true || !msg.is_verbatim()) {

        // The first four items are { " ", "<FILENAME>", ":", "<LINE>" }
        while (it != usrMsgStart) {
          if (!it->compare(" ") && !std::next(it)->compare("--")) {
            // Do not emit if " --:0" is the match
            std::advance(it,4);
          }
          else {
            // Emit if <FILENAME> and <LINE> are meaningful
            emit(oss, *it++);
          }
        }

        // Check for user-requested line breaks
        if (format.want(NO_LINE_BREAKS)) emit(oss, " ==> ");
        else emit(oss, "", true);
      }

      // For verbatim (and user-supplied) messages, just print the contents
      auto const end = msg.items().cend();
      for (; it != end; ++it) {
        emit(oss, *it);
      }

    }


    //=============================================================================
    void ELFriendly::fillSuffix(std::ostringstream& oss, ErrorObj const& msg)
    {
		if ((true || !msg.is_verbatim()) && !format.want(NO_LINE_BREAKS)) {
        emit(oss,"\n%MSG");
      }
      oss << '\n';
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
		return std::make_unique<mfplugins::ELFriendly>(pset);
	}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
