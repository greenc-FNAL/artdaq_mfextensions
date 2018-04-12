#include "mfextensions/Receivers/LogReader_receiver.hh"
#include "mfextensions/Receivers/ReceiverMacros.hh"
#include <iostream>

mfviewer::LogReader::LogReader(fhicl::ParameterSet pset)
	: MVReceiver(pset)
	, log_()
	, pos_(0)
	, filename_(pset.get<std::string>("filename"))
	, counter_(0)
	, metadata_1
	("\\%MSG-([wide])\\s([^:]*):\\s\\s([^\\s]*)\\s*(\\d\\d-[^-]*-\\d{4}\\s\\d+:\\d+:\\d+)\\s.[DS]T\\s\\s(\\w+)")
	//, metadata_2 
	//  ( "([^\\s]*)\\s([^\\s]*)\\s([^\\s]*)\\s(([^\\s]*)\\s)?([^:]*):(\\d*)" )
{
	std::cout << "LogReader_receiver Constructor" << std::endl;
}

mfviewer::LogReader::~LogReader()
{
	log_.close();
}

void mfviewer::LogReader::run()
{
	while (!stopRequested_)
	{
		log_.open(filename_.c_str(), std::fstream::in);
		log_.seekg(pos_);

		if (log_.is_open() && log_.good())
		{
			bool msgFound = false;
			while (!msgFound)
			{
				if (log_.eof())
				{
					break;
				}

				std::string line;
				pos_ = log_.tellg();
				getline(log_, line);
				if (line.find("%MSG") != std::string::npos)
				{
					msgFound = true;
					log_.seekg(pos_);
				}
			}

			if (msgFound)
			{
				std::cout << "Message found, emitting!" << std::endl;
				emit NewMessage(read_next());
				++counter_;
			}
			log_.clear();
			pos_ = log_.tellg();
		}

		log_.clear();
		log_.close();
		usleep(500000);
		//sleep(1);
	}

	std::cout << "LogReader_receiver shutting down!" << std::endl;
}


#include <iostream>
#include <time.h>

qt_mf_msg mfviewer::LogReader::read_next()
{
	std::string line;

	// meta data 1
	getline(log_, line);

	std::string category, application, eventID;
	timeval tv;
	sev_code_t sev = SERROR;

	if (boost::regex_search(line, what_, metadata_1))
	{
#if 0
		std::cout << ">> " << std::string(what_[1].first, what_[1].second) << "\n";
		std::cout << ">> " << std::string(what_[2].first, what_[2].second) << "\n";
		std::cout << ">> " << std::string(what_[3].first, what_[3].second) << "\n";
		std::cout << ">> " << std::string(what_[4].first, what_[4].second) << "\n";
		std::cout << ">> " << std::string(what_[5].first, what_[5].second) << "\n";
#endif

		std::string value = std::string(what_[1].first, what_[1].second);

		switch (value[0])
		{
		case 'e': sev = SERROR;
			break;
		case 'w':sev = SWARNING;
			break;
		case 'i': sev = SINFO;
			break;
		case 'd': sev = SDEBUG;
			break;
		default: break;
		}

		struct tm tm;
		time_t t;

		value = std::string(what_[4].first, what_[4].second);
		strptime(value.c_str(), "%d-%b-%Y %H:%M:%S", &tm);

		tm.tm_isdst = -1;
		t = mktime(&tm);
		tv.tv_sec = t;
		tv.tv_usec = 0;

		category = std::string(what_[2].first, what_[2].second);
		application = std::string(what_[3].first, what_[3].second);

		eventID = std::string(what_[5].first, what_[5].second);
		//msg.setHostname ( std::string(what_[4].first, what_[4].second) );
		//msg.setHostaddr ( std::string(what_[5].first, what_[5].second) );
	}
	std::string body;
	getline(log_, line);

	qt_mf_msg msg("", category, application, 0, tv);
	msg.setSeverityLevel(sev);
	msg.setEventID(eventID);


	while (!log_.eof() && line.find("%MSG") == std::string::npos)
	{
		body += line;
		getline(log_, line);
	}

	msg.setMessage(filename_, counter_, body);
	msg.updateText();

	return msg;
}

#include "moc_LogReader_receiver.cpp"

DEFINE_MFVIEWER_RECEIVER(mfviewer::LogReader)
