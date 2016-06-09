#include <messagefacility/MessageLogger/MessageFacilityMsg.h>
#include <messagefacility/MessageLogger/ELseverityLevel.h>

#include "mfextensions/Binaries/qt_mf_msg.hh"
//#include "mfextensions/Extensions/MFExtensions.hh"
#include <iostream>

qt_mf_msg::qt_mf_msg(mf::MessageFacilityMsg const & msg)
    : text_()
    , shortText_()
    , color_()
    , sev_()
    , host_(QString(msg.hostname().substr(0, msg.hostname().find_first_of('.')).c_str()))
    , cat_(QString(msg.category().c_str()))
    , app_(QString(msg.application().c_str()))
    , time_(msg.timestamp())
{
  mf::ELseverityLevel severity(msg.severity());
  int sevid = severity.getLevel();

    text_ = QString("<font color=");

    switch (sevid)
    {
    case mf::ELseverityLevel::ELsev_incidental:
    case mf::ELseverityLevel::ELsev_success:
    case mf::ELseverityLevel::ELsev_zeroSeverity:
    case mf::ELseverityLevel::ELsev_unspecified:
        sev_ = SDEBUG;
        text_ += QString("#505050>");
        color_.setRgb(80, 80, 80);
        break;

    case mf::ELseverityLevel::ELsev_info:
    case mf::ELseverityLevel::ELsev_severe:
        sev_ = SINFO;
        text_ += QString("#008000>");
        color_.setRgb(0, 128, 0);
        break;

    case mf::ELseverityLevel::ELsev_warning:
    case mf::ELseverityLevel::ELsev_warning2:
        sev_ = SWARNING;
        text_ += QString("#E08000>");
        color_.setRgb(224, 128, 0);
        break;

    case mf::ELseverityLevel::ELsev_error:
    case mf::ELseverityLevel::ELsev_error2:
    case mf::ELseverityLevel::ELsev_next:
    case mf::ELseverityLevel::ELsev_severe2:
    case mf::ELseverityLevel::ELsev_abort:
    case mf::ELseverityLevel::ELsev_fatal:
    case mf::ELseverityLevel::ELsev_highestSeverity:
        sev_ = SERROR;
        text_ += QString("#FF0000>");
        color_.setRgb(255, 0, 0);
        break;

    default: break;
    }

    shortText_ = QString(text_);
    shortText_ += QString("<p style=\"margin-top: 0; margin-bottom: 0;\">");
    shortText_ += QString(msg.message().c_str()).toHtmlEscaped();

    //std::cout << "qt_mf_msg.cc:" << msg.message() << std::endl;
    text_ += QString("<p>")
      + QString(severity.getName().c_str()).toHtmlEscaped()  + " / "
        + QString(msg.category().c_str()).toHtmlEscaped()  + "<br>"
        + QString(msg.timestr().c_str()).toHtmlEscaped()  + "<br>"
        + QString(msg.hostname().c_str()).toHtmlEscaped()  + " ("
        + QString(msg.hostaddr().c_str()).toHtmlEscaped()  + ")<br>"
        + QString(msg.process().c_str()).toHtmlEscaped()  + " ("
        + QString::number(msg.pid()) + ")";

    if (msg.file().compare("--"))
        text_ += QString(" / ") + QString(msg.file().c_str()).toHtmlEscaped() 
        + QString(":") + QString::number(msg.line());

    text_ += QString("<br>")
        + QString(msg.application().c_str()).toHtmlEscaped()  + " / "
        + QString(msg.module().c_str()).toHtmlEscaped()  + " / "
        + QString(msg.context().c_str()).toHtmlEscaped()  + "<br>"
      + QString(msg.message().c_str()).toHtmlEscaped()    // + "<br>"
        + QString("</p>");

    text_ += QString("</font>");
}
