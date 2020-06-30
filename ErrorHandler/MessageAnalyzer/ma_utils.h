#ifndef ERROR_HANDLER_UTILS_H
#define ERROR_HANDLER_UTILS_H

#include "ErrorHandler/MessageAnalyzer/ma_types.h"
#include "messagefacility/Utilities/ELseverityLevel.h"

namespace novadaq {
namespace errorhandler {

  inline string_t 
    trim_hostname(string_t const & host);

  inline node_type_t 
    get_source_from_msg(string_t & src, msg_t const & msg);

  inline string_t
    get_message_type_str(message_type_t type);

  inline sev_code_t get_sev_from_string(string_t const& sev);

} // end of namespace errorhandler
} // end of namespace novadaq

// ------------------------------------------------------------------
// misc. utilities

novadaq::errorhandler::string_t 
  novadaq::errorhandler::trim_hostname(string_t const & host)
{
  size_t pos = host.find('.');
  if (pos==std::string::npos) return host;
  else                        return host.substr(0, pos);
}

/**
 * @brief Determine the source type of the message
 * @param[out] src Resolved Source
 * @param msg Message object
 * @return node_type_t indicating the type of the source
*/
novadaq::errorhandler::node_type_t
novadaq::errorhandler::get_source_from_msg(string_t& src, msg_t const& /*msg*/)
{
	src = "artdaq";
	return Framework;
}


novadaq::errorhandler::string_t 
  novadaq::errorhandler::get_message_type_str(message_type_t type)
{
  switch(type)
  {
  case MSG_SYSTEM:  return "SYSTEM";
  case MSG_ERROR:   return "ERROR";
  case MSG_WARNING: return "WARNING";
  case MSG_INFO:    return "INFO";
  case MSG_DEBUG:   return "DEBUG";
  default:          return "UNKNOWN";
  }
}

sev_code_t novadaq::errorhandler::get_sev_from_string(string_t const& sev) {
	mf::ELseverityLevel elss(sev);

	int sevid = elss.getLevel();

	switch (sevid)
	{
		case mf::ELseverityLevel::ELsev_success:
		case mf::ELseverityLevel::ELsev_zeroSeverity:
		case mf::ELseverityLevel::ELsev_unspecified:
			return SDEBUG;

		case mf::ELseverityLevel::ELsev_info:
			return SINFO;

		case mf::ELseverityLevel::ELsev_warning:
			return SWARNING;

		case mf::ELseverityLevel::ELsev_error:
		case mf::ELseverityLevel::ELsev_severe:
		case mf::ELseverityLevel::ELsev_highestSeverity:
		default:
			return SERROR;
	}
}

#endif









