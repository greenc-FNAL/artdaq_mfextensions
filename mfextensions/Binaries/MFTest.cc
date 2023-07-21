/**
 * @file MFTest.cc
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
// #define NDEBUG

#define MF_DEBUG

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/MessageLogger/MessageLogger.h"

void anotherLogger()
{
	// Set module name
	mf::SetApplicationName("anotherLogger");

	mf::LogWarning("warn1 | warn2") << "Followed by a WARNING message.";
	mf::LogDebug("debug") << "The debug message in the other thread";
}

int main()
{
	try
	{
		// Start MessageFacility Service
		std::ostringstream ss;
		std::ifstream logfhicl("MessageFacility.cfg");
		if (logfhicl.is_open())
		{
			std::stringstream fhiclstream;
			fhiclstream << logfhicl.rdbuf();
			ss << fhiclstream.str();
		}

		std::string pstr(ss.str());
		fhicl::ParameterSet pset = fhicl::ParameterSet::make(pstr);
		mf::StartMessageFacility(pset);
	}
	catch (std::exception& e)
	{
		std::cerr << "Catched\n"
		          << e.what();
		exit(-1);
	}

	// Set module name for the main thread
	mf::SetApplicationName("mftest");

	// Start up another logger in a seperate thread
	// boost::thread loggerThread(anotherLogger);

	// Issue messages with different severity levels
	// mf::LogError("err1|err2") << "This is an ERROR message.";
	// mf::LogWarning("warning") << "Followed by a WARNING message.";

	// Switch context

	// mf::SwitchChannel(2);

	char buf[100];

	// Log Debugs
	for (int i = 0; i < 2; ++i)
	{
		if (i % 1000 == 0)
		{
			snprintf(buf, sizeof(buf), "mftest-%d", i);
			mf::SetApplicationName(buf);
		}

		mf::LogError("catError") << "Error information. " << i;
		mf::LogWarning("catWarning") << "Warning information. " << i;
		mf::LogInfo("catInfo") << "Info information. " << i;

		MF_LOG_DEBUG("debug") << "DEBUG information. " << i;

		// sleep(1);
		usleep(400000);
	}

	// Thread join
	// loggerThread.join();

	mf::LogStatistics();

	// sleep(2);

	return 0;
}
