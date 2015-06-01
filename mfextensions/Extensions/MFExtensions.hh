#ifndef MF_EXTENSIONS_H
#define MF_EXTENSIONS_H

#include <string>

namespace mfviewer {
  enum class SysMsgCode {
     NEW_MESSAGE, 
       SWITCH_PARTITION,
       UNKNOWN,
  };

  const std::string NULL_PARTITION = "NULL_PARTITION";

  /*
  enum class SeverityCode {
    DEBUG,
    INFO,
    WARNING,
      ERROR,
      UNKNOWN,
  };

  inline SeverityCode GetSeverityCode(std::string input) {
    if(input == "DEBUG") { return SeverityCode::DEBUG; }
    else if(input == "INFO") { return SeverityCode::INFO; }
    else if(input == "WARNING") { return SeverityCode::WARNING; }
    else if(input == "ERROR") { return SeverityCode::ERROR; }
 
    return SeverityCode::UNKNOWN;
  }

  inline std::string SeverityToString(SeverityCode input) {
    switch(input) {
    case SeverityCode::DEBUG:
      return "DEBUG";
    case SeverityCode::INFO:
      return "INFO";
    case SeverityCode::WARNING:
      return "WARNING";
    case SeverityCode::ERROR:
      return "ERROR";
    default:
      return "UNKNOWN";
    }
  }

  inline SeverityCode GetSeverityCode(int input) {
  switch( input )
  {
  case 0: return SeverityCode::DEBUG;
  case 1: return SeverityCode::INFO;
  case 2: return SeverityCode::WARNING;
  case 3: return SeverityCode::ERROR;
  default: return SeverityCode::UNKNOWN;
  }
  }
*/
  inline SysMsgCode StringToSysMsgCode(std::string input) {
    if(input == "NEW_MESSAGE") { return SysMsgCode::NEW_MESSAGE; }
    else if(input == "SWITCH_PARTITION") { return SysMsgCode::SWITCH_PARTITION; }
   
    return SysMsgCode::UNKNOWN;
  }

  inline std::string SysMsgCodeToString(SysMsgCode input) {
    switch(input) {
    case SysMsgCode::NEW_MESSAGE:
      return "NEW_MESSAGE";
    case SysMsgCode::SWITCH_PARTITION:
      return "SWITCH_PARTITION";
    default:
      return "UNKNOWN";
    }
  }
}

#endif
