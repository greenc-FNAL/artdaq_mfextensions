#include <string>

#include <messagefacility/MessageLogger/MessageFacilityMsg.h>
#include "mfextensions/Extensions/MFExtensions.h"

#include <QtCore/QThread>

namespace mfviewer {
  class MVReceiver : public QThread {
Q_OBJECT

public:
  MVReceiver();
  virtual ~MVReceiver() =0;
  virtual const std::string & getPartition() = 0;
  virtual void setPartition(std::string const & partition) =0;
  virtual void start() =0;
  virtual void stop() =0;
signals:
  void NewMessage(mf::MessageFacilityMsg const &);
  void NewSysMessage(mfviewer::SysMsgCode const &, QString const &);

};
}
