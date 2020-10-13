#ifndef MF_LOG_READER_H
#define MF_LOG_READER_H

#include <fstream>
#include <string>

#include "fhiclcpp/fwd.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include <boost/regex.hpp>

#include "mfextensions/Receivers/MVReceiver.hh"

namespace mfviewer {
/// <summary>
/// MessageFacility Log Reader
///   Read messagefacility log archive and reemit as
///   messagefacility messages
/// </summary>
class LogReader : public MVReceiver
{
	Q_OBJECT
public:
	/// <summary>
	/// LogReader Constructor
	/// </summary>
	/// <param name="pset">ParameterSet used to configure the LogReader</param>
	explicit LogReader(const fhicl::ParameterSet& pset);

	/// <summary>
	/// LogReader Destructor
	/// </summary>
	virtual ~LogReader();

	/// <summary>
	/// Receiver loop method. Reads messages from file and emits newMessage signal
	/// </summary>
	void run();

	/// <summary>
	/// Read the next message from the input stream
	/// </summary>
	/// <returns>qt_mf_msg from log file</returns>
	msg_ptr_t read_next();  // read next log

	/**
	 * @brief Determine if the LogReader has reached the end of file
	 * @return Whether the reader is at the end of the file
	*/
	bool iseof() { return log_.eof(); }

private:
	LogReader(LogReader const&) = delete;
	LogReader(LogReader&&) = delete;
	LogReader& operator=(LogReader const&) = delete;
	LogReader& operator=(LogReader&&) = delete;

	std::ifstream log_;
	size_t pos_;

	std::string filename_;
	int counter_;

	boost::regex metadata_1;
	// boost::regex  metadata_2;
	boost::smatch what_;
};
}  // namespace mfviewer

#endif
