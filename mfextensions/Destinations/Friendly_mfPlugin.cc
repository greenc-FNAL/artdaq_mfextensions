#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ostream_handle.h"
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "messagefacility/MessageService/ELostreamOutput.h"
#include "messagefacility/Utilities/ELseverityLevel.h"
#include "messagefacility/Utilities/exception.h"
#include "cetlib/compiler_macros.h"

// C/C++ includes
#include <iostream>
#include <memory>
#include <algorithm>

#if MESSAGEFACILITY_HEX_VERSION < 0x20201 // format changed to format_ for s67
#define format_ format
#endif

namespace mfplugins
{
	using namespace mf::service;
	using mf::ELseverityLevel;
	using mf::ErrorObj;

	/// <summary>
	/// Parser-Friendly Message Facility destination plugin
	/// </summary>
	class ELFriendly : public ELostreamOutput
	{
#if MESSAGEFACILITY_HEX_VERSION >= 0x20103
		struct Config
		{
			fhicl::TableFragment<ELostreamOutput::Config> elOstrConfig;
			fhicl::Atom<std::string> delimiter{ fhicl::Name{ "field_delimiter" },fhicl::Comment{ "String to print between each message field" },"  " };
		};
		using Parameters = fhicl::WrappedTable<Config>;
#endif
	public:
#if MESSAGEFACILITY_HEX_VERSION < 0x20103 // v2_01_03 is s58, pre v2_01_03 is s50
		ELFriendly(const fhicl::ParameterSet& pset);
#else
		ELFriendly(Parameters const& pset);
#endif

		virtual void fillPrefix(std::ostringstream&, const ErrorObj&) override;
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

#if MESSAGEFACILITY_HEX_VERSION < 0x20103 // v2_01_03 is s58, pre v2_01_03 is s50
	ELFriendly::ELFriendly(const fhicl::ParameterSet& pset)
		: ELostreamOutput(pset, cet::ostream_handle{ std::cout }, false)
		, delimeter_(pset.get<std::string>("field_delimeter", "  "))
#else
	ELFriendly::ELFriendly(Parameters const& pset)
		: ELostreamOutput(pset().elOstrConfig(), cet::ostream_handle{ std::cout }, false)
		, delimeter_(pset().delimiter())
#endif
	{}

	//======================================================================
	// Message prefix filler ( overriddes ELdestination::fillPrefix )
	//======================================================================
	void ELFriendly::fillPrefix(std::ostringstream& oss, const ErrorObj& msg)
	{
		//if (msg.is_verbatim()) return;

		// Output the prologue:
		//
#if MESSAGEFACILITY_HEX_VERSION < 0x20201 // format changed to format_ for s67
		format_.preambleMode = true;
#endif

		auto const& xid = msg.xid();

		auto id = xid.id();
		auto app = xid.application();
		auto module = xid.module();
		auto subroutine = xid.subroutine();
		std::replace(id.begin(), id.end(), ' ', '-');
		std::replace(app.begin(), app.end(), ' ', '-');
		std::replace(module.begin(), module.end(), ' ', '-');
		std::replace(subroutine.begin(), subroutine.end(), ' ', '-');

		emitToken(oss, "%MSG");
		emitToken(oss, xid.severity().getSymbol());
		emitToken(oss, delimeter_);
		emitToken(oss, id);
		emitToken(oss, msg.idOverflow());
		emitToken(oss, ":");
		emitToken(oss, delimeter_);

		// Output serial number of message:
		//
		if (format_.want(SERIAL))
		{
			std::ostringstream s;
			s << msg.serial();
			emitToken(oss, "[serial #" + s.str() + "]");
			emitToken(oss, delimeter_);
		}

		// Provide further identification:
		//
		bool needAspace = true;
		if (format_.want(EPILOGUE_SEPARATE))
		{
			if (module.length() + subroutine.length() > 0)
			{
				emitToken(oss, "\n");
				needAspace = false;
			}
			else if (format_.want(TIMESTAMP) && !format_.want(TIME_SEPARATE))
			{
				emitToken(oss, "\n");
				needAspace = false;
			}
		}
		if (format_.want(MODULE) && (module.length() > 0))
		{
			if (needAspace)
			{
				emitToken(oss, delimeter_);
				needAspace = false;
			}
			emitToken(oss, module + "  ");
		}
		if (format_.want(SUBROUTINE) && (subroutine.length() > 0))
		{
			if (needAspace)
			{
				emitToken(oss, delimeter_);
				needAspace = false;
			}
			emitToken(oss, subroutine + "()");
			emitToken(oss, delimeter_);
		}

		// Provide time stamp:
		//
		if (format_.want(TIMESTAMP))
		{
			if (format_.want(TIME_SEPARATE))
			{
				emitToken(oss, "\n");
				needAspace = false;
			}
			if (needAspace)
			{
				emitToken(oss, delimeter_);
				needAspace = false;
			}
			emitToken(oss, format_.timestamp(msg.timestamp()));
			emitToken(oss, delimeter_);
		}

		// Provide the context information:
		//
		if (format_.want(SOME_CONTEXT))
		{
			if (needAspace)
			{
				emitToken(oss, delimeter_);
				needAspace = false;
			}
			emitToken(oss, msg.context());
		}

	}


	//=============================================================================
	void ELFriendly::fillUsrMsg(std::ostringstream& oss, ErrorObj const& msg)
	{
		if (!format_.want(TEXT)) return;

#if MESSAGEFACILITY_HEX_VERSION < 0x20201 // format changed to format_ for s67
		format_.preambleMode = false;
#endif
		auto const usrMsgStart = std::next(msg.items().cbegin(), 4);
		auto it = msg.items().cbegin();

		// Determine if file and line should be included
		if (true || !msg.is_verbatim())
		{

			// The first four items are { " ", "<FILENAME>", ":", "<LINE>" }
			while (it != usrMsgStart)
			{
				if (!it->compare(" ") && !std::next(it)->compare("--"))
				{
					// Do not emit if " --:0" is the match
					std::advance(it, 4);
				}
				else
				{
					// Emit if <FILENAME> and <LINE> are meaningful
					emitToken(oss, *it++);
				}
			}

			// Check for user-requested line breaks
			if (format_.want(NO_LINE_BREAKS)) emitToken(oss, " ==> ");
			else emitToken(oss, "", true);
		}

		// For verbatim (and user-supplied) messages, just print the contents
		auto const end = msg.items().cend();
		for (; it != end; ++it)
		{
			emitToken(oss, *it);
		}

	}


	//=============================================================================
	void ELFriendly::fillSuffix(std::ostringstream& oss, ErrorObj const& msg)
	{
		if ((true || !msg.is_verbatim()) && !format_.want(NO_LINE_BREAKS))
		{
			emitToken(oss, "\n%MSG");
		}
		oss << '\n';
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
	return std::make_unique<mfplugins::ELFriendly>(pset);
}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
