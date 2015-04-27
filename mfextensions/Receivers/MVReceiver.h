#include <string>

#include <fhiclcpp/ParameterSet.h>

#include <messagefacility/MessageLogger/MessageFacilityMsg.h>
#include "mfextensions/Extensions/MFExtensions.h"

#include <QtCore/QThread>

namespace mfviewer {
  class MVReceiver : public QThread {
Q_OBJECT

public:
  MVReceiver() : partition_("0") {};
  virtual ~MVReceiver() =0;
  virtual void run() =0;
  virtual bool init(fhicl::ParameterSet pset) =0;
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
