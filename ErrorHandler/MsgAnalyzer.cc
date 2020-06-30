#include <QtWidgets/QApplication>
#include <QtCore/QSettings>

#include "ErrorHandler/MsgAnalyzerDlg.h"

#include <iostream>
#include <messagefacility/MessageLogger/MessageLogger.h>
#include <fhiclcpp/make_ParameterSet.h>

using namespace novadaq::errorhandler;
using namespace std;

void printUsage()
{
  std::cout << "MsgAnalyzer usage:\n"
            << "  -h, --help                \tdisplay help message\n"
            << "  -c, --configuration [file]\tspecify the path and filename to the message analyzer conf file\n"
            << "  -l, --log [file]          \tspecify the path and filename to the log (messagefacility) conf file\n";
}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	std::string cfg("msganalyzer.fcl");
	std::string mf_cfg("msganalyzer_mf.fcl");

	int partition = 0;
	// The following lines will cause the application not working properly.
	// There is a hidden error involving pointers in QT in this code.
	//const char* partenv = getenv("NOVADAQ_PARTITION_NUMBER");
	//if(partenv) {
	//  partition = atoi(partenv);
	//}

	if (argc > 1)
	{
		for (int i = 0; i < argc; ++i)
		{
			if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
			{
				printUsage();
				return 0;
			}

			if ((!strcmp(argv[i], "-c") || !strcmp(argv[i], "--configuration")) && i < argc - 1)
			{
				cfg = std::string(argv[i + 1]);
				++i;
				continue;
			}

			if ((!strcmp(argv[i], "-l") || !strcmp(argv[i], "--log")) && i < argc - 1)
			{
				mf_cfg = std::string(argv[i + 1]);
				++i;
				continue;
			}
		}
	}

	// message facility
	//putenv((char*)"FHICL_FILE_PATH=.");
	fhicl::ParameterSet pset;
	try
	{
		cet::filepath_lookup_after1 lookup_policy("FHICL_FILE_PATH");
		make_ParameterSet(mf_cfg, lookup_policy, pset);
	}
	catch (cet::exception const& ex)
	{
		TLOG(TLVL_ERROR) << "Unable to load configuration file " << mf_cfg << ": " << ex.explain_self();
	}

  mf::StartMessageFacility(pset, "MsgAnalyzer");

  mf::SetIteration("context");
  mf::SetModuleName("module");


  // first log
  TLOG_DEBUG("category") << "DEBUG: MessageFacility service started";
  TLOG_INFO("category") << "INFO: MessageFacility service started";

  // start MA dialog
  MsgAnalyzerDlg dialog(cfg,partition);

  QSettings settings("NOvA DAQ", "MsgAnalyzer");
  dialog.restoreGeometry(settings.value("geometry").toByteArray());
  dialog.show();

  return app.exec();
}
