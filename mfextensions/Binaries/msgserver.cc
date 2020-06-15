#include "messagefacility/MessageLogger/MessageLogger.h"
#include "messagefacility/Utilities/ErrorObj.h"

#include <boost/bind.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <string>
#include "fhiclcpp/make_ParameterSet.h"
#include "mfextensions/Binaries/ReceiverManager.hh"

namespace po = boost::program_options;

bool cmdline = false;
int z = 0;

int main(int argc, char* argv[])
{
	// checking options
	std::string filename;
	std::string configFile;

	try
	{
		po::options_description cmdopt("Allowed options");
		cmdopt.add_options()("help,h", "display help message")("config,c",
		                                                       po::value<std::string>(&configFile)->default_value(""),
		                                                       "Specify the FHiCL configuration file to use")(
		    "filename,f", po::value<std::string>(&filename)->default_value("msg_archive"),
		    "specify the message archive file name");

		po::options_description desc;
		desc.add(cmdopt);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
		po::notify(vm);

		if (vm.count("help") != 0u)
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
	std::ostringstream descstr;
	descstr << "";
	fhicl::ParameterSet main_pset;
	mf::StartMessageFacility(main_pset);

	fhicl::ParameterSet pset;
	auto maker = cet::filepath_maker();
	fhicl::make_ParameterSet(configFile, maker, pset);
	mfviewer::ReceiverManager rm(pset);

	// Welcome message
	std::cout << "Message Facility MsgServer is up and listening to configured Receivers" << std::endl;

	// Command line message loop
	std::string cmd;

	while (true)
	{
		if (cmdline)
		{
			std::cout << "> ";
		}
		getline(std::cin, cmd);

		if (cmd.empty())
		{
			cmdline = true;
		}
		else if (cmdline && (cmd == "r" || cmd == "resume"))
		{
			cmdline = false;
			;
		}
		else if (cmdline && (cmd == "q" || cmd == "quit"))
		{
			// dds.stop();
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
	}  // end of command line message loop

	return 0;
}
