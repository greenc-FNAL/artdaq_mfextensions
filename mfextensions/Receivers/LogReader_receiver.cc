#include "mfextensions/Receivers/LogReader_receiver.hh"
#include "mfextensions/Receivers/ReceiverMacros.hh"
#include <iostream>

mfviewer::LogReader::LogReader(fhicl::ParameterSet pset)
  : MVReceiver(pset), log_  ( )
  , size_ ( 0 )
  , filename_(pset.get<std::string>("filename"))
  , counter_(0)
, metadata_1 
  ( "\\%MSG-([wide])\\s([^:]*):\\s\\s([^\\s]*)\\s*(\\d\\d-[^-]*-\\d{4}\\s\\d+:\\d+:\\d+)\\s.[DS]T\\s\\s(\\w+)" )
    //, metadata_2 
    //  ( "([^\\s]*)\\s([^\\s]*)\\s([^\\s]*)\\s(([^\\s]*)\\s)?([^:]*):(\\d*)" )
{
  std::cout << "LogReader_receiver Constructor" << std::endl;
  open(filename_);
}

mfviewer::LogReader::~LogReader()
{
  log_.close();
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

    if(msgFound) {
      std::cout << "Message found, emitting!" << std::endl;
       emit NewMessage(read_next());
       ++counter_;
    }
    
  }
}

void mfviewer::LogReader::open( std::string const & filename )
{
  if( log_.is_open() ) log_.close();
  std::cout << "Opening Log File" << std::endl;
  log_.open( filename.c_str() );
  std::cout << "Log file Open" <<std::endl;
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

    value = std::string(what_[4].first, what_[4].second);
    strptime(value.c_str(), "%d-%b-%Y %H:%M:%S", &tm);

    tm.tm_isdst = -1;
    t = mktime(&tm);
    tv.tv_sec = t;
    tv.tv_usec = 0;

    msg.setCategory ( std::string(what_[2].first, what_[2].second) );
    msg.setTimestamp( tv );
    msg.setApplication(std::string(what_[3].first, what_[3].second) );
    msg.setProcess(std::string(what_[5].first, what_[5].second) );
    //msg.setHostname ( std::string(what_[4].first, what_[4].second) );
    //msg.setHostaddr ( std::string(what_[5].first, what_[5].second) );
  }
  /*
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
  */
  std::string body;
  getline( log_, line );

  while( !log_.eof() && line.find("%MSG")==std::string::npos )
  {
    body += line;
    getline( log_, line );
  }

  msg.setMessage( filename_, std::to_string(counter_), body );

  std::cout <<
    "Time: " << msg.timestr() <<
    ", Severity: " << msg.severity() <<
    ", Category: " << msg.category() <<
    ", Hostname: " << msg.hostname() <<
    ", Hostaddr: " << msg.hostaddr() <<
    ", Process: " << msg.process() <<
    ", Application: " << msg.application() <<
    ", Module: " << msg.module() <<
    ", Context: " << msg.context() <<
    ", File: " << msg.file() <<
    ", Message: " << msg.message() << std::endl;

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

#include "moc_LogReader_receiver.cpp"

DEFINE_MFVIEWER_RECEIVER(mfviewer::LogReader)
