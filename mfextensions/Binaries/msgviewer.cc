#include <stdio.h>
#include <iostream>
#include <QtGui/QApplication>
#include <QtGui/qdesktopwidget.h>

#include "mfextensions/Binaries/mvdlg.hh"

void print_usage()
{
  std::cout << "usage: msgviewer [options]\n"
            << "allowed options:\n"
            << "  -h [ --help ]                 display this help message\n"
            << "  -c [ --configuration ] arg    specify the configuration file to msgviewer\n";
}


int main( int argc, char ** argv )
{
  QApplication app(argc, argv);

  std::string p = mfviewer::NULL_PARTITION;
  std::string conf = std::string();

  if(argc >1) 
  {
    for(int i = 1; i<argc; ++i) 
    {
      if( !strcmp(argv[i], "-h") || !strcmp(argv[i], "--help") )
      {
        print_usage();
        return 0;
      }

      else if( (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--configuration"))
          && i<argc-1 )
      {
        conf = std::string(argv[i+1]);
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

  msgViewerDlg dialog(p, conf);
  dialog.show();

  return app.exec();

  return 0;
}
