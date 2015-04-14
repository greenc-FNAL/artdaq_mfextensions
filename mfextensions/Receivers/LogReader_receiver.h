#ifndef MF_LOG_READER_H
#define MF_LOG_READER_H

// ------------------------------
// MessageFacility Log Reader
//
//   Read messagefacility log archive and reemit as 
//   messagefacility messages
//

#include <string>
#include <fstream>

#include <messagefacility/MessageLogger/MessageLogger.h>
#include <messagefacility/MessageLogger/MessageFacilityMsg.h>

#include <boost/regex.hpp>

namespace mfviewer {

  class LogReader : public MVReceiver
{

public:

  LogReader  ( );
  ~LogReader ( );

  // access methods
  void   open ( std::string const & filename );
  bool   iseof( ) const;  // reaches the end of the log file
  size_t size ( ) const;

  MessageFacilityMsg read_next( );   // read next log

private:

  std::ifstream log_;
  size_t        size_;

  boost::regex  metadata_1;
  boost::regex  metadata_2;
  boost::smatch what_;

};

} // end of namespace mf

#endif
