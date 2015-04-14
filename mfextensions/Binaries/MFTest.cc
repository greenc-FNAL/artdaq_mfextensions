//#define NDEBUG
#define ML_DEBUG    // always enable debug

#include <iostream>
#include <stdlib.h>
#include <cstdio>

#include "messagefacility/MessageLogger/MessageLogger.h"

void anotherLogger()
{
  // Set module name
  mf::SetModuleName("anotherLogger");

  mf::LogWarning("warn1 | warn2") << "Followed by a WARNING message.";
  mf::LogDebug("debug")           << "The debug message in the other thread";

  return;
}

int main()
{

  try
  {
    // Start MessageFacility Service
    mf::StartMessageFacility(
        mf::MessageFacilityService::MultiThread,
        mf::MessageFacilityService::ConfigurationFile(
            "MessageFacility.cfg",
            mf::MessageFacilityService::logCF("mylog")));
  }
  catch(std::exception & e)
  {
    std::cerr << "Catched\n" << e.what();
    exit(-1);
  }

  // Set module name for the main thread
  mf::SetApplicationName("mftest");
  mf::SetModuleName("MFTest");
  mf::SetContext("pre-event");

  // Start up another logger in a seperate thread
  //boost::thread loggerThread(anotherLogger);

  // Issue messages with different severity levels
  //mf::LogError("err1|err2") << "This is an ERROR message.";
  //mf::LogWarning("warning") << "Followed by a WARNING message.";

  // Switch context
  mf::SetContext("pro-event");

  //mf::SwitchChannel(2);

  char buf[100];

  // Log Debugs
  for(int i=0; i<2; ++i)
  {
    if( i%1000 == 0 ) 
    {
      sprintf(buf, "mftest-%d", i);
      mf::SetApplicationName(buf);
    }

    mf::LogError("catError")     << "Error information. "   << i;
    mf::LogWarning("catWarning") << "Warning information. " << i;
    mf::LogInfo("catInfo")       << "Info information. "    << i;
    LOG_DEBUG("debug")           << "DEBUG information. "   << i;

    //sleep(1);
    usleep(400000);
  }

  // Thread join
  //loggerThread.join();

  mf::LogStatistics();

  //sleep(2);

  return 0;
}
