#include "messagefacility/Utilities/MessageFacilityMsg.h"
#include "messagefacility/Utilities/ELseverityLevel.h"

#include "mfextensions/Binaries/qt_mf_msg.hh"
//#include "mfextensions/Extensions/MFExtensions.hh"
#include <iostream>

qt_mf_msg::qt_mf_msg(mf::MessageFacilityMsg const& msg)
	: text_()
	, shortText_()
	, color_()
	, sev_()
	, host_(QString(msg.hostname().substr(0, msg.hostname().find_first_of('.')).c_str()))
	, cat_(QString(msg.category().c_str()))
	, app_(QString((msg.application() + " (" + std::to_string(msg.pid()) + ")").c_str()))
	, time_(msg.timestamp())
{
	mf::ELseverityLevel severity(msg.severity());
	int sevid = severity.getLevel();

	text_ = QString("<font color=");

	switch (sevid)
	{
#  if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	case mf::ELseverityLevel::ELsev_incidental:
#  endif
	case mf::ELseverityLevel::ELsev_success:
	case mf::ELseverityLevel::ELsev_zeroSeverity:
	case mf::ELseverityLevel::ELsev_unspecified:
		sev_ = SDEBUG;
		text_ += QString("#505050>");
		color_.setRgb(80, 80, 80);
		break;

	case mf::ELseverityLevel::ELsev_info:
		sev_ = SINFO;
		text_ += QString("#008000>");
		color_.setRgb(0, 128, 0);
		break;

	case mf::ELseverityLevel::ELsev_warning:
#  if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	case mf::ELseverityLevel::ELsev_warning2:
#  endif
		sev_ = SWARNING;
		text_ += QString("#E08000>");
		color_.setRgb(224, 128, 0);
		break;

	case mf::ELseverityLevel::ELsev_error:
#  if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
	case mf::ELseverityLevel::ELsev_error2:
	case mf::ELseverityLevel::ELsev_next:
	case mf::ELseverityLevel::ELsev_severe2:
	case mf::ELseverityLevel::ELsev_abort:
	case mf::ELseverityLevel::ELsev_fatal:
#  endif
	case mf::ELseverityLevel::ELsev_severe:
	case mf::ELseverityLevel::ELsev_highestSeverity:
		sev_ = SERROR;
		text_ += QString("#FF0000>");
		color_.setRgb(255, 0, 0);
		break;

	default: break;
	}

	shortText_ = QString(text_);
	shortText_ += QString("<pre style=\"margin-top: 0; margin-bottom: 0;\">");
	shortText_ += QString(msg.message().c_str()).trimmed().toHtmlEscaped();
	shortText_ += QString("</pre></font>");

	//std::cout << "qt_mf_msg.cc:" << msg.message() << std::endl;
	text_ += QString("<pre>")
		+ QString(severity.getName().c_str()).toHtmlEscaped() + " / "
		+ QString(msg.category().c_str()).toHtmlEscaped() + "<br>"
		+ QString(msg.timestr().c_str()).toHtmlEscaped() + "<br>"
		+ QString(msg.hostname().c_str()).toHtmlEscaped() + " ("
		+ QString(msg.hostaddr().c_str()).toHtmlEscaped() + ")<br>"
#  if MESSAGEFACILITY_HEX_VERSION < 0x20002 // v2_00_02 is s50, pre v2_00_02 is s48
		+ QString(msg.process().c_str()).toHtmlEscaped()
#endif
		+ " ("
		+ QString::number(msg.pid()) + ")";

	if (msg.file().compare("--"))
		text_ += QString(" / ") + QString(msg.file().c_str()).toHtmlEscaped()
			+ QString(":") + QString::number(msg.line());

	text_ += QString("<br>")
		+ QString(msg.application().c_str()).toHtmlEscaped() + " / "
		+ QString(msg.module().c_str()).toHtmlEscaped() + " / "
		+ QString(msg.context().c_str()).toHtmlEscaped() + "<br>"
		+ QString(msg.message().c_str()).toHtmlEscaped() // + "<br>"
		+ QString("</pre>");

	text_ += QString("</font>");
}
