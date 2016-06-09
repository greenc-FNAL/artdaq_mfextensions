#ifndef MSGVIEWERDLG_H
#define MSGVIEWERDLG_H

#include "ui_msgviewerdlgui.h"
#include <messagefacility/MessageLogger/MessageFacilityMsg.h>
#include "mfextensions/Extensions/throttle.hh"
#include "mfextensions/Extensions/suppress.hh"
#include "mfextensions/Binaries/qt_mf_msg.hh"
#include "mfextensions/Binaries/ReceiverManager.hh"
#include "mfextensions/Extensions/MFExtensions.hh"

#include <QtCore/QTimer>
#include <QtCore/QMutex>

#include <boost/regex.hpp>

#include <string>
#include <vector>
#include <map>
#include <list>

namespace fhicl
{
  class ParameterSet;
}

class msgViewerDlg : public QDialog, private Ui::MsgViewerDlg
{
  Q_OBJECT

public:
  msgViewerDlg( std::string const & part, std::string const & conf, QDialog *parent = 0 );


public slots:

  void pause();
  void exit();
  void clear();
  void shortMode();
  void changeSeverity(int sev);
  void switchChannel();

protected:
  void closeEvent(QCloseEvent *event);

private slots:

  void onNewMsg(mf::MessageFacilityMsg const & mfmsg);
  void onNewSysMsg(mfviewer::SysMsgCode code, QString const & msg);

  void setFilter();
  void resetFilter();

  void renderMode();

  void setSevError();
  void setSevWarning();
  void setSevInfo();
  void setSevDebug();

  void searchMsg();
  void searchClear();

  void setSuppression(QAction * act);
  void setThrottling(QAction * act);

  // Trim the leading lines of messages in message box to avoid overwhelming
  void updateDisplayMsgs();

//---------------------------------------------------------------------------

private:

  // Display all messages stored in the buffer
  void displayMsg();

  // test if the message is suppressed or throttled
  bool msg_throttled(mf::MessageFacilityMsg const & mfmsg);

  unsigned int update_index( msgs_t::iterator it );

  // Update the list. Returns true if there's a change in the selection
  // before and after the update. e.g., the selected entry has been deleted
  // during the process of updateMap(); otherwise it returns a false.
  template<typename M>
  bool updateList(QListWidget * lw, M const & map);

  void displayMsg( msgs_t::const_iterator it );
  void displayMsg( msg_iters_t const & msgs );

  void readSettings();
  void writeSettings();

  void parseConf(fhicl::ParameterSet const & conf);

  QStringList toQStringList(QList<QListWidgetItem *> in);

//---------------------------------------------------------------------------

private:


  // buffer size
  static const size_t  BUFFER_SIZE[4];
  static const size_t  MAX_DISPLAY_MSGS;

  bool              updating;
  bool              paused;
  bool              shortMode_;

  // # of received messages
  int               nMsgs;
  int               nSupMsgs;  // suppressed msgs
  int               nThrMsgs;  // throttled msgs

  // Rendering messages in speed mode or full mode
  bool              simpleRender;

  // severity threshold
  sev_code_t        sevThresh;

  // suppression regex
  std::vector<suppress>        e_sup_host;
  std::vector<suppress>        e_sup_app;
  std::vector<suppress>        e_sup_cat;

  // throttling regex
  std::vector<throttle>         e_thr_host;
  std::vector<throttle>         e_thr_app;
  std::vector<throttle>         e_thr_cat;


  // filter strings for hosts, applications, and categories
  QStringList       hostFilter;
  QStringList       appFilter;
  QStringList       catFilter;

  // search string
  QString           searchStr;

  // msg pool storing the formatted text body
  msgs_t            msg_pool_;

  // map of a key to a list of msg iters
  msg_iters_map_t   host_msgs_;
  msg_iters_map_t   cat_msgs_;

  msg_sevs_map_t    app_sev_msgs_;

  QString           buffer;
  QMutex            buf_lock;

  // Qt timer for purging the over displayed messages
  QTimer            timer;

  // context menu for "suppression" and "throttling" button
  QMenu           * sup_menu;
  QMenu           * thr_menu;

  //Receiver Plugin Manager
  mfviewer::ReceiverManager receivers_;
  
};

enum list_mask_t
{
  LIST_APP  = 0x01
, LIST_CAT  = 0x02
, LIST_HOST = 0x04
};

#endif
