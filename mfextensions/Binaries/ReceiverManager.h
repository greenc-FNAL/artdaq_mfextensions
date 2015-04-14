#ifndef RECEIVER_MANAGER_H
#define RECEIVER_MANAGER_H

#include <fhiclcpp/ParameterSet.h>
#include <QObject>
#include <messagefacility/MessageLogger/MessageFacilityMsg.h>
#include "mfextensions/Extensions/MFExtensions.h"

namespace mfviewer {

  class ReceiverManager : public QObject {
  Q_OBJECT

  public:
    ReceiverManager(fhicl::ParameterSet pset);
    virtual ~ReceiverManager() {}
    void start() {;}
    void stop() {;}
  signals:
    void onNewMessage(mf::MessageFacilityMsg const &);
    void onNewSysMessage(mfviewer::SysMsgCode, QString const &);
  private:
    fhicl::ParameterSet pset_;
  };

}

#endif
