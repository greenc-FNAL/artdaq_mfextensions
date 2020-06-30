#ifndef ERRORHANDLER_QT_LOG_READER_H
#define ERRORHANDLER_QT_LOG_READER_H


#include "mfextensions/Receivers/LogReader_receiver.hh"
#include <QtCore/QThread>

namespace novadaq {
namespace errorhandler {

class qt_log_reader : public QThread
{
  Q_OBJECT

protected:
  virtual void run();

signals:
  void updateProgress(int);
  void readCompleted();
  void newMessage(qt_mf_msg const &);

public:
  qt_log_reader() : pause(false), idx(0), reader(nullptr) { }

  bool open( QString const & filename );

  void pause_exec();
  void resume();

private:
  bool pause;
  int  idx;

  std::unique_ptr<mfviewer::LogReader> reader;
  
};


} // end of namespace errorhandler
} // end of namespace novadaq


#endif
