#include "messagefacility/Utilities/ELseverityLevel.h"

#include "messagefacility/MessageService/ELdestination.h"
#include "mfextensions/Receivers/qt_mf_msg.hh"
//#include "mfextensions/Extensions/MFExtensions.hh"
#include <iostream>

size_t qt_mf_msg::sequence = 0;

qt_mf_msg::qt_mf_msg(const std::string& hostname, const std::string& category, const std::string& application, pid_t pid, timeval time)
    : text_(), shortText_(), color_(), sev_(SERROR), host_(QString(hostname.c_str())), cat_(QString(category.c_str())), app_(QString((application + " (" + std::to_string(pid) + ")").c_str())), time_(time), seq_(++sequence), msg_(""), application_(QString(application.c_str()).toHtmlEscaped()), pid_(QString::number(pid)) {}

void qt_mf_msg::setSeverity(mf::ELseverityLevel sev)
{
	int sevid = sev.getLevel();

	switch (sevid)
	{
		case mf::ELseverityLevel::ELsev_success:
		case mf::ELseverityLevel::ELsev_zeroSeverity:
		case mf::ELseverityLevel::ELsev_unspecified:
			sev_ = SDEBUG;
			break;

		case mf::ELseverityLevel::ELsev_info:
			sev_ = SINFO;
			break;

		case mf::ELseverityLevel::ELsev_warning:
			sev_ = SWARNING;
			break;

		case mf::ELseverityLevel::ELsev_error:
		case mf::ELseverityLevel::ELsev_severe:
		case mf::ELseverityLevel::ELsev_highestSeverity:
			sev_ = SERROR;
			break;

		default:
			break;
	}
}

void qt_mf_msg::setMessage(std::string const& prefix, int iteration, std::string const& msg)
{
	sourceType_ = QString(prefix.c_str()).toHtmlEscaped();
	sourceSequence_ = iteration;
	msg_ = QString(msg.c_str()).toHtmlEscaped();
}

void qt_mf_msg::updateText()
{
	text_ = QString("<font color=");

	QString sev_name = "Error";
	switch (sev_)
	{
		case SDEBUG:
			text_ += QString("#505050>");
			color_.setRgb(80, 80, 80);
			sev_name = "Debug";
			break;

		case SINFO:
			text_ += QString("#008000>");
			color_.setRgb(0, 128, 0);
			sev_name = "Info";
			break;

		case SWARNING:
			text_ += QString("#E08000>");
			color_.setRgb(224, 128, 0);
			sev_name = "Warning";
			break;

		case SERROR:
			text_ += QString("#FF0000>");
			color_.setRgb(255, 0, 0);
			sev_name = "Error";
			break;

		default:
			break;
	}

	shortText_ = QString(text_);
	shortText_ += QString("<pre style=\"margin-top: 0; margin-bottom: 0;\">");
	shortText_ += msg_;
	shortText_ += QString("</pre></font>");

	size_t constexpr SIZE{144};
	struct tm timebuf;
	char ts[SIZE];
	strftime(ts, sizeof(ts), "%d-%b-%Y %H:%M:%S %Z", localtime_r(&time_.tv_sec, &timebuf));

	text_ += QString("<pre style=\"width: 100%;\">") + sev_name.toHtmlEscaped() + " / " + cat_.toHtmlEscaped() + "<br>" +
	         QString(ts).toHtmlEscaped() + "<br>" + host_.toHtmlEscaped() + " (" + hostaddr_ + ")<br>" + sourceType_ +
	         " " + QString::number(sourceSequence_) + " / " + "PID " + pid_;

	if (file_ != "") text_ += QString(" / ") + file_ + QString(":") + line_;

	text_ += QString("<br>") + application_ + " / " + module_ + " / " + eventID_ + "<br>" + msg_  // + "<br>"
	         + QString("</pre>");

	text_ += QString("</font>");
}
