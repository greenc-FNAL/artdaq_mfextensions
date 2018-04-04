#ifndef MESSAGEFACILITY_EXTENSIONS_QT_MF_MSG_H
#define MESSAGEFACILITY_EXTENSIONS_QT_MF_MSG_H

// Wrapped mf message type to be used by msgviewer for
// the purpose of fast processing

#include <QtCore/QString>
#include <QtGui/QColor>

#include <string>
#include <vector>
#include <list>
#include <map>

#include "messagefacility/Utilities/ELseverityLevel.h"
#include <sys/time.h>

namespace mf
{
	class ErrorObj;
}

/// <summary>
/// Severity codes enumeration for internal use
/// </summary>
enum sev_code_t
{
	SDEBUG,
	SINFO,
	SWARNING,
	SERROR
};

/// <summary>
/// Qt wrapper around MessageFacility message
/// </summary>
class qt_mf_msg
{
public:
	/// <summary>
	/// Construct a qt_mf_msg
	/// </summary>
	qt_mf_msg(std::string hostname, std::string category, std::string application, pid_t pid, timeval time);

	qt_mf_msg() {}
	qt_mf_msg(const qt_mf_msg&) = default;

	// get method
	/// <summary>
	/// Get the text of the message
	/// </summary>
	/// <param name="mode">Whether to return the short-form text</param>
	/// <returns>Text of the message</returns>
	QString const& text(bool mode) const { return mode ? shortText_ : text_; }
	/// <summary>
	/// Get the severity-based color of the message
	/// </summary>
	/// <returns>Color of the message</returns>
	QColor const& color() const { return color_; }
	/// <summary>
	/// Get the severity of the message
	/// </summary>
	/// <returns>Message severity</returns>
	sev_code_t sev() const { return sev_; }
	/// <summary>
	/// Get the host from which the message came
	/// </summary>
	/// <returns>Hostname of message</returns>
	QString const& host() const { return host_; }
	/// <summary>
	/// Get the category of the message
	/// </summary>
	/// <returns>Message category</returns>
	QString const& cat() const { return cat_; }
	/// <summary>
	/// Get the application of the message
	/// </summary>
	/// <returns>Message application</returns>
	QString const& app() const { return app_; }
	/// <summary>
	/// Get the message timestamp
	/// </summary>
	/// <returns>Timestamp of the message</returns>
	timeval time() const { return time_; }
	/// <summary>
	/// Get the sequence number of the message
	/// </summary>
	/// <returns>Message sequence number</returns>
	size_t seq() const { return seq_; }

	void setSeverity(mf::ELseverityLevel sev);
	void setSeverityLevel(sev_code_t sev) { sev_ = sev; }
	void setMessage(std::string prefix, int iteration, std::string msg) { msg_ = QString((prefix + " " + std::to_string(iteration) + ": " + msg).c_str()).trimmed().toHtmlEscaped(); }
	void setHostAddr(std::string hostaddr) { hostaddr_ = QString(hostaddr.c_str()).toHtmlEscaped(); }
	void setFileName(std::string file) { file_ = QString(file.c_str()).toHtmlEscaped(); }
	void setLineNumber(std::string line) { line_ = QString(line.c_str()).toHtmlEscaped(); }
	void setModule(std::string module) { module_ = QString(module.c_str()).toHtmlEscaped(); }
	void setEventID(std::string eventID) { eventID_ = QString(eventID.c_str()).toHtmlEscaped(); }

	void updateText();
private:

	QString text_;
	QString shortText_;
	QColor color_;
	sev_code_t sev_;
	QString host_;
	QString cat_;
	QString app_;
	timeval time_;
	size_t seq_;
	static size_t sequence;

	QString msg_;
	QString application_;
	QString pid_;
	QString hostaddr_;
	QString file_;
	QString line_;
	QString module_;
	QString eventID_;

};

/// <summary>
/// A std::list of qt_mf_msgs
/// </summary>
typedef std::list<qt_mf_msg> msgs_t;

/// <summary>
/// Iterator for the msgs_t type
/// </summary>
class msg_iter_t
{
public:

	/// <summary>
	/// Construct a msg_iter_t
	/// </summary>
	/// <param name="it">Iterator to msgs_t list</param>
	msg_iter_t(msgs_t::iterator it)
	{
		iter_ = it;
		seq_ = it->seq();
	}

	/// <summary>
	/// Equality operator. Equality is based on the message's sequence number
	/// </summary>
	/// <param name="other">msg_iter_t to check for equality</param>
	/// <returns>True if both messages have the same sequence id</returns>
	bool operator==(msg_iter_t const& other) const
	{
		return seq_ == other.seq_;
	}

	/// <summary>
	/// Comparison operator, based on message sequence number
	/// </summary>
	/// <param name="other">msg_iter_t to compare</param>
	/// <returns>seq() < other.seq()</returns>
	bool operator<(msg_iter_t const& other) const
	{
		return seq_ < other.seq_;
	}

	/// <summary>
	/// Get the wrapper iterator
	/// </summary>
	/// <returns>msgs_t::iterator</returns>
	msgs_t::iterator get() const { return iter_; };


private:

	msgs_t::iterator iter_;
	size_t seq_;
};

/// <summary>
/// A std::list of msg_iter_ts
/// </summary>
typedef std::list<msg_iter_t> msg_iters_t;
/// <summary>
/// A std::map relating a QString and a msg_iters_t
/// </summary>
typedef std::map<QString, msg_iters_t> msg_iters_map_t;

#endif
