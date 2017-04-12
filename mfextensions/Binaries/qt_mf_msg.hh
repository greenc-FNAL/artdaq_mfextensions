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

#include <sys/time.h>

namespace mf
{
	class MessageFacilityMsg;
}

enum sev_code_t
{
	SDEBUG,
	SINFO,
	SWARNING,
	SERROR
};

class qt_mf_msg
{
public:
	// ctor & dtor
	qt_mf_msg(mf::MessageFacilityMsg const& msg);

	// get method
	QString const& text(bool mode) const { return mode ? shortText_ : text_; }
	QColor const& color() const { return color_; }
	sev_code_t sev() const { return sev_; }
	QString const& host() const { return host_; }
	QString const& cat() const { return cat_; }
	QString const& app() const { return app_; }
	timeval time() const { return time_; }

private:

	QString text_;
	QString shortText_;
	QColor color_;
	sev_code_t sev_;
	QString host_;
	QString cat_;
	QString app_;
	timeval time_;
};

typedef std::list<qt_mf_msg> msgs_t;

class msg_iter_t
{
public:

	// ctor
	msg_iter_t(msgs_t::iterator it)
	{
		iter_ = it;
		time_ = it->time();
	}

	// operators
	bool operator==(msg_iter_t const& other) const
	{
		return iter_ == other.iter_;
	}

	bool operator<(msg_iter_t const& other) const
	{
		return (time_.tv_sec == other.time_.tv_sec)
			       ? (time_.tv_usec < other.time_.tv_usec)
			       : (time_.tv_sec < other.time_.tv_sec);
	}

	// get methods
	msgs_t::iterator get() const { return iter_; };


private:

	msgs_t::iterator iter_;
	timeval time_;
};

typedef std::list<msg_iter_t> msg_iters_t;
typedef std::vector<msg_iters_t> msg_sevs_t;
typedef std::map<QString, msg_sevs_t> msg_sevs_map_t;
typedef std::map<QString, msg_iters_t> msg_iters_map_t;

#endif
