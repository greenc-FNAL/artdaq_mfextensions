
#include "ErrorHandler/Components/qt_log_reader.h"
#include <fhiclcpp/ParameterSet.h>

using namespace novadaq::errorhandler;

bool qt_log_reader::open(QString const& filename)
{
	fhicl::ParameterSet ps;
	ps.put<std::string>("filename", filename.toStdString());
	reader = std::make_unique<mfviewer::LogReader>(ps);

	idx = 0;
	pause = false;
	return true;
}

void qt_log_reader::run()
{
	pause = false;

	reader->start();

	while (!reader->iseof())
	{
		if (pause)
		{
			reader->stop();
			pause = false;
			return;
		}

		// read
		qt_mf_msg msg = reader->read_next();
		emit newMessage(msg);

		// report progress
		//if( idx%10000000 == 0 )
		//  emit updateProgress(idx/10000000);

		//++idx;
	}

	emit readCompleted();
}

void qt_log_reader::pause_exec()
{
	pause = true;
	wait();
}

void qt_log_reader::resume()
{
	start();
}
