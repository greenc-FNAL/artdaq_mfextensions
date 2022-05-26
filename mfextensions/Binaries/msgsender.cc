//
// msgLogger.cc
// ------------------------------------------------------------------
// Command line appication to send a message through MessageFacility.
//
// ql   03/01/2010
//

#include "messagefacility/MessageLogger/MessageLogger.h"

#include "fhiclcpp/ParameterSet.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace boost;
namespace po = boost::program_options;

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>

int main(int ac, char* av[])
{
	std::string severity;
	std::string application;
	std::string message;
	std::string cat;
	std::string conf;
	bool dump;

	std::vector<std::string> messages;
	std::vector<std::string> vcat;

	std::vector<std::string> vcat_def;

	vcat_def.emplace_back("");

	try
	{
		po::options_description cmdopt("Allowed options");
		cmdopt.add_options()("help,h", "display help message")("severity,s",
		                                                       po::value<std::string>(&severity)->default_value("info"),
		                                                       "severity of the message (error, warning, info, debug)")(
		    "category,g", po::value<std::vector<std::string>>(&vcat)->default_value(vcat_def, "null"),
		    "message id / categories")("application,a",
		                               po::value<std::string>(&application)->default_value("msgsenderApplication"),
		                               "issuing application name")(
		    "config,c", po::value<std::string>(&conf)->default_value(""), "MessageFacility configuration file")(
		    "dump,d", po::bool_switch(&dump)->default_value(false));

		po::options_description hidden("Hidden options");
		hidden.add_options()("message", po::value<std::vector<std::string>>(&messages), "message text");

		po::options_description desc;
		desc.add(cmdopt).add(hidden);

		po::positional_options_description p;
		p.add("message", -1);

		po::variables_map vm;
		po::store(po::command_line_parser(ac, av).options(desc).positional(p).run(), vm);
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

	std::vector<std::string>::iterator it;

	// must have message text
	if (messages.empty())
	{
		std::cout << "Message text is missing!\n";
		std::cout << "Use \"msglogger --help\" for help messages\n";
		return 1;
	}

	if (application.empty())
	{
		std::cout << "Application name is missing!\n";
		std::cout << "Message cannot be issued without specifying the application name.\n";
		return 1;
	}

	// build message text string
	it = messages.begin();
	while (it != messages.end())
	{
		message += *it + " ";
		++it;
	}

	// checking severity...
	transform(severity.begin(), severity.end(), severity.begin(), ::toupper);
	if ((severity != "ERROR") && (severity != "WARNING") && (severity != "INFO") && (severity != "DEBUG"))
	{
		std::cerr << "Unknown severity level!\n";
		return 1;
	}

	// checking categories..
	it = vcat.begin();
	while (it != vcat.end())
	{
		cat += *it + ((it == vcat.end() - 1) ? "" : "|");
		++it;
	}

	// preparing parameterset for detinations...
	fhicl::ParameterSet pset;

	std::ifstream logfhicl(conf);
	if (logfhicl.is_open())
	{
		std::stringstream fhiclstream;
		fhiclstream << logfhicl.rdbuf();
		std::string pstr(fhiclstream.str());
		pset = fhicl::ParameterSet::make(pstr);
	}

	// start up message facility service
	mf::StartMessageFacility(pset);
	if (dump)
	{
		std::cout << pset.to_indented_string() << std::endl;
	}
	mf::SetApplicationName(application);

	// logging message...
	if (severity == "ERROR")
	{
		mf::LogError(cat) << message;
	}
	else if (severity == "WARNING")
	{
		mf::LogWarning(cat) << message;
	}
	else if (severity == "INFO")
	{
		mf::LogInfo(cat) << message;
	}
	else if (severity == "DEBUG")
	{
		mf::LogDebug(cat) << message;
	}

	return 0;
}
