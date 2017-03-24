#include "messagefacility/MessageLogger/MessageLogger.h"
#ifdef NO_MF_UTILITIES
#include <messagefacility/MessageLogger/MessageFacilityMsg.h>
#else
#include <messagefacility/Utilities/MessageFacilityMsg.h>
#endif

#include <boost/program_options.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <string>
#include <mfextensions/Binaries/ReceiverManager.hh>
#include <fhiclcpp/make_ParameterSet.h>

namespace po = boost::program_options;

bool cmdline = false;
int z = 0;

void printmsg(mf::MessageFacilityMsg const& mfmsg)
{
	// No display of received messages in command line mode
	if (cmdline) return;

	// First archive the received message
	// have to do it this way because LogErrorObj() takes a pointer
	// to ErrorObj and take over the ownership (it will deal with
	// with deletion), plus the fact ErrorObj doesn't have a copy
	// assignment operator
	mf::ErrorObj* eop = new mf::ErrorObj(mfmsg.ErrorObject());
	mf::LogErrorObj(eop);

	// Show received message on screen
	std::cout << "severity:       " << mfmsg.severity() << "\n";
	std::cout << "timestamp:      " << mfmsg.timestr() << "\n";
	std::cout << "hostname:       " << mfmsg.hostname() << "\n";
	std::cout << "hostaddr(ip):   " << mfmsg.hostaddr() << "\n";
	std::cout << "process:        " << mfmsg.process() << "\n";
	std::cout << "porcess_id:     " << mfmsg.pid() << "\n";
	std::cout << "application:    " << mfmsg.application() << "\n";
	std::cout << "module:         " << mfmsg.module() << "\n";
	std::cout << "context:        " << mfmsg.context() << "\n";
	std::cout << "category(id):   " << mfmsg.category() << "\n";
	std::cout << "file:           " << mfmsg.file() << "\n";
	std::cout << "line:           " << mfmsg.line() << "\n";
	std::cout << "message:        " << mfmsg.message() << "\n";
	std::cout << std::endl;
}

void printnull(mf::MessageFacilityMsg const& mfmsg)
{
	mf::ErrorObj* eop = new mf::ErrorObj(mfmsg.ErrorObject());
	mf::LogErrorObj(eop);
}

int main(int argc, char* argv[])
{
	// checking options
	std::string filename;
	std::string configFile;
	
	try
	{
		po::options_description cmdopt("Allowed options");
		cmdopt.add_options()
			("help,h", "display help message")
		("config,c", po::value<std::string>(&configFile)->default_value(""),"Specify the FHiCL configuration file to use")
			("filename,f",
			 po::value<std::string>(&filename)->default_value("msg_archive"),
			 "specify the message archive file name");

		po::options_description desc;
		desc.add(cmdopt);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);

		if (vm.count("help"))
		{
			std::cout << "Usage: msglogger [options] <message text>\n";
			std::cout << cmdopt;
			return 0;
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	}
	catch (...)
	{
		std::cerr << "Exception of unknown type!\n";
		return 1;
	}


	// Start MessageFacility Service
	mf::StartMessageFacility(
		mf::MessageFacilityService::SingleThread,
		mf::MessageFacilityService::logArchive(filename));

	fhicl::ParameterSet pset;
	auto maker = cet::filepath_maker();
	fhicl::make_ParameterSet(configFile, maker, pset);
	mfviewer::ReceiverManager rm(pset);
	
	
	// Welcome message
	std::cout << "Message Facility MsgServer is up and listening to configured Receivers" <<std::endl;
	
	// Command line message loop
	std::string cmd;

	while (true)
	{
		if (cmdline) std::cout << "> ";
		getline(std::cin, cmd);

		if (cmd.empty())
		{
			cmdline = true;
		}
		else if (cmdline && (cmd == "r" || cmd == "resume"))
		{
			cmdline = false;;
		}
		else if (cmdline && (cmd == "q" || cmd == "quit"))
		{
			//dds.stop();
			return 0;
		}
		else if (cmdline && (cmd == "h" || cmd == "help"))
		{
			std::cout << "MessageFacility DDS server available commands:\n"
				<< "  (h)elp          display this help message\n"
				<< "  (s)tat          summary of received messages\n"
				<< "  (r)esume        resume to message listening mode\n"
				//<< "  (p)artition     listen to a new partition\n"
				<< "  (q)uit          exit MessageFacility DDS server\n"
				<< "  ... more interactive commands on the way.\n";
		}
		else if (cmdline && (cmd == "s" || cmd == "stat"))
		{
			std::cout << "Currently listening, " << z << " messages have been received." << std::endl;
		}
		else if (cmdline)
		{
			std::cout << "Command " << cmd << " not found. "
				<< "Type \"help\" or \"h\" for a list of available commands.\n";
		}
	} // end of command line message loop


	return 0;
}
