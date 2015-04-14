#include <messagefacility/MessageLogger/MessageFacilityMsg.h>

#include "qt_mf_msg.h"
#include "mfextensions/Extensions/MFExtensions.h"

qt_mf_msg::qt_mf_msg(mf::MessageFacilityMsg const & msg)
    : text_()
    , color_()
    , sev_()
    , host_(QString(msg.hostname().substr(0, msg.hostname().find_first_of('.')).c_str()))
    , cat_(QString(msg.category().c_str()))
    , app_(QString(msg.application().c_str()))
    , time_(msg.timestamp())
{

    mfviewer::SeverityCode sevid =
        mfviewer::GetSeverityCode(msg.severity());

    text_ = QString("<font color=");

    switch (sevid)
    {
    case mfviewer::SeverityCode::DEBUG:
        sev_ = SDEBUG;
        text_ += QString("#505050>");
        color_.setRgb(80, 80, 80);
        break;

    case mfviewer::SeverityCode::INFO:
        sev_ = SINFO;
        text_ += QString("#008000>");
        color_.setRgb(0, 128, 0);
        break;

    case mfviewer::SeverityCode::WARNING:
        sev_ = SWARNING;
        text_ += QString("#E08000>");
        color_.setRgb(224, 128, 0);
        break;

    case mfviewer::SeverityCode::ERROR:
        sev_ = SERROR;
        text_ += QString("#FF0000>");
        color_.setRgb(255, 0, 0);
        break;

    default: break;
    }


    text_ += QString("<p>")
        + QString(msg.severity().c_str()) + " / "
        + QString(msg.category().c_str()) + "<br>"
        + QString(msg.timestr().c_str()) + "<br>"
        + QString(msg.hostname().c_str()) + " ("
        + QString(msg.hostaddr().c_str()) + ")<br>"
        + QString(msg.process().c_str()) + " ("
        + QString::number(msg.pid()) + ")";

    if (msg.file().compare("--"))
        text_ += QString(" / ") + QString(msg.file().c_str())
        + QString(":") + QString::number(msg.line());

    text_ += QString("<br>")
        + QString(msg.application().c_str()) + " / "
        + QString(msg.module().c_str()) + " / "
        + QString(msg.context().c_str()) + "<br>"
        + QString(msg.message().c_str())    // + "<br>"
        + QString("</p>");

    text_ += QString("</font>");
}
