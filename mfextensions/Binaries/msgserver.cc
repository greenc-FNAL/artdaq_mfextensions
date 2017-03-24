#include "messagefacility/MessageLogger/MessageLogger.h"
#ifdef NO_MF_UTILITIES
#include <messagefacility/MessageLogger/MessageFacilityMsg.h>
#else
#include <messagefacility/Utilities/MessageFacilityMsg.h>
#endif
#include "mfextensions/Extensions/MFExtensions.hh"

#include <boost/program_options.hpp>
#include <boost/bind.hpp>

#include <iostream>
#include <string>

namespace po = boost::program_options;

bool cmdline = false;
std::string Partition(mfviewer::NULL_PARTITION);
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

void printsysmsg(mfviewer::SysMsgCode syscode, std::string const& msg)
{
	std::cout << SysMsgCodeToString(syscode) << msg << "\n";
}


int main(int argc, char* argv[])
{
	// checking options
	std::string filename;

	// silent mode
	int silent = 0;

	// non interactive mode
	int noninteractive = 0;

	try
	{
		po::options_description cmdopt("Allowed options");
		cmdopt.add_options()
			("help,h", "display help message")
			("noninteractive,i", "run the msgserver in non-interactive mode")
			//("partition,p", 
			//  po::value<std::string>(&Partition)->default_value(mf::DDSReciever::NULL_PARTITION), 
			//  "Partition number the msgserver will listen to (0 - 4)")
			("silent,s",
			 "run the msgserver in silent mode.")
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
		if (vm.count("silent"))
		{
			silent = 1;
		}
		if (vm.count("noninteractive"))
		{
			noninteractive = 1;
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

	// Start MessageFacility DDS Receiver
	// void (*print)(mf::MessageFacilityMsg const & mfmsg);
	//if(silent) print = printnull;
	//else       print = printmsg;

	//mf::DDSReceiver dds("msgserver", Partition, print, printsysmsg);

	// silent mode
	while (silent) { sleep(10); }

	// Welcome message
	std::cout << "Message Facility MsgServer is up and listening in partition '"
		<< Partition << "'.\n";

	// noninteractive mode
	while (noninteractive) { sleep(10); }

	// Command line message loop
	std::string cmd;

	while (true)
	{
		if (silent)
		{
			getline(std::cin, cmd);
			if (cmd == "q")
			{
				//dds.stop();
				return 0;
			}
			continue;
		}


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
			std::cout << "Currently listening in partition "
				<< "\"" << Partition << "\".\n"
				<< "Total " << z << " messages has been received.\n";
		}
#if 0
        else if( cmdline && (cmd == "p" || cmd == "partition") )
        {
            std::cout << "Please enter a partition number (0-4): ";

            getline(std::cin, cmd);
            std::istringstream ss(cmd);

            if( ss >> PartitionNumber )
            {
		//dds.switchPartition(PartitionNumber);
                cmdline = false;
            }
            else
            {
                std::cout << "Please use an integer value for the partition!\n";
            }
        }
#endif
		else if (cmdline)
		{
			std::cout << "Command " << cmd << " not found. "
				<< "Type \"help\" or \"h\" for a list of available commands.\n";
		}
	} // end of command line message loop


	return 0;
}
