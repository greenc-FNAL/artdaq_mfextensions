
#include <QtGui>
#include <QMenu>
#include <QProgressDialog>

#include <cetlib/filepath_maker.h>
#include <fhiclcpp/make_ParameterSet.h>
#include <fhiclcpp/ParameterSet.h>

#include "mvdlg.h"

const size_t msgViewerDlg::BUFFER_SIZE[4] = {500, 1000, 1000, 1000};
const size_t msgViewerDlg::MAX_DISPLAY_MSGS = 10000;

// replace the ${..} part in the filename with env variable
// throw if the env does not exist
static void
  process_fname(std::string & fname)
{
  size_t sub_start = fname.find("${");
  size_t sub_end   = fname.find("}");
        
  const size_t npos = std::string::npos;
             
  if(    (sub_start==npos && sub_end!=npos)
      || (sub_start!=npos && sub_end==npos)
      || (sub_start > sub_end) )
  {  
    throw std::runtime_error( "Unrecognized configuration file. Use default configuration instead." );
  }

  if( sub_start==npos ) return;

  std::string env = std::string( getenv( fname.substr(sub_start+2, sub_end-sub_start-2).c_str() ) );
  fname.replace(sub_start, sub_end-sub_start+1, env);

  //printf("%s\n", fname.c_str());
}

static fhicl::ParameterSet 
  readConf ( std::string const & fname )
{
  if( fname.empty() ) return fhicl::ParameterSet();

  std::string filename = fname;
  process_fname(filename);

  std::string env("FHICL_FILE_PATH=");

  if( filename[0]=='/' )
  {
    env.append("/");
  }
  else
  {
    env.append(".");
  }

  char * mfe_path = getenv( "MFEXTENSIONS_DIR" );
  if( mfe_path ) env.append(":").append(mfe_path).append("/config");

  putenv((char *)env.c_str());

  //printf("%s\n", env.c_str());

  cet::filepath_lookup policy("FHICL_FILE_PATH");

  // it throws when the file is not parsable
  fhicl::ParameterSet pset;
  fhicl::make_ParameterSet( filename, policy, pset );

  return pset;
}

msgViewerDlg::msgViewerDlg(std::string const & part, std::string const & conf, QDialog * parent)
  : QDialog(parent)
, updating      ( false )
, paused        ( false )
, nMsgs         ( 0     )
, nSupMsgs      ( 0     )
, nThrMsgs      ( 0     )
, simpleRender  ( true  )
, sevThresh     ( SINFO )
, hostFilter    ( )
, appFilter     ( )
, catFilter     ( )
, searchStr     ( ""    )
, msg_pool_     (       )
, host_msgs_    (       )
, cat_msgs_     (       )
, app_sev_msgs_ (       )
, buffer        (       )
, buf_lock      (       )
, timer         ( this  )
, sup_menu      ( new QMenu(this) )
, thr_menu      ( new QMenu(this) )
, receivers_    ( readConf(conf) )
{
  setupUi(this);

  // window geo settings
  readSettings();

  // read configuration file
  fhicl::ParameterSet pset = readConf(conf); 

  // parse configuration file
  parseConf(pset);

  // associate menu with push buttons
  btnSuppression->setMenu(sup_menu);
  btnThrottling ->setMenu(thr_menu);

  // slots
  connect( btnPause    , SIGNAL( clicked() ), this, SLOT( pause() ) );
  connect( btnExit     , SIGNAL( clicked() ), this, SLOT( exit()  ) );

  //connect( btnSwitchChannel, 
  //                       SIGNAL( clicked() ), this, SLOT( switchChannel() ) );
  btnSwitchChannel->setEnabled(false);

  connect( btnRMode    , SIGNAL( clicked() ), this, SLOT( renderMode()  ) );

  connect( btnSearch   , SIGNAL( clicked() ), this, SLOT( searchMsg()   ) );
  connect( btnSearchClear, 
                         SIGNAL( clicked() ), this, SLOT( searchClear() ) );

  connect( btnFilter   , SIGNAL( clicked() ), this, SLOT( setFilter()   ) );
  connect( btnReset    , SIGNAL( clicked() ), this, SLOT( resetFilter() ) );

  connect( btnClearHost, SIGNAL( clicked() ), this, SLOT( clearHostSelection() ) );
  connect( btnClearApp , SIGNAL( clicked() ), this, SLOT( clearAppSelection()  ) );
  connect( btnClearCat , SIGNAL( clicked() ), this, SLOT( clearCatSelection()  ) );

  connect( btnError    , SIGNAL( clicked() ), this, SLOT( setSevError()   ) );
  connect( btnWarning  , SIGNAL( clicked() ), this, SLOT( setSevWarning() ) );
  connect( btnInfo     , SIGNAL( clicked() ), this, SLOT( setSevInfo()    ) );
  connect( btnDebug    , SIGNAL( clicked() ), this, SLOT( setSevDebug()   ) );

  connect( sup_menu    
         , SIGNAL( triggered(QAction*) )
         , this        
         , SLOT( setSuppression(QAction*) ) );

  connect( thr_menu
         , SIGNAL( triggered(QAction*) )
         , this
         , SLOT( setThrottling(QAction*)  ) );

  connect( vsSeverity
         , SIGNAL( valueChanged(int) )
         , this
         , SLOT(changeSeverity(int)) );

  connect( &receivers_
	   , SIGNAL(newMessage(mf::MessageFacilityMsg const & ))
	   , this
	   , SLOT(onNewMsg(mf::MessageFacilityMsg const &)) );

  connect ( &receivers_
	    ,SIGNAL(newSysMessage(mfviewer::SysMsgCode, QString const & ))
	    ,this
	    ,SLOT(onNewSysMessage(mfviewer::SysMsgCode, QString const & )) );

  connect( &timer, SIGNAL(timeout()), this, SLOT(updateDisplayMsgs()));

  QString partStr = QString(part.c_str());
  btnSwitchChannel->setText(partStr);

  if(simpleRender)  btnRMode -> setChecked(true);
  else              btnRMode -> setChecked(false);

  btnRMode ->setEnabled(false);

  changeSeverity(sevThresh);

  QTextDocument * doc = new QTextDocument(txtMessages);
  doc->setMaximumBlockCount(1*MAX_DISPLAY_MSGS);
  txtMessages->setDocument(doc);

  // start the text edit widget update timer
  timer.start(100);
  receivers_.start();
}

static void str_to_suppress( std::vector<std::string> const & vs, std::vector<suppress> & s, QMenu * menu )
{
  QAction * act;

  if( vs.empty() )
  {
    act = menu->addAction("None"); 
    act->setEnabled(false); 
    return;
  }

  s.reserve( vs.size() );

  for(size_t i=0; i<vs.size(); ++i)
  {
    s.push_back( suppress( vs[i] ) );
    act = menu->addAction( QString(vs[i].c_str()) );
    act->setCheckable(true);
    act->setChecked(true);
    QVariant v = qVariantFromValue( (void*) &s[i] );
    act->setData( v );
  }
}

static void pset_to_throttle( std::vector<fhicl::ParameterSet> const & ps, std::vector<throttle> & t, QMenu * menu )
{
  QAction * act;

  if( ps.empty() )
  {
    act = menu->addAction("None"); 
    act->setEnabled(false); 
    return;
  }

  t.reserve( ps.size() );

  for(size_t i=0; i<ps.size(); ++i)
  {
    std::string name = ps[i].get<std::string>("name");
    t.push_back( throttle( name
                         , ps[i].get<int>("limit", -1)
                         , ps[i].get<long>("timespan", -1) ) );
    act = menu->addAction( QString(name.c_str()) );
    act->setCheckable(true);
    act->setChecked(true);
    QVariant v = qVariantFromValue( (void*) &t[i] );
    act->setData( v );
  }
}
 
void msgViewerDlg::parseConf(fhicl::ParameterSet const & conf)
{
  fhicl::ParameterSet nulp;
  //QAction * act;

  // suppression list
  fhicl::ParameterSet sup = conf.get<fhicl::ParameterSet>("suppress", nulp);

  auto sup_host = sup.get<std::vector<std::string>>( "hosts", std::vector<std::string>() );
  auto sup_app  = sup.get<std::vector<std::string>>( "applications", std::vector<std::string>() );
  auto sup_cat  = sup.get<std::vector<std::string>>( "categories", std::vector<std::string>() );

  str_to_suppress(sup_host, e_sup_host, sup_menu);
  sup_menu->addSeparator();

  str_to_suppress(sup_app , e_sup_app , sup_menu);
  sup_menu->addSeparator();

  str_to_suppress(sup_cat , e_sup_cat , sup_menu);

  // throttling list
  auto thr = conf.get<fhicl::ParameterSet>("throttle", nulp);

  auto thr_host = thr.get<std::vector<fhicl::ParameterSet>>( "hosts", std::vector<fhicl::ParameterSet>() );
  auto thr_app  = thr.get<std::vector<fhicl::ParameterSet>>( "applications", std::vector<fhicl::ParameterSet>() );
  auto thr_cat  = thr.get<std::vector<fhicl::ParameterSet>>( "categories", std::vector<fhicl::ParameterSet>() );

  pset_to_throttle(thr_host, e_thr_host, thr_menu);
  thr_menu->addSeparator();

  pset_to_throttle(thr_app , e_thr_app , thr_menu);
  thr_menu->addSeparator();

  pset_to_throttle(thr_cat , e_thr_cat , thr_menu);
}

bool msgViewerDlg::msg_throttled(mf::MessageFacilityMsg const & mfmsg)
{
  // suppression list

  ++nSupMsgs;

  for( size_t i=0; i<e_sup_host.size(); ++i )
    if( e_sup_host[i].match(mfmsg.hostname()) )
      return true;

  for( size_t i=0; i<e_sup_app.size(); ++i )
    if( e_sup_app[i].match(mfmsg.application()) )
      return true;

  for( size_t i=0; i<e_sup_cat.size(); ++i )
    if( e_sup_cat[i].match(mfmsg.category()) )
      return true;

  --nSupMsgs;

  // throttling

  ++nThrMsgs;

  for( size_t i=0; i<e_thr_host.size(); ++i )
    if( e_thr_host[i].reach_limit(mfmsg.hostname(), mfmsg.timestamp() ) )
      return true;

  for( size_t i=0; i<e_thr_app.size(); ++i )
    if( e_thr_app[i].reach_limit(mfmsg.application(), mfmsg.timestamp() ) )
      return true;

  for( size_t i=0; i<e_thr_cat.size(); ++i )
    if( e_thr_cat[i].reach_limit(mfmsg.category(), mfmsg.timestamp() ) )
      return true;

  --nThrMsgs;

  return false;
}

void msgViewerDlg::writeSettings()
{
  QSettings settings("ARTDAQ", "MsgViewer");

  settings.beginGroup("MainWindow");
  settings.setValue("size", size());
  settings.setValue("pos", pos());
  settings.endGroup();
}

void msgViewerDlg::readSettings()
{
  QSettings settings("ARTDAQ", "MsgViewer");

  settings.beginGroup("MainWindow");
  QPoint pos = settings.value("pos", QPoint(100, 100)).toPoint();
  QSize size = settings.value("size", QSize(660, 760)).toSize();
  resize(size);
  move(pos);
  settings.endGroup();
}

void msgViewerDlg::onNewMsg(mf::MessageFacilityMsg const & mfmsg) 
{
  // test if the message is suppressed or throttled
  if( msg_throttled(mfmsg) ) 
  {
    lcdSuppressionCount->display(nSupMsgs);
    lcdThrottlingCount->display(nThrMsgs);
    return;
  }

  // push the message to the message pool
  msg_pool_.push_back(mfmsg);
  msgs_t::iterator it = --msg_pool_.end();

  // update corresponding lists of index
  unsigned int flag = update_index(it);

  // update gui list
  if( flag & LIST_APP )  updateList<msg_sevs_map_t>(lwApplication, app_sev_msgs_);
  if( flag & LIST_CAT )  updateList<msg_iters_map_t>(lwCategory, cat_msgs_);
  if( flag & LIST_HOST ) updateList<msg_iters_map_t>(lwHost, host_msgs_);

  bool hostMatch = hostFilter.contains(it->host(), Qt::CaseInsensitive);
  bool appMatch  = appFilter.contains(it->app(), Qt::CaseInsensitive);
  bool catMatch  = catFilter.contains(it->cat(), Qt::CaseInsensitive);

  // Check to display the message
  if(  !(hostMatch || appMatch || catMatch)   ) 
  {
    displayMsg(it);
  }
}

void msgViewerDlg::onNewSysMsg( mfviewer::SysMsgCode syscode
                              , QString const & msg
                              ) 
{
  if(syscode == mfviewer::SysMsgCode::NEW_MESSAGE) 
  {
    ++nMsgs;
    lcdMsgs->display( nMsgs );
  }
  else 
  {
    QString qtmsg = "SYSTEM: " + msg + "\n";
    txtMessages->setTextColor(QColor(0, 0, 128));
    txtMessages->append(qtmsg);
  }
}

unsigned int msgViewerDlg::update_index( msgs_t::iterator it )
{
  sev_code_t      sev  = it->sev();
  QString const & app  = it->app();
  QString const & cat  = it->cat();
  QString const & host = it->host();

  unsigned int update = 0x0;

  if( cat_msgs_.find(cat)==cat_msgs_.end()    ) update |= LIST_CAT;
  if( host_msgs_.find(host)==host_msgs_.end() ) update |= LIST_HOST;

  cat_msgs_[cat].push_back(it);
  host_msgs_[host].push_back(it);

  msg_sevs_map_t::iterator ait = app_sev_msgs_.find(app);

  if( ait==app_sev_msgs_.end() )  // new app
  {
    msg_sevs_t msg_sevs(4);
    msg_sevs[sev].push_back(it);
    app_sev_msgs_.insert(std::make_pair(app, msg_sevs));

    update |= LIST_APP;
  }
  else  // already existing app
  {
    // push to corresponding list
    ait->second[sev].push_back(it);

    // remove earlist msg
    if( ait->second[sev].size()>BUFFER_SIZE[sev] )
    {
      // iter to the msg node to be deleted
      msgs_t::iterator mit = ait->second[sev].front().get();

      // update obselete host_map and cat_map
      msg_iters_map_t::iterator map_it = host_msgs_.find(mit->host());
      if( map_it!=host_msgs_.end() )
      {
        map_it->second.remove( ait->second[sev].front() );
        if( map_it->second.empty() ) 
        {
          host_msgs_.erase(map_it);
          update |= LIST_HOST;
        }
      }

      map_it = cat_msgs_.find(mit->cat());
      if( map_it!=cat_msgs_.end() )
      {
        map_it->second.remove( ait->second[sev].front() );
        if( map_it->second.empty() ) 
        {
          cat_msgs_.erase(map_it);
          update |= LIST_CAT;
        }
      }

      // remove the message from msg pool
      msg_pool_.erase( mit );

      // remove from app_sev index
      ait->second[sev].pop_front();
    }
  }

  return update;
}


void msgViewerDlg::displayMsg( msgs_t::const_iterator it )
{
  if( it->sev() < sevThresh ) return;

  buf_lock.lock();
  buffer += it->text();
  buf_lock.unlock();
}

void msgViewerDlg::displayMsg( msg_iters_t const & msgs )
{
  int n = 0;

  msg_iters_t::const_iterator it;

  if( msgs.size()>MAX_DISPLAY_MSGS )
  {
    n  = MAX_DISPLAY_MSGS;
    it = msgs.end();
    std::advance(it, -n);
  }
  else
  {
    n  = msgs.size();
    it = msgs.begin();
  }

  QProgressDialog progress( "Fetching data...", "Cancel"
                          , 0, n/1000, this);

  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(2000); // 2 seconds

  QString txt;
  int i = 0, prog = 0;

  updating = true;

  for( ; it!=msgs.end(); ++it, ++i)
  {
    if( it->get()->sev() >= sevThresh )
    {
      txt += it->get()->text();
    }

    if( i==1000 )
    {
      i=0; ++prog;
      progress.setValue(prog);

      txtMessages->append(txt);
      txt.clear();
    }

    if( progress.wasCanceled() )
      break;
  }

  txtMessages->append(txt);
  txtMessages->moveCursor(QTextCursor::End);

  updating = false;
}

// display all messages in buffer
void msgViewerDlg::displayMsg() 
{
  int n = 0;

  msgs_t::const_iterator it;

  if( msg_pool_.size()>MAX_DISPLAY_MSGS )
  {
    n  = MAX_DISPLAY_MSGS;
    it = msg_pool_.end();
    std::advance(it, -n);
  }
  else
  {
    n  = msg_pool_.size();
    it = msg_pool_.begin();
  }

  QProgressDialog progress( "Fetching data...", "Cancel"
                          , 0, n/1000, this);

  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(2000); // 2 seconds

  QString txt;
  int i = 0, prog = 0;

  updating = true;

  for( ; it!=msg_pool_.end(); ++it, ++i)
  {
    if( it->sev() >= sevThresh )
    {
      txt += it->text();
    }

    if( i==1000 )
    {
      i=0; ++prog;
      progress.setValue(prog);

      txtMessages->append(txt);
      txt.clear();
    }

    if( progress.wasCanceled() )
      break;
  }

  txtMessages->append(txt);
  txtMessages->moveCursor(QTextCursor::End);

  updating = false;
}

void msgViewerDlg::updateDisplayMsgs() 
{
  if( updating || paused ) return;

  buf_lock.lock();
  {
    if( !buffer.isEmpty() )
    {
      txtMessages->append(buffer);
      buffer.clear();
    }
  }
  buf_lock.unlock();
}

template <typename M>
bool msgViewerDlg::updateList( QListWidget * lw
                             , M const & map
                             ) 
{
  bool nonSelectedBefore = ( lw->currentRow() == -1 );
  bool nonSelectedAfter = true;

  QString item = nonSelectedBefore ? "" : lw->currentItem()->text();

  lw -> clear();
  int row = 0;
  typename M::const_iterator it = map.begin();

  while(it!=map.end()) 
  {
    lw -> addItem(it->first);
    if(!nonSelectedBefore && nonSelectedAfter) 
    {
      if(item == it->first) 
      {
        lw ->setCurrentRow(row);
        nonSelectedAfter = false;
      }
    }
    ++row;
    ++it;
  }

  if(!nonSelectedBefore && nonSelectedAfter)  return true;

  return false;
}

void list_intersect( msg_iters_t & l1, msg_iters_t const & l2 )
{
  msg_iters_t::iterator       it1 = l1.begin();
  msg_iters_t::const_iterator it2 = l2.begin();

  while( it1!=l1.end() && it2!=l2.end() )
  {
    if( *it1 < *it2 )      { it1 = l1.erase(it1); }
    else if( *it2 < *it1 ) { ++it2; }
    else                   { ++it1; ++it2; }
  }
}

void msgViewerDlg::setFilter() 
{
  hostFilter = toQStringList(lwHost->selectedItems());

  appFilter  = toQStringList(lwApplication->selectedItems());

  catFilter  = toQStringList(lwCategory->selectedItems());

  if(    hostFilter.isEmpty()
	 && appFilter.isEmpty()
	 && catFilter.isEmpty() ) 
  {
    resetFilter();
    return;
  }

  msg_iters_t result;

  for(auto app = 0; app < appFilter.size(); ++app)
  { // app-sev index
    msg_sevs_map_t::const_iterator it = app_sev_msgs_.find(appFilter[app]);

    if( it!=app_sev_msgs_.end() )
    {
      for(int s=sevThresh; s<=SERROR; ++s)
      {
        msg_iters_t temp(it->second[s]);
        result.merge(temp);
      }
    }
  }

  msg_iters_t hostResult;
  for(auto host = 0; host < hostFilter.size(); ++host)
  { // host index
    msg_iters_map_t::const_iterator it = host_msgs_.find(hostFilter[host]);
    if( it!=host_msgs_.end() )  
    {
      msg_iters_t temp(it->second);
      hostResult.merge(temp);
    }
  }
  if(result.empty()) { result = hostResult; }
  else { list_intersect(result, hostResult); }

  msg_iters_t catResult;
  for(auto cat = 0; cat < catFilter.size(); ++cat)
  { // cat index
    msg_iters_map_t::const_iterator it = cat_msgs_.find(catFilter[cat]);
    if( it!=cat_msgs_.end() )
    {
      msg_iters_t temp(it->second);
      catResult.merge(temp);
    }
  }
  if(result.empty()) { result = catResult; }
  else { list_intersect(result, catResult); }

  // Update the view
  txtMessages->clear();
  displayMsg(result);
}

void msgViewerDlg::resetFilter() 
{
  hostFilter.clear();
  appFilter.clear();
  catFilter.clear();

  lwHost->setCurrentRow(-1);
  lwApplication->setCurrentRow(-1);
  lwCategory->setCurrentRow(-1);

  // Update the view
  txtMessages->clear();
  displayMsg();
}

void msgViewerDlg::clearHostSelection() { lwHost->setCurrentRow(-1); }
void msgViewerDlg::clearAppSelection()  { lwApplication->setCurrentRow(-1); }
void msgViewerDlg::clearCatSelection()  { lwCategory->setCurrentRow(-1); }

void msgViewerDlg::pause()
{
  if( !paused )
  {
    paused = true;
    btnPause->setText("Resume");
    //QMessageBox::about(this, "About MsgViewer", "Message receiving paused ...");
  }
  else
  {
    paused = false;
    btnPause->setText("Pause");
  }
}

void msgViewerDlg::exit()
{
  close();
}

void msgViewerDlg::changeSeverity(int sev)
{
  mfviewer::SeverityCode s = mfviewer::GetSeverityCode(sev);

  switch(s) 
  {
  case mfviewer::SeverityCode::ERROR  :
      setSevError();   break;

    case mfviewer::SeverityCode::WARNING:
      setSevWarning(); break;

    case mfviewer::SeverityCode::INFO   :
      setSevInfo();    break;

    default: setSevDebug();
  }

  setFilter();
}

void msgViewerDlg::setSevError()
{
  sevThresh = SERROR;
  btnError   -> setChecked(true);
  btnWarning -> setChecked(false);
  btnInfo    -> setChecked(false);
  btnDebug   -> setChecked(false);
  vsSeverity -> setValue(sevThresh);
}

void msgViewerDlg::setSevWarning()
{
  sevThresh = SWARNING;
  btnError   -> setChecked(false);
  btnWarning -> setChecked(true);
  btnInfo    -> setChecked(false);
  btnDebug   -> setChecked(false);
  vsSeverity -> setValue(sevThresh);
}

void msgViewerDlg::setSevInfo()
{
  sevThresh = SINFO;
  btnError   -> setChecked(false);
  btnWarning -> setChecked(false);
  btnInfo    -> setChecked(true);
  btnDebug   -> setChecked(false);
  vsSeverity -> setValue(sevThresh);
}

void msgViewerDlg::setSevDebug()
{
  sevThresh = SDEBUG;
  btnError   -> setChecked(false);
  btnWarning -> setChecked(false);
  btnInfo    -> setChecked(false);
  btnDebug   -> setChecked(true);
  vsSeverity -> setValue(sevThresh);
}

void msgViewerDlg::switchChannel()
{
#if 0
  bool ok;
  int partition = QInputDialog::getInteger(this, 
    "Partition", 
    "Please enter a partition number:",
    qtdds.getPartition(),
    -1, 9, 1, &ok);

  if(ok)
  {
    qtdds.switchPartition(partition);

    QString partStr = "Partition " + QString::number(qtdds.getPartition());
    btnSwitchChannel->setText(partStr);
  }
#endif
}

void msgViewerDlg::renderMode()
{
  simpleRender = !simpleRender;

  if(simpleRender) 
  {
    btnRMode -> setChecked(true);
    txtMessages->setPlainText(txtMessages->toPlainText());
  }
  else 
  {
    btnRMode -> setChecked(false);
    setFilter();
  }
}

void msgViewerDlg::searchMsg()
{
  QString search = editSearch->text();

  if (search.isEmpty())
    return;
        
  if (search != searchStr) 
  {
    txtMessages->moveCursor(QTextCursor::Start);
    if (!txtMessages->find(search))
    {
      txtMessages->moveCursor(QTextCursor::End);
      searchStr = "";
    }
    else
      searchStr = search;
  }
  else
  {
    if (!txtMessages->find(search))
    {
      txtMessages->moveCursor(QTextCursor::Start);
      if (!txtMessages->find(search))
      {
        txtMessages->moveCursor(QTextCursor::End);
        searchStr = "";
      }
    }
  }
}

void msgViewerDlg::searchClear()
{
  editSearch->setText("");
  searchStr = "";
  txtMessages->find("");
  txtMessages->moveCursor(QTextCursor::End);
}

void msgViewerDlg::setSuppression(QAction * act)
{
  bool status = act->isChecked();
  suppress * sup = (suppress *)act->data().value<void*>();
  sup->use(status);
}

void msgViewerDlg::setThrottling(QAction * act)
{
  bool status = act->isChecked();
  throttle * thr = (throttle *)act->data().value<void*>();
  thr->use(status);
}

void msgViewerDlg::closeEvent(QCloseEvent *event)
{
  //qtdds.stop();
  receivers_.stop();
  writeSettings();
  event->accept();
}

QStringList msgViewerDlg::toQStringList(QList<QListWidgetItem *> in)
{
  QStringList out;
  
  for(auto i = 0; i < in.size(); ++i) {
    out << in[i]->text();
  }

  return out;
}
