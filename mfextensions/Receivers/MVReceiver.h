#include <string>

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
  virtual void stop() =0;
  const std::string& getPartition() { return partition_; }
  void setPartition(std::string const & partition) { partition_ = partition; }
protected:
    std::string partition_;
signals:
    void NewMessage(mf::MessageFacilityMsg const &);
    void NewSysMessage(mfviewer::SysMsgCode const &, QString const &);

};
}
