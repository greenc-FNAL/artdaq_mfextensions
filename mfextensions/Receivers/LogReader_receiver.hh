#ifndef MF_LOG_READER_H
#define MF_LOG_READER_H

// ------------------------------
// MessageFacility Log Reader
//
//   Read messagefacility log archive and reemit as 
//   messagefacility messages
//

#include <string>
#include <fstream>

#include <fhiclcpp/fwd.h>
#include <messagefacility/MessageLogger/MessageLogger.h>
#ifdef NO_MF_UTILITIES
#include <messagefacility/MessageLogger/MessageFacilityMsg.h>
#else
#include <messagefacility/Utilities/MessageFacilityMsg.h>
#endif

#include <boost/regex.hpp>

#include "mfextensions/Receivers/MVReceiver.hh"

namespace mfviewer
{
	class LogReader : public MVReceiver
	{
		Q_OBJECT
	public:

		LogReader(fhicl::ParameterSet pset);

		virtual ~LogReader();

		// Receiver Method
		void run();

		mf::MessageFacilityMsg read_next(); // read next log

	private:

		std::ifstream log_;
		size_t pos_;

		std::string filename_;
		int counter_;

		boost::regex metadata_1;
		//boost::regex  metadata_2;
		boost::smatch what_;
	};
} // end of namespace mf

#endif
