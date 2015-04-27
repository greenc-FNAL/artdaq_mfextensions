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

#include <fhiclcpp/fwd.h>
#include <messagefacility/MessageLogger/MessageLogger.h>
#include <messagefacility/MessageLogger/MessageFacilityMsg.h>

#include <boost/regex.hpp>
#include "MVReceiver.h"

namespace mfviewer {

class LogReader : public MVReceiver
{

public:

  LogReader  ();
  virtual ~LogReader( );

  // Receiver Method
  bool init(fhicl::ParameterSet pset);
  void run();
  void stop();

  // access methods
  void   open ( std::string const & filename );
  bool   iseof( ) const;  // reaches the end of the log file
  size_t size ( ) const;

  mf::MessageFacilityMsg read_next( );   // read next log

private:

  std::ifstream log_;
  size_t        size_;

  boost::regex  metadata_1;
  boost::regex  metadata_2;
  boost::smatch what_;

};

} // end of namespace mf

#endif
