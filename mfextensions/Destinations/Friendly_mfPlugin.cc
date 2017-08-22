#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ostream_handle.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/MessageService/ELostreamOutput.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
# include "messagefacility/MessageService/ELcontextSupplier.h"
# include "messagefacility/MessageLogger/MessageDrop.h"
#endif
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

#      if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		virtual void fillPrefix(std::ostringstream&, const ErrorObj&) override;
#      else
		virtual void fillPrefix(std::ostringstream&, const ErrorObj&, const ELcontextSupplier&) override;
#      endif
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
#  if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
	void ELFriendly::fillPrefix(std::ostringstream& oss, const ErrorObj& msg)
#  else
	void ELFriendly::fillPrefix(std::ostringstream& oss, const ErrorObj& msg, ELcontextSupplier const& contextSupplier)
#  endif
	{
		//if (msg.is_verbatim()) return;

		// Output the prologue:
		//
		format.preambleMode = true;

		auto const& xid = msg.xid();

#      if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		auto id = xid.id();
		auto app = xid.application();
		auto module = xid.module();
		auto subroutine = xid.subroutine();
#      else
		auto id = xid.id;
		auto app = xid.application;
		auto process = xid.process;
		auto module = xid.module;
		auto subroutine = xid.subroutine;
		std::replace(process.begin(), process.end(), ' ', '-');
#      define emitToken emit
#      endif
		std::replace(id.begin(), id.end(), ' ', '-');
		std::replace(app.begin(), app.end(), ' ', '-');
		std::replace(module.begin(), module.end(), ' ', '-');
		std::replace(subroutine.begin(), subroutine.end(), ' ', '-');

		emitToken(oss, "%MSG");
#      if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
		emitToken(oss, xid.severity().getSymbol());
#      else
		emit(oss, xid.severity.getSymbol());
#      endif
		emitToken(oss, delimeter_);
		emitToken(oss, id);
		emitToken(oss, msg.idOverflow());
		emitToken(oss, ":");
		emitToken(oss, delimeter_);

		// Output serial number of message:
		//
		if (format.want(SERIAL)) {
			std::ostringstream s;
			s << msg.serial();
			emitToken(oss, "[serial #" + s.str() + "]");
			emitToken(oss, delimeter_);
		}

		// Provide further identification:
		//
		bool needAspace = true;
		if (format.want(EPILOGUE_SEPARATE)) {
			if (module.length() + subroutine.length() > 0) {
				emitToken(oss, "\n");
				needAspace = false;
			}
			else if (format.want(TIMESTAMP) && !format.want(TIME_SEPARATE)) {
				emitToken(oss, "\n");
				needAspace = false;
			}
		}
		if (format.want(MODULE) && (module.length() > 0)) {
			if (needAspace) {
				emitToken(oss, delimeter_);
				needAspace = false;
			}
			emitToken(oss, module + "  ");
		}
		if (format.want(SUBROUTINE) && (subroutine.length() > 0)) {
			if (needAspace) {
				emitToken(oss, delimeter_);
				needAspace = false;
			}
			emitToken(oss, subroutine + "()");
			emitToken(oss, delimeter_);
		}

		// Provide time stamp:
		//
		if (format.want(TIMESTAMP)) {
			if (format.want(TIME_SEPARATE)) {
				emitToken(oss, "\n");
				needAspace = false;
			}
			if (needAspace) {
				emitToken(oss, delimeter_);
				needAspace = false;
			}
			emitToken(oss, format.timestamp(msg.timestamp()));
			emitToken(oss, delimeter_);
		}

		// Provide the context information:
		//
		if (format.want(SOME_CONTEXT)) {
			if (needAspace) {
				emitToken(oss, delimeter_);
				needAspace = false;
			}
#          if MESSAGEFACILITY_HEX_VERSION >= 0x20002 // an indication of a switch from s48 to s50
			emitToken(oss, msg.context());
#          else
			if (format.want(FULL_CONTEXT)) {
				emit(oss, contextSupplier.fullContext());
			}
			else {
				emit(oss, contextSupplier.context());
			}
#          endif
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
            emitToken(oss, *it++);
          }
        }

        // Check for user-requested line breaks
        if (format.want(NO_LINE_BREAKS)) emitToken(oss, " ==> ");
        else emitToken(oss, "", true);
      }

      // For verbatim (and user-supplied) messages, just print the contents
      auto const end = msg.items().cend();
      for (; it != end; ++it) {
        emitToken(oss, *it);
      }

    }


    //=============================================================================
    void ELFriendly::fillSuffix(std::ostringstream& oss, ErrorObj const& msg)
    {
		if ((true || !msg.is_verbatim()) && !format.want(NO_LINE_BREAKS)) {
        emitToken(oss,"\n%MSG");
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
