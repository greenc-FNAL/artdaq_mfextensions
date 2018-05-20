#include <QtGui>
#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QScrollBar>

#include "cetlib/filepath_maker.h"
#include "fhiclcpp/make_ParameterSet.h"
#include "fhiclcpp/ParameterSet.h"

#include "mfextensions/Binaries/mvdlg.hh"


#if GCC_VERSION >= 701000 || defined(__clang__) 
#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#include "trace.h"

#if GCC_VERSION >= 701000 || defined(__clang__) 
#pragma GCC diagnostic pop 
#endif


#include "mvdlg.hh"

// replace the ${..} part in the filename with env variable
// throw if the env does not exist
static void
process_fname(std::string& fname)
{
	size_t sub_start = fname.find("${");
	size_t sub_end = fname.find("}");

	const size_t npos = std::string::npos;

	if ((sub_start == npos && sub_end != npos)
		|| (sub_start != npos && sub_end == npos)
		|| (sub_start > sub_end))
	{
		throw std::runtime_error("Unrecognized configuration file. Use default configuration instead.");
	}

	if (sub_start == npos) return;

	std::string env = std::string(getenv(fname.substr(sub_start + 2, sub_end - sub_start - 2).c_str()));
	fname.replace(sub_start, sub_end - sub_start + 1, env);

	//printf("%s\n", fname.c_str());
}

static fhicl::ParameterSet
readConf(std::string const& fname)
{
	if (fname.empty()) return fhicl::ParameterSet();

	std::string filename = fname;
	process_fname(filename);

	std::string env("FHICL_FILE_PATH=");

	if (filename[0] == '/')
	{
		env.append("/");
	}
	else
	{
		env.append(".");
	}

	char* mfe_path = getenv("MFEXTENSIONS_DIR");
	if (mfe_path) env.append(":").append(mfe_path).append("/config");

	putenv((char *)env.c_str());

	//printf("%s\n", env.c_str());

	cet::filepath_lookup policy("FHICL_FILE_PATH");

	// it throws when the file is not parsable
	fhicl::ParameterSet pset;
	fhicl::make_ParameterSet(filename, policy, pset);

	return pset;
}

msgViewerDlg::msgViewerDlg(std::string const& conf, QDialog* parent)
	: QDialog(parent)
	, updating(false)
	, paused(false)
	, shortMode_(false)
	, nMsgs(0)
	, nSupMsgs(0)
	, nThrMsgs(0)
	, nFilters(0)
	, simpleRender(true)
	, sevThresh(SINFO)
	, searchStr("")
	, msg_pool_()
	, host_msgs_()
	, cat_msgs_()
	, app_msgs_()
	, sup_menu(new QMenu(this))
	, thr_menu(new QMenu(this))
	, receivers_(readConf(conf).get<fhicl::ParameterSet>("receivers", fhicl::ParameterSet()))
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
	btnThrottling->setMenu(thr_menu);

	// slots
	connect(btnPause, SIGNAL(clicked()), this, SLOT(pause()));
	connect(btnExit, SIGNAL(clicked()), this, SLOT(exit()));
	connect(btnClear, SIGNAL(clicked()), this, SLOT(clear()));

	connect(btnRMode, SIGNAL(clicked()), this, SLOT(renderMode()));
	connect(btnDisplayMode, SIGNAL(clicked()), this, SLOT(shortMode()));

	connect(btnSearch, SIGNAL(clicked()), this, SLOT(searchMsg()));
	connect(btnSearchClear,
		SIGNAL(clicked()), this, SLOT(searchClear()));

	connect(btnFilter, SIGNAL(clicked()), this, SLOT(setFilter()));

	connect(btnError, SIGNAL(clicked()), this, SLOT(setSevError()));
	connect(btnWarning, SIGNAL(clicked()), this, SLOT(setSevWarning()));
	connect(btnInfo, SIGNAL(clicked()), this, SLOT(setSevInfo()));
	connect(btnDebug, SIGNAL(clicked()), this, SLOT(setSevDebug()));

	connect(sup_menu
		, SIGNAL(triggered(QAction*))
		, this
		, SLOT(setSuppression(QAction*)));

	connect(thr_menu
		, SIGNAL(triggered(QAction*))
		, this
		, SLOT(setThrottling(QAction*)));

	connect(vsSeverity
		, SIGNAL(valueChanged(int))
		, this
		, SLOT(changeSeverity(int)));

	connect(&receivers_
		, SIGNAL(newMessage(qt_mf_msg const &))
		, this
		, SLOT(onNewMsg(qt_mf_msg const &)));

	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabWidgetCurrentChanged(int)));
	connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
	MsgFilterDisplay allMessages;
	allMessages.txtDisplay = txtMessages;
	msgFilters_.push_back(allMessages);

	//https://stackoverflow.com/questions/2616483/close-button-only-for-some-tabs-in-qt
	QTabBar *tabBar = tabWidget->findChild<QTabBar *>();
	tabBar->setTabButton(0, QTabBar::RightSide, 0);
	tabBar->setTabButton(0, QTabBar::LeftSide, 0);

	if (simpleRender) btnRMode->setChecked(true);
	else btnRMode->setChecked(false);

	btnRMode->setEnabled(false);

	changeSeverity(sevThresh);

	QTextDocument* doc = new QTextDocument(txtMessages);
	txtMessages->setDocument(doc);

	receivers_.start();
}

msgViewerDlg::~msgViewerDlg()
{
	receivers_.stop();
	writeSettings();
}

static void str_to_suppress(std::vector<std::string> const& vs, std::vector<suppress>& s, QMenu* menu)
{
	QAction* act;

	if (vs.empty())
	{
		act = menu->addAction("None");
		act->setEnabled(false);
		return;
	}

	s.reserve(vs.size());

	for (size_t i = 0; i < vs.size(); ++i)
	{
		s.push_back(suppress(vs[i]));
		act = menu->addAction(QString(vs[i].c_str()));
		act->setCheckable(true);
		act->setChecked(true);
		QVariant v = qVariantFromValue((void*)&s[i]);
		act->setData(v);
	}
}

static void pset_to_throttle(std::vector<fhicl::ParameterSet> const& ps, std::vector<throttle>& t, QMenu* menu)
{
	QAction* act;

	if (ps.empty())
	{
		act = menu->addAction("None");
		act->setEnabled(false);
		return;
	}

	t.reserve(ps.size());

	for (size_t i = 0; i < ps.size(); ++i)
	{
		std::string name = ps[i].get<std::string>("name");
		t.push_back(throttle(name
			, ps[i].get<int>("limit", -1)
			, ps[i].get<long>("timespan", -1)));
		act = menu->addAction(QString(name.c_str()));
		act->setCheckable(true);
		act->setChecked(true);
		QVariant v = qVariantFromValue((void*)&t[i]);
		act->setData(v);
	}
}

void msgViewerDlg::parseConf(fhicl::ParameterSet const& conf)
{
	fhicl::ParameterSet nulp;
	//QAction * act;

	// suppression list
	fhicl::ParameterSet sup = conf.get<fhicl::ParameterSet>("suppress", nulp);

	auto sup_host = sup.get<std::vector<std::string>>("hosts", std::vector<std::string>());
	auto sup_app = sup.get<std::vector<std::string>>("applications", std::vector<std::string>());
	auto sup_cat = sup.get<std::vector<std::string>>("categories", std::vector<std::string>());

	str_to_suppress(sup_host, e_sup_host, sup_menu);
	sup_menu->addSeparator();

	str_to_suppress(sup_app, e_sup_app, sup_menu);
	sup_menu->addSeparator();

	str_to_suppress(sup_cat, e_sup_cat, sup_menu);

	// throttling list
	auto thr = conf.get<fhicl::ParameterSet>("throttle", nulp);

	auto thr_host = thr.get<std::vector<fhicl::ParameterSet>>("hosts", std::vector<fhicl::ParameterSet>());
	auto thr_app = thr.get<std::vector<fhicl::ParameterSet>>("applications", std::vector<fhicl::ParameterSet>());
	auto thr_cat = thr.get<std::vector<fhicl::ParameterSet>>("categories", std::vector<fhicl::ParameterSet>());

	pset_to_throttle(thr_host, e_thr_host, thr_menu);
	thr_menu->addSeparator();

	pset_to_throttle(thr_app, e_thr_app, thr_menu);
	thr_menu->addSeparator();

	pset_to_throttle(thr_cat, e_thr_cat, thr_menu);

	auto lvl = conf.get<std::string>("threshold", "INFO");
	if (lvl == "DEBUG" || lvl == "debug" || lvl == "0") { sevThresh = SDEBUG; }
	if (lvl == "INFO" || lvl == "info" || lvl == "1") { sevThresh = SINFO; }
	if (lvl == "WARN" || lvl == "warn" || lvl == "2") { sevThresh = SWARNING; }
	if (lvl == "ERROR" || lvl == "error" || lvl == "3") { sevThresh = SERROR; }
}

bool msgViewerDlg::msg_throttled(qt_mf_msg const& mfmsg)
{
	// suppression list

	++nSupMsgs;

	for (size_t i = 0; i < e_sup_host.size(); ++i)
		if (e_sup_host[i].match(mfmsg.host().toStdString()))
			return true;

	for (size_t i = 0; i < e_sup_app.size(); ++i)
		if (e_sup_app[i].match(mfmsg.app().toStdString()))
			return true;

	for (size_t i = 0; i < e_sup_cat.size(); ++i)
		if (e_sup_cat[i].match(mfmsg.cat().toStdString()))
			return true;

	--nSupMsgs;

	// throttling

	++nThrMsgs;

	for (size_t i = 0; i < e_thr_host.size(); ++i)
		if (e_thr_host[i].reach_limit(mfmsg.host().toStdString(), mfmsg.time()))
			return true;

	for (size_t i = 0; i < e_thr_app.size(); ++i)
		if (e_thr_app[i].reach_limit(mfmsg.app().toStdString(), mfmsg.time()))
			return true;

	for (size_t i = 0; i < e_thr_cat.size(); ++i)
		if (e_thr_cat[i].reach_limit(mfmsg.cat().toStdString(), mfmsg.time()))
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

void msgViewerDlg::onNewMsg(qt_mf_msg const& mfmsg)
{
	// 21-Aug-2015, KAB: copying the incrementing (and displaying) of the number
	// of messages to here. I'm also not sure if we want to
	// count all messages or just non-suppressed ones or what. But, at least this
	// change gets the counter incrementing on the display.
	++nMsgs;
	lcdMsgs->display(nMsgs);

	// test if the message is suppressed or throttled
	if (msg_throttled(mfmsg))
	{
		lcdSuppressionCount->display(nSupMsgs);
		lcdThrottlingCount->display(nThrMsgs);
		return;
	}

	// push the message to the message pool
	msg_pool_.emplace_back(mfmsg);
	msgs_t::iterator it = --msg_pool_.end();

	// update corresponding lists of index
	unsigned int flag = update_index(it);

	// update gui list
	if (flag & LIST_APP) updateList<msg_iters_map_t>(lwApplication, app_msgs_);
	if (flag & LIST_CAT) updateList<msg_iters_map_t>(lwCategory, cat_msgs_);
	if (flag & LIST_HOST) updateList<msg_iters_map_t>(lwHost, host_msgs_);

	for (size_t d = 0; d < msgFilters_.size(); ++d)
	{
		bool hostMatch = msgFilters_[d].hostFilter.contains(it->host(), Qt::CaseInsensitive) || msgFilters_[d].hostFilter.size() == 0;
		bool appMatch = msgFilters_[d].appFilter.contains(it->app(), Qt::CaseInsensitive) || msgFilters_[d].appFilter.size() == 0;
		bool catMatch = msgFilters_[d].catFilter.contains(it->cat(), Qt::CaseInsensitive) || msgFilters_[d].catFilter.size() == 0;

		// Check to display the message
		if (hostMatch && appMatch && catMatch)
		{
			msgFilters_[d].msgs.push_back(it);
			displayMsg(it, d);
		}
	}
}

unsigned int msgViewerDlg::update_index(msgs_t::iterator it)
{
	QString const& app = it->app();
	QString const& cat = it->cat();
	QString const& host = it->host();

	unsigned int update = 0x0;

	if (cat_msgs_.find(cat) == cat_msgs_.end()) update |= LIST_CAT;
	if (host_msgs_.find(host) == host_msgs_.end()) update |= LIST_HOST;
	if (app_msgs_.find(app) == app_msgs_.end()) update |= LIST_APP;

	cat_msgs_[cat].push_back(it);
	host_msgs_[host].push_back(it);
	app_msgs_[app].push_back(it);

	return update;
}


void msgViewerDlg::displayMsg(msgs_t::const_iterator it, int display)
{
	if (it->sev() < sevThresh) return;

	msgFilters_[display].nDisplayMsgs++;
	if (display == tabWidget->currentIndex())
	{
		lcdDisplayedMsgs->display(msgFilters_[display].nDisplayMsgs);
	}

	auto txt = it->text(shortMode_);
	UpdateTextAreaDisplay(txt, msgFilters_[display].txtDisplay);
}

void msgViewerDlg::displayMsg(int display)
{
	int n = 0;
	msgFilters_[display].txtDisplay->clear();
	msgFilters_[display].nDisplayMsgs = 0;

	msg_iters_t::const_iterator it;

	n = msgFilters_[display].msgs.size();
	it = msgFilters_[display].msgs.begin();
	QProgressDialog progress("Fetching data...", "Cancel"
		, 0, n / 1000, this);

	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(2000); // 2 seconds

	QString txt;
	int i = 0, prog = 0;

	updating = true;

	for (; it != msgFilters_[display].msgs.end(); ++it, ++i)
	{
		if (it->get()->sev() >= sevThresh)
		{
			txt += it->get()->text(shortMode_);
			++msgFilters_[display].nDisplayMsgs;
		}

		if (i == 1000)
		{
			i = 0;
			++prog;
			progress.setValue(prog);

			UpdateTextAreaDisplay(txt, msgFilters_[display].txtDisplay);
			txt.clear();
		}

		if (progress.wasCanceled())
			break;
	}

	if (display == tabWidget->currentIndex())
	{
		lcdDisplayedMsgs->display(msgFilters_[display].nDisplayMsgs);
	}

	UpdateTextAreaDisplay(txt, msgFilters_[display].txtDisplay);

	updating = false;
}

//https://stackoverflow.com/questions/21955923/prevent-a-qtextedit-widget-from-scrolling-when-there-is-a-selection
void msgViewerDlg::UpdateTextAreaDisplay(QString text, QTextEdit* widget)
{
	const QTextCursor old_cursor = widget->textCursor();
	const int old_scrollbar_value = widget->verticalScrollBar()->value();
	const bool is_scrolled_down = old_scrollbar_value >= widget->verticalScrollBar()->maximum() * 0.95; // At least 95% scrolled down
	
	// Insert the text at the position of the cursor (which is the end of the document).
	widget->append(text);

	if (old_cursor.hasSelection() || !is_scrolled_down)
	{
		// The user has selected text or scrolled away from the bottom: maintain position.
		widget->setTextCursor(old_cursor);
		widget->verticalScrollBar()->setValue(old_scrollbar_value);
	}
	else
	{
		// The user hasn't selected any text and the scrollbar is at the bottom: scroll to the bottom.
		widget->moveCursor(QTextCursor::End);
		widget->verticalScrollBar()->setValue(widget->verticalScrollBar()->maximum());
		widget->horizontalScrollBar()->setValue(0);
	}
}

void msgViewerDlg::updateDisplays()
{
	for (size_t ii = 0; ii < msgFilters_.size(); ++ii)
	{
		displayMsg(ii);
	}
}

template <typename M>
bool msgViewerDlg::updateList(QListWidget* lw, M const& map)
{
	bool nonSelectedBefore = (lw->currentRow() == -1);
	bool nonSelectedAfter = true;

	QString item = nonSelectedBefore ? "" : lw->currentItem()->text();

	lw->clear();
	int row = 0;
	typename M::const_iterator it = map.begin();

	while (it != map.end())
	{
		lw->addItem(it->first);
		if (!nonSelectedBefore && nonSelectedAfter)
		{
			if (item == it->first)
			{
				lw->setCurrentRow(row);
				nonSelectedAfter = false;
			}
		}
		++row;
		++it;
	}

	if (!nonSelectedBefore && nonSelectedAfter) return true;

	return false;
}

msg_iters_t msgViewerDlg::list_intersect(msg_iters_t const& l1, msg_iters_t const& l2)
{
	msg_iters_t output;
	msg_iters_t::const_iterator it1 = l1.begin();
	msg_iters_t::const_iterator it2 = l2.begin();

	while (it1 != l1.end() && it2 != l2.end())
	{
		if (*it1 < *it2) { ++it1; }
		else if (*it2 < *it1) { ++it2; }
		else
		{
			output.push_back(*it1);
			++it1;
			++it2;
		}
	}

	TLOG(10) << "list_intersect: output list has " << output.size() << " entries";
	return output;
}

std::string sev_to_string(sev_code_t s)
{
	switch (s)
	{
	case SDEBUG:
		return "DEBUG";
	case SINFO:
		return "INFO";
	case SWARNING:
		return "WARNING";
	case SERROR:
		return "ERROR";
	}
	return "UNKNOWN";
}

void msgViewerDlg::setFilter()
{
	auto hostFilter = toQStringList(lwHost->selectedItems());
	auto appFilter = toQStringList(lwApplication->selectedItems());
	auto catFilter = toQStringList(lwCategory->selectedItems());

	lwHost->setCurrentRow(-1, QItemSelectionModel::Clear);
	lwApplication->setCurrentRow(-1, QItemSelectionModel::Clear);
	lwCategory->setCurrentRow(-1, QItemSelectionModel::Clear);

	if (hostFilter.isEmpty()
		&& appFilter.isEmpty()
		&& catFilter.isEmpty())
	{
		return;
	}

	msg_iters_t result;
	QString catFilterExpression = "";
	QString hostFilterExpression = "";
	QString appFilterExpression = "";
	bool first = true;

	for (auto app = 0; app < appFilter.size(); ++app)
	{ // app-sev index
		msg_iters_map_t::const_iterator it = app_msgs_.find(appFilter[app]);
		appFilterExpression += QString(first ? "" : " || ") + appFilter[app];
		first = false;
		if (it != app_msgs_.end())
		{
			msg_iters_t temp(it->second);
			TLOG(10) << "setFilter: app " << appFilter[app].toStdString() << " has " << temp.size() << " messages";
			result.merge(temp);
		}
	}
	TLOG(10) << "setFilter: result contains %zu messages", result.size();

	first = true;
	if (!hostFilter.isEmpty())
	{
		msg_iters_t hostResult;
		for (auto host = 0; host < hostFilter.size(); ++host)
		{ // host index
			hostFilterExpression += QString(first ? "" : " || ") + hostFilter[host];
			first = false;
			msg_iters_map_t::const_iterator it = host_msgs_.find(hostFilter[host]);
			if (it != host_msgs_.end())
			{
				msg_iters_t temp(it->second);
				TLOG(10) << "setFilter: host " << hostFilter[host].toStdString() << " has " << temp.size() << " messages";
				hostResult.merge(temp);
			}
		}
		if (result.empty()) { result = hostResult; }
		else { result = list_intersect(result, hostResult); }
		TLOG(10) << "setFilter: result contains " << result.size() << " messages";
	}

	first = true;
	if (!catFilter.isEmpty())
	{
		msg_iters_t catResult;
		for (auto cat = 0; cat < catFilter.size(); ++cat)
		{ // cat index
			catFilterExpression += QString(first ? "" : " || ") + catFilter[cat];
			first = false;
			msg_iters_map_t::const_iterator it = cat_msgs_.find(catFilter[cat]);
			if (it != cat_msgs_.end())
			{
				msg_iters_t temp(it->second);
				TLOG(10) << "setFilter: cat " << catFilter[cat].toStdString() << " has " << temp.size() << " messages";
				catResult.merge(temp);
			}
		}
		if (result.empty()) { result = catResult; }
		else { result = list_intersect(result, catResult); }
		TLOG(10) << "setFilter: result contains " << result.size() << " messages";
	}

	// Create the filter expression
	auto nFilterExpressions = (appFilterExpression != "" ? 1 : 0) + (hostFilterExpression != "" ? 1 : 0) + (catFilterExpression != "" ? 1 : 0);
	QString filterExpression = "";
	if (nFilterExpressions == 1)
	{
		filterExpression = catFilterExpression + hostFilterExpression + appFilterExpression;
	}
	else
	{
		filterExpression = "(" + (catFilterExpression != "" ? catFilterExpression + ") && (" : "")
			+ hostFilterExpression
			+ (hostFilterExpression != "" && appFilterExpression != "" ? ") && (" : "")
			+ appFilterExpression + ")";
	}

	// Add the tab and populate it

	auto newTabTitle = QString("Filter ") + QString::number(++nFilters);
	QWidget* newTab = new QWidget();

	QTextEdit* txtDisplay = new QTextEdit(newTab);
	QTextDocument* doc = new QTextDocument(txtDisplay);
	txtDisplay->setDocument(doc);

	QVBoxLayout* layout = new QVBoxLayout();
	layout->addWidget(txtDisplay);
	layout->setContentsMargins(0, 0, 0, 0);
	newTab->setLayout(layout);

	MsgFilterDisplay filteredMessages;
	filteredMessages.msgs = result;
	filteredMessages.hostFilter = hostFilter;
	filteredMessages.appFilter = appFilter;
	filteredMessages.catFilter = catFilter;
	filteredMessages.txtDisplay = txtDisplay;
	filteredMessages.nDisplayMsgs = result.size();
	msgFilters_.push_back(filteredMessages);

	tabWidget->addTab(newTab, newTabTitle);
	tabWidget->setTabToolTip(tabWidget->count() - 1, filterExpression);
	tabWidget->setCurrentIndex(tabWidget->count() - 1);

	displayMsg(msgFilters_.size() - 1);
}

void msgViewerDlg::pause()
{
	if (!paused)
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

void msgViewerDlg::clear()
{
	int ret = QMessageBox::question(this, tr("Message Viewer"), tr("Are you sure you want to clear all received messages?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	switch (ret)
	{
	case QMessageBox::Yes:
		nMsgs = 0;
		nSupMsgs = 0;
		nThrMsgs = 0;
		msg_pool_.clear();
		host_msgs_.clear();
		cat_msgs_.clear();
		app_msgs_.clear();
		updateList<msg_iters_map_t>(lwApplication, app_msgs_);
		updateList<msg_iters_map_t>(lwCategory, cat_msgs_);
		updateList<msg_iters_map_t>(lwHost, host_msgs_);
		for (auto& display : msgFilters_)
		{
			display.txtDisplay->clear();
			display.msgs.clear();
			display.nDisplayMsgs = 0;
		}

		lcdMsgs->display(nMsgs);
		lcdDisplayedMsgs->display(0);
		break;
	case QMessageBox::No:
	default:
		break;
	}
}

void msgViewerDlg::shortMode()
{
	if (!shortMode_)
	{
		shortMode_ = true;
		btnDisplayMode->setText("Long View");
	}
	else
	{
		shortMode_ = false;
		btnDisplayMode->setText("Compact View");
	}
	updateDisplays();
}

void msgViewerDlg::changeSeverity(int sev)
{
	switch (sev)
	{
	case SERROR:
		setSevError();
		break;

	case SWARNING:
		setSevWarning();
		break;

	case SINFO:
		setSevInfo();
		break;

	default: setSevDebug();
	}

	updateDisplays();
}

void msgViewerDlg::setSevError()
{
	sevThresh = SERROR;
	btnError->setChecked(true);
	btnWarning->setChecked(false);
	btnInfo->setChecked(false);
	btnDebug->setChecked(false);
	vsSeverity->setValue(sevThresh);
}

void msgViewerDlg::setSevWarning()
{
	sevThresh = SWARNING;
	btnError->setChecked(false);
	btnWarning->setChecked(true);
	btnInfo->setChecked(false);
	btnDebug->setChecked(false);
	vsSeverity->setValue(sevThresh);
}

void msgViewerDlg::setSevInfo()
{
	sevThresh = SINFO;
	btnError->setChecked(false);
	btnWarning->setChecked(false);
	btnInfo->setChecked(true);
	btnDebug->setChecked(false);
	vsSeverity->setValue(sevThresh);
}

void msgViewerDlg::setSevDebug()
{
	sevThresh = SDEBUG;
	btnError->setChecked(false);
	btnWarning->setChecked(false);
	btnInfo->setChecked(false);
	btnDebug->setChecked(true);
	vsSeverity->setValue(sevThresh);
}

void msgViewerDlg::renderMode()
{
	simpleRender = !simpleRender;

	if (simpleRender)
	{
		btnRMode->setChecked(true);
		for (auto display : msgFilters_)
		{
			display.txtDisplay->setPlainText(display.txtDisplay->toPlainText());
		}
	}
	else
	{
		btnRMode->setChecked(false);
		updateDisplays();
	}
}

void msgViewerDlg::searchMsg()
{
	QString search = editSearch->text();

	if (search.isEmpty())
		return;

	auto display = tabWidget->currentIndex();
	if (search != searchStr)
	{
		msgFilters_[display].txtDisplay->moveCursor(QTextCursor::Start);
		if (!msgFilters_[display].txtDisplay->find(search))
		{
			msgFilters_[display].txtDisplay->moveCursor(QTextCursor::End);
			searchStr = "";
		}
		else
			searchStr = search;
	}
	else
	{
		if (!msgFilters_[display].txtDisplay->find(search))
		{
			msgFilters_[display].txtDisplay->moveCursor(QTextCursor::Start);
			if (!msgFilters_[display].txtDisplay->find(search))
			{
				msgFilters_[display].txtDisplay->moveCursor(QTextCursor::End);
				searchStr = "";
			}
		}
	}
}

void msgViewerDlg::searchClear()
{
	auto display = tabWidget->currentIndex();
	editSearch->setText("");
	searchStr = "";
	msgFilters_[display].txtDisplay->find("");
	msgFilters_[display].txtDisplay->moveCursor(QTextCursor::End);
}

void msgViewerDlg::setSuppression(QAction* act)
{
	bool status = act->isChecked();
	suppress* sup = (suppress *)act->data().value<void*>();
	sup->use(status);
}

void msgViewerDlg::setThrottling(QAction* act)
{
	bool status = act->isChecked();
	throttle* thr = (throttle *)act->data().value<void*>();
	thr->use(status);
}

void msgViewerDlg::tabWidgetCurrentChanged(int newTab)
{
	lcdDisplayedMsgs->display(msgFilters_[newTab].nDisplayMsgs);

	lwHost->setCurrentRow(-1, QItemSelectionModel::Clear);
	lwApplication->setCurrentRow(-1, QItemSelectionModel::Clear);
	lwCategory->setCurrentRow(-1, QItemSelectionModel::Clear);

	for (auto host : msgFilters_[newTab].hostFilter)
	{
		auto items = lwHost->findItems(host, Qt::MatchExactly);
		if (items.size() > 0)
		{
			items[0]->setSelected(true);
		}
	}
	for (auto app : msgFilters_[newTab].appFilter)
	{
		auto items = lwApplication->findItems(app, Qt::MatchExactly);
		if (items.size() > 0)
		{
			items[0]->setSelected(true);
		}
	}
	for (auto cat : msgFilters_[newTab].catFilter)
	{
		auto items = lwCategory->findItems(cat, Qt::MatchExactly);
		if (items.size() > 0)
		{
			items[0]->setSelected(true);
		}
	}
}

void msgViewerDlg::tabCloseRequested(int tabIndex)
{
	if (tabIndex == 0 || static_cast<size_t>(tabIndex) >= msgFilters_.size()) return;

	auto widget = tabWidget->widget(tabIndex);
	tabWidget->removeTab(tabIndex);
	delete widget;

	msgFilters_.erase(msgFilters_.begin() + tabIndex);
}

void msgViewerDlg::closeEvent(QCloseEvent* event)
{
	event->accept();
}

QStringList msgViewerDlg::toQStringList(QList<QListWidgetItem *> in)
{
	QStringList out;

	for (auto i = 0; i < in.size(); ++i)
	{
		out << in[i]->text();
	}

	return out;
}
