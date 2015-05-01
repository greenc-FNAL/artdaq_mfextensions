#ifndef MFVIEWER_MVRECEIVER_H
#define MFVIEWER_MVRECEIVER_H

#include <string>

#include <fhiclcpp/ParameterSet.h>

#include <messagefacility/MessageLogger/MessageFacilityMsg.h>
#include "mfextensions/Extensions/MFExtensions.hh"

#include <QtCore/QThread>
#include <iostream>

namespace mfviewer {
  class MVReceiver : public QThread {
Q_OBJECT

public:
    MVReceiver(fhicl::ParameterSet pset) : partition_(pset.get<std::string>("Partition","0")), stopRequested_(false) {std::cout << "MVReceiver Constructor" << std::endl;}
    virtual ~MVReceiver() {;}
  
  void stop() {stopRequested_ = true;}
  const std::string& getPartition() { return partition_; }
  void setPartition(std::string const & partition) { partition_ = partition; }
protected:
    std::string partition_;
    bool        stopRequested_;
signals:
    void NewMessage(mf::MessageFacilityMsg const &);
    void NewSysMessage(mfviewer::SysMsgCode const &, QString const &);

};
}

#endif //MVRECEIVER_H
