#include <QtWidgets/QApplication>
#include <QtWidgets/qdesktopwidget.h>
#include <cstdio>
#include <iostream>

#include "mfextensions/Binaries/mvdlg.hh"

void print_usage()
{
	std::cout << "usage: msgviewer [options]\n"
	          << "allowed options:\n"
	          << "  -h [ --help ]                 display this help message\n"
	          << "  -c [ --configuration ] arg    specify the configuration file to msgviewer\n";
}

int main(int argc, char** argv)
{
	QApplication app(argc, argv);

	std::string conf = std::string();

	if (argc > 1)
	{
		for (int i = 1; i < argc; ++i)
		{
			if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0))
			{
				print_usage();
				return 0;
			}

			if (((strcmp(argv[i], "-c") == 0) || (strcmp(argv[i], "--configuration") == 0)) && i < argc - 1)
			{
				conf = std::string(argv[i + 1]);
				++i;
			}

			else
			{
				std::cout << "unknown option: " << argv[i] << "\n";
				print_usage();
				return -1;
			}
		}
	}

	msgViewerDlg dialog(conf);
	dialog.setWindowFlags(Qt::Window);
	dialog.show();

	return QApplication::exec();

	return 0;
}
