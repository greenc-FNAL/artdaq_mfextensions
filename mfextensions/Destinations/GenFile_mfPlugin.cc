#include "cetlib/PluginTypeDeducer.h"
#include "cetlib/ostream_handle.h"
#include "fhiclcpp/ParameterSet.h"

#include "boost/date_time/posix_time/posix_time.hpp"

#include "messagefacility/MessageService/ELdestination.h"
# include "messagefacility/Utilities/ELseverityLevel.h"
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
# include "messagefacility/MessageService/ELcontextSupplier.h"
# include "messagefacility/MessageLogger/MessageDrop.h"
#endif
#include "messagefacility/Utilities/exception.h"

#include <fstream>

namespace mfplugins
{
	using mf::service::ELdestination;
	using mf::ELseverityLevel;
	using mf::ErrorObj;
#if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	using mf::service::ELcontextSupplier;
#endif

	/// <summary>
	/// Message Facility destination which generates the output file name based on some combination of 
	/// PID, hostname, application name, and/or timestamp
	/// </summary>
	class ELGenFileOutput : public ELdestination
	{
	public:

		ELGenFileOutput(const fhicl::ParameterSet& pset);

		virtual ~ELGenFileOutput() {}

		virtual void routePayload(const std::ostringstream&, const ErrorObj&
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
								  , const ELcontextSupplier&
#endif
		) override;

		virtual void flush(
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
			const ELcontextSupplier&
#endif
		) override;

	private:
		std::string baseDir_;
		bool append_;
		std::unique_ptr<cet::ostream_handle> output_;
	};

	// END DECLARATION
	//======================================================================
	// BEGIN IMPLEMENTATION


	//======================================================================
	// ELGenFileOutput c'tor
	//======================================================================

	ELGenFileOutput::ELGenFileOutput(const fhicl::ParameterSet& pset)
		: ELdestination(pset)
		, baseDir_(pset.get<std::string>("base_directory", "/tmp"))
		, append_(pset.get<bool>("append", true))
	{
		std::string sep = pset.get<std::string>("sep", "_");
		std::string fileName = baseDir_ + "/";
		std::string prefix = pset.get<std::string>("file_name_prefix", "");
		if (prefix.size()) fileName += prefix;

		auto pid = getpid();
		if (pset.get<bool>("use_app_name", true))
		{
			// get process name from '/proc/pid/exe'
			std::string exe;
			std::ostringstream pid_ostr;
			pid_ostr << "/proc/" << pid << "/exe";
			exe = realpath(pid_ostr.str().c_str(), NULL);

			size_t end = exe.find('\0');
			size_t start = exe.find_last_of('/', end);

			fileName += (fileName.size() ? sep : "") + exe.substr(start + 1, end - start - 1);
		}
		if (pset.get<bool>("use_process_id", true)) { fileName += (fileName.size() ? sep : "") + std::to_string(pid); }

		if (pset.get<bool>("use_hostname", true))
		{
			char hostname[256];
			std::string hostString = "";
			if (gethostname(&hostname[0], 256) == 0)
			{
				std::string tmpString(hostname);
				hostString = tmpString;
				size_t pos = hostString.find(".");
				if (pos != std::string::npos && pos > 2)
				{
					hostString = hostString.substr(0, pos);
				}
			}
			if (hostString.size())
			{
				fileName += (fileName.size() ? sep : "") + hostString;
			}
		}
		if (pset.get<bool>("use_timestamp", true)) { fileName += (fileName.size() ? sep : "") + boost::posix_time::to_iso_string(boost::posix_time::second_clock::universal_time()); }

		std::string suffix = pset.get<std::string>("file_name_sufix", "");
		if (suffix.size())
		{
			fileName += (fileName.size() ? sep : "") + suffix;
		}
		fileName += ".log";

		output_ = std::make_unique<cet::ostream_handle>(fileName.c_str(), append_ ? std::ios::app : std::ios::trunc);
	}

	//======================================================================
	// Message router ( overriddes ELdestination::routePayload )
	//======================================================================
	void ELGenFileOutput::routePayload(const std::ostringstream& oss, const ErrorObj& 
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
									   , ELcontextSupplier const& sup
#endif
	)
	{
		*output_ << oss.str();
		flush(
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
			sup
#endif
		);
	}

	void ELGenFileOutput::flush(
# if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		ELcontextSupplier const&
#endif
	)
	{
		output_->flush();
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
		return std::make_unique<mfplugins::ELGenFileOutput>(pset);
	}
}

DEFINE_BASIC_PLUGINTYPE_FUNC(mf::service::ELdestination)
