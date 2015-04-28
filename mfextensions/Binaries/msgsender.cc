//
// msgLogger.cc
// ------------------------------------------------------------------
// Command line appication to send a message through MessageFacility.
// 
// ql   03/01/2010
//

#include "messagefacility/MessageLogger/MessageLogger.h"
#include "mfextensions/Extensions/MFExtensions.hh"

#include "fhiclcpp/ParameterSet.h"

#include <boost/program_options.hpp>

using namespace boost;
namespace po = boost::program_options;

#include <iostream>
#include <algorithm>
#include <iterator>

std::string trim_copy(std::string const src)
{
  std::string::size_type len = src.length();
  std::string::size_type i    = 0;
  std::string::size_type j    = len-1;

  while( (i < len) && (src[i] == ' ') ) ++i;
  while( (j > 0  ) && (src[j] == ' ') ) --j;

  return src.substr(i,j-i+1);
}

void parseDestinations (std::string const & s, std::vector<std::string> & dests)
{
  dests.clear();

  const std::string::size_type npos = s.length();
        std::string::size_type i    = 0;
  while ( i < npos ) {    
    std::string::size_type j = s.find('|',i); 
    std::string dest = trim_copy(s.substr(i,j-i));  
    dests.push_back (dest);
    i = j;
    while ( (i < npos) && (s[i] == '|') ) ++i; 
    // the above handles cases of || and also | at end of string
  } 
}

int main(int ac, char* av[])
{
  std::string         severity;
  std::string         application;
  std::string         message;
  std::string         cat;
  std::string         dest;
  std::string         filename;

  std::string         partition(mfviewer::NULL_PARTITION);

  std::vector<std::string> messages;
  std::vector<std::string> vcat;
  std::vector<std::string> vdest;

  std::vector<std::string> vcat_def;
  std::vector<std::string> vdest_def;

  vcat_def.push_back("");
  vdest_def.push_back("stdout");

  try {
    po::options_description cmdopt("Allowed options");
    cmdopt.add_options()
      ("help,h", "display help message")
      ("severity,s", 
        po::value<std::string>(&severity)->default_value("info"), 
        "severity of the message (error, warning, info, debug)")
      ("category,c", 
        po::value< std::vector<std::string> >(&vcat)->default_value(vcat_def, "null"),
        "message id / categories")
      ("application,a", 
        po::value<std::string>(&application)->default_value(""), 
        "issuing application name")
      ("destination,d", 
        po::value< std::vector<std::string> >(&vdest)->default_value(vdest_def, "stdout"),
        "logging destination(s) of the message (stdout, file, server)")
      //("partition,p",
      //  po::value<string>(&partition)->default_value(mf::MF_DDS_Types::NULL_PARTITION),
      //  "partition of the message, applicable only when a sever destination is specified (0 - 4)")
      ("filename,f",
       po::value<std::string>(&filename)->default_value("logfile"),
        "specify the log file name");

    po::options_description hidden("Hidden options");
    hidden.add_options()
      ("message", po::value< std::vector<std::string> >(&messages), "message text");

    po::options_description desc;
    desc.add(cmdopt).add(hidden);

    po::positional_options_description p;
    p.add("message", -1);
        
    po::variables_map vm;
    po::store(po::command_line_parser(ac, av).options(desc).positional(p).run(), vm);
    po::notify(vm);

    if(vm.count("help"))
    {
      std::cout << "Usage: msglogger [options] <message text>\n";
      std::cout << cmdopt;
      return 0;
    }
  } 
  catch(std::exception & e)
  {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
  catch(...) {
    std::cerr << "Exception of unknown type!\n";
    return 1;
  }
    
  std::vector<std::string>::iterator it;

  // must have message text
  if(messages.size()==0)
  {
    std::cout << "Message text is missing!\n";
    std::cout << "Use \"msglogger --help\" for help messages\n";
    return 1;
  }

  if(application.empty())
  {
    std::cout << "Application name is missing!\n";
    std::cout << "Message cannot be issued without specifying the application name.\n";
    return 1;
  }

  // build message text string
  it = messages.begin();
  while(it!=messages.end())
  {
    message += *it + " ";
    ++it;
  }

  // checking severity...
  transform(severity.begin(), severity.end(), severity.begin(), ::toupper);
  if( (severity!="ERROR") && (severity!="WARNING")
      && (severity!="INFO") && (severity!="DEBUG") )
  {
    std::cerr << "Unknown severity level!\n";
    return 1;
  }

  // checking categories..
  it = vcat.begin();
  while(it!=vcat.end())
  {
    cat += *it + ((it==vcat.end()-1) ? "" : "|");
    ++it;
  }

  // checking destiantions...
  it = vdest.begin();
  while(it!=vdest.end())
  {
    transform((*it).begin(), (*it).end(), (*it).begin(), ::toupper);
    dest += *it + "|";
    ++it;
  }

  // parsing destinations...
  parseDestinations(dest, vdest);
  int flag = 0x00;
  it = vdest.begin();
  while(it!=vdest.end())
  {
    if( (*it) == "STDOUT" )      flag = flag | 0x01;
    else if( (*it) == "FILE" )   flag = flag | 0x02;
    else if( (*it) == "SERVER")  flag = flag | 0x04;
    ++it;
  }

  // preparing parameterset for detinations...
  fhicl::ParameterSet pset;
  switch(flag)
  {
    case 0x01:
      pset = mf::MessageFacilityService::logConsole();
      break;
    case 0x02:
      pset = mf::MessageFacilityService::logFile(filename, true);
      break;
    case 0x03:
      pset = mf::MessageFacilityService::logCF(filename, true);
      break;
    case 0x04:
      pset = mf::MessageFacilityService::logServer();
      break;
    case 0x05:
      pset = mf::MessageFacilityService::logCS();
      break;
    case 0x06:
      pset = mf::MessageFacilityService::logFS(filename, true);
      break;
    case 0x07:
      pset = mf::MessageFacilityService::logCFS(filename, true);
      break;
    default:
      pset = mf::MessageFacilityService::logConsole();
  }

  // start up message facility service
  mf::StartMessageFacility( mf::MessageFacilityService::MultiThread, pset );
  mf::SetApplicationName(application);
  
  // logging message...
  if( severity == "ERROR" )
    mf::LogError  (cat) << message;
  else if(severity == "WARNING" )
    mf::LogWarning(cat) << message;
  else if(severity == "INFO"    )
    mf::LogInfo   (cat) << message;
  else if(severity == "DEBUG"   )
    mf::LogDebug  (cat) << message;
  

  return 0;
}
