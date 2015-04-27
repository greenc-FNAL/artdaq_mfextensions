#include "LogReader_receiver.h"

mfviewer::LogReader::LogReader()
: log_  ( )
, size_ ( 0 )
, metadata_1 
  ( "\\%MSG-([wide])\\s([^:]*):\\s\\s(.{24})\\s*([^\\s]*)\\s*\\(([\\d\\.]*)\\)" )
, metadata_2 
  ( "([^\\s]*)\\s([^\\s]*)\\s([^\\s]*)\\s(([^\\s]*)\\s)?([^:]*):(\\d*)" )
{

}

mfviewer::LogReader::~LogReader()
{
  log_.close();
}

bool mfviewer::LogReader::init(fhicl::ParameterSet pset)
{
  try{
    open(pset.get<std::string>("filename"));
  }
  catch(...) {
    mf::LogError("LogReader") << "Unable to open log file or log file name not specified in parameter set: \"" << pset.to_string() << "\".";
    return false;
  }
  setPartition(pset.get<std::string>("Partition","0"));

  return true;
}


void mfviewer::LogReader::run()
{
  while(!stopRequested_) {
    if(iseof()) {
      usleep(5000);
      continue;
    }
    
    bool msgFound = false;
    while(!msgFound)
    {
      if(iseof()) {
        usleep(5000);
        break;
      }

      std::string line;
      size_t pos = log_.tellg();
      getline(log_, line);
      if(line.find("%MSG") != std::string::npos)
	{
	  msgFound = true;
          log_.seekg(pos);
	}
    }

    if(msgFound)
      emit NewMessage(read_next());
    
  }
}

void mfviewer::LogReader::open( std::string const & filename )
{
  if( log_.is_open() ) log_.close();

  log_.open( filename.c_str() );
  size_ = 0;
}

#include <iostream>
#include <time.h>

mf::MessageFacilityMsg mfviewer::LogReader::read_next( )
{
  mf::MessageFacilityMsg msg;
  std::string line;

  // meta data 1
  getline( log_, line );

  if( boost::regex_search( line, what_, metadata_1 ) )
  {
#if 0
    std::cout << ">> " << std::string(what_[1].first, what_[1].second) << "\n";
    std::cout << ">> " << std::string(what_[2].first, what_[2].second) << "\n";
    std::cout << ">> " << std::string(what_[3].first, what_[3].second) << "\n";
    std::cout << ">> " << std::string(what_[4].first, what_[4].second) << "\n";
    std::cout << ">> " << std::string(what_[5].first, what_[5].second) << "\n";
#endif

    std::string value = std::string(what_[1].first, what_[1].second);

    switch(value[0])
    {
    case 'e': msg.setSeverity("ERROR");   break;
    case 'w': msg.setSeverity("WARNING"); break;
    case 'i': msg.setSeverity("INFO");    break;
    case 'd': msg.setSeverity("DEBUG");   break;
    default : break;
    }

    struct tm tm;
    time_t t;
    timeval tv;

    value = std::string(what_[3].first, what_[3].second);
    strptime(value.c_str(), "%d-%b-%Y %H:%M:%S", &tm);

    tm.tm_isdst = -1;
    t = mktime(&tm);
    tv.tv_sec = t;
    tv.tv_usec = 0;

    msg.setCategory ( std::string(what_[2].first, what_[2].second) );
    msg.setTimestamp( tv );
    msg.setHostname ( std::string(what_[4].first, what_[4].second) );
    msg.setHostaddr ( std::string(what_[5].first, what_[5].second) );
  }

  // meta data 2
  getline( log_, line );

  std::string file;
  std::string no;

  if( boost::regex_search( line, what_, metadata_2 ) )
  {
#if 0
    std::cout << ">> " << std::string(what_[1].first, what_[1].second) << "\n";
    std::cout << ">> " << std::string(what_[2].first, what_[2].second) << "\n";
    std::cout << ">> " << std::string(what_[3].first, what_[3].second) << "\n";
    std::cout << ">> " << std::string(what_[5].first, what_[5].second) << "\n";
    std::cout << ">> " << std::string(what_[6].first, what_[6].second) << "\n";
    std::cout << ">> " << std::string(what_[7].first, what_[7].second) << "\n";
#endif

    msg.setProcess    ( std::string(what_[1].first, what_[1].second) );
    msg.setApplication( std::string(what_[2].first, what_[2].second) );
    msg.setModule     ( std::string(what_[3].first, what_[3].second) );
    msg.setContext    ( std::string(what_[5].first, what_[5].second) );

    file = std::string(what_[6].first, what_[6].second);  
    no   = std::string(what_[7].first, what_[7].second);  
  }

  std::string body;
  getline( log_, line );

  while( !log_.eof() && line.find("%MSG")==std::string::npos )
  {
    body += line;
    getline( log_, line );
  }

  msg.setMessage( file, no, body );

  return msg;
}

bool mfviewer::LogReader::iseof( ) const
{
  if( !log_.is_open() ) return true;
  return log_.eof();
}

size_t mfviewer::LogReader::size() const
{
  return size_;
}

