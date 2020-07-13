#include <QMenu>
#include <QMessageBox>
#include <QProgressDialog>
#include <QScrollBar>
#include <QtGui>

#include "cetlib/filepath_maker.h"
#include "fhiclcpp/ParameterSet.h"
#include "fhiclcpp/make_ParameterSet.h"

#include "mfextensions/Binaries/mvdlg.hh"

#if GCC_VERSION >= 701000 || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#define TRACE_NAME "MessageViewer"
#include "trace.h"

#if GCC_VERSION >= 701000 || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "mvdlg.hh"

// replace the ${..} part in the filename with env variable
// throw if the env does not exist
static void process_fname(std::string& fname)
{
	size_t sub_start = fname.find("${");
	size_t sub_end = fname.find("}");

	const size_t npos = std::string::npos;

	if ((sub_start == npos && sub_end != npos) || (sub_start != npos && sub_end == npos) || (sub_start > sub_end))
	{
		throw std::runtime_error("Unrecognized configuration file. Use default configuration instead.");
	}

	if (sub_start == npos) return;

	std::string env = std::string(getenv(fname.substr(sub_start + 2, sub_end - sub_start - 2).c_str()));
	fname.replace(sub_start, sub_end - sub_start + 1, env);

	// printf("%s\n", fname.c_str());
}

static fhicl::ParameterSet readConf(std::string const& fname)
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

	env.append("\0"); // So that putenv gets a valid C string

	putenv(&env[0]);

	// printf("%s\n", env.c_str());

	cet::filepath_lookup policy("FHICL_FILE_PATH");

	// it throws when the file is not parsable
	fhicl::ParameterSet pset;
	fhicl::make_ParameterSet(filename, policy, pset);

	return pset;
}

msgViewerDlg::msgViewerDlg(std::string const& conf, QDialog* parent)
    : QDialog(parent), paused(false), shortMode_(false), nMsgs(0), nSupMsgs(0), nThrMsgs(0), nFilters(0), nDeleted(0), simpleRender(true), searchStr(""), msg_pool_(), host_msgs_(), cat_msgs_(), app_msgs_(), sup_menu(new QMenu(this)), thr_menu(new QMenu(this)), receivers_(readConf(conf).get<fhicl::ParameterSet>("receivers", fhicl::ParameterSet()))
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
	connect(btnScrollToBottom, SIGNAL(clicked()), this, SLOT(scrollToBottom()));
	connect(btnExit, SIGNAL(clicked()), this, SLOT(exit()));
	connect(btnClear, SIGNAL(clicked()), this, SLOT(clear()));

	connect(btnRMode, SIGNAL(clicked()), this, SLOT(renderMode()));
	connect(btnDisplayMode, SIGNAL(clicked()), this, SLOT(shortMode()));

	connect(btnSearch, SIGNAL(clicked()), this, SLOT(searchMsg()));
	connect(btnSearchClear, SIGNAL(clicked()), this, SLOT(searchClear()));

	connect(btnFilter, SIGNAL(clicked()), this, SLOT(setFilter()));

	connect(btnError, SIGNAL(clicked()), this, SLOT(setSevError()));
	connect(btnWarning, SIGNAL(clicked()), this, SLOT(setSevWarning()));
	connect(btnInfo, SIGNAL(clicked()), this, SLOT(setSevInfo()));
	connect(btnDebug, SIGNAL(clicked()), this, SLOT(setSevDebug()));

	connect(sup_menu, SIGNAL(triggered(QAction*)), this, SLOT(setSuppression(QAction*)));

	connect(thr_menu, SIGNAL(triggered(QAction*)), this, SLOT(setThrottling(QAction*)));

	connect(vsSeverity, SIGNAL(valueChanged(int)), this, SLOT(changeSeverity(int)));

	connect(&receivers_, SIGNAL(newMessage(msg_ptr_t)), this, SLOT(onNewMsg(msg_ptr_t)));

	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabWidgetCurrentChanged(int)));
	connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
	MsgFilterDisplay allMessages;
	allMessages.txtDisplay = txtMessages;
	allMessages.nDisplayMsgs = 0;
	allMessages.filterExpression = "";
	allMessages.nDisplayedDeletedMsgs = 0;
	allMessages.sevThresh = SINFO;
	msgFilters_.push_back(allMessages);

	// https://stackoverflow.com/questions/2616483/close-button-only-for-some-tabs-in-qt
	auto tabBar = tabWidget->findChild<QTabBar*>();
	tabBar->setTabButton(0, QTabBar::RightSide, nullptr);
	tabBar->setTabButton(0, QTabBar::LeftSide, nullptr);

	if (simpleRender)
		btnRMode->setChecked(true);
	else
		btnRMode->setChecked(false);

	btnRMode->setEnabled(false);

	changeSeverity(SINFO);

	auto doc = new QTextDocument(txtMessages);
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
		s.emplace_back(vs[i]);
		act = menu->addAction(QString(vs[i].c_str()));
		act->setCheckable(true);
		act->setChecked(true);
		QVariant v = qVariantFromValue(static_cast<void*>(&s[i]));
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
		auto name = ps[i].get<std::string>("name");
		t.emplace_back(name, ps[i].get<int>("limit", -1), ps[i].get<int64_t>("timespan", -1));
		act = menu->addAction(QString(name.c_str()));
		act->setCheckable(true);
		act->setChecked(true);
		QVariant v = qVariantFromValue(static_cast<void*>(&t[i]));
		act->setData(v);
	}
}

void msgViewerDlg::parseConf(fhicl::ParameterSet const& conf)
{
	fhicl::ParameterSet nulp;
	// QAction * act;

	// suppression list
	auto sup = conf.get<fhicl::ParameterSet>("suppress", nulp);

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

	maxMsgs = conf.get<size_t>("max_message_buffer_size", 100000);
	maxDeletedMsgs = conf.get<size_t>("max_displayed_deleted_messages", 100000);
}

bool msgViewerDlg::msg_throttled(msg_ptr_t const& msg)
{
	// suppression list

	++nSupMsgs;

	for (size_t i = 0; i < e_sup_host.size(); ++i)
		if (e_sup_host[i].match(msg->host().toStdString())) return true;

	for (size_t i = 0; i < e_sup_app.size(); ++i)
		if (e_sup_app[i].match(msg->app().toStdString())) return true;

	for (size_t i = 0; i < e_sup_cat.size(); ++i)
		if (e_sup_cat[i].match(msg->cat().toStdString())) return true;

	--nSupMsgs;

	// throttling

	++nThrMsgs;

	for (size_t i = 0; i < e_thr_host.size(); ++i)
		if (e_thr_host[i].reach_limit(msg->host().toStdString(), msg->time())) return true;

	for (size_t i = 0; i < e_thr_app.size(); ++i)
		if (e_thr_app[i].reach_limit(msg->app().toStdString(), msg->time())) return true;

	for (size_t i = 0; i < e_thr_cat.size(); ++i)
		if (e_thr_cat[i].reach_limit(msg->cat().toStdString(), msg->time())) return true;

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

void msgViewerDlg::onNewMsg(msg_ptr_t const& msg)
{
	// 21-Aug-2015, KAB: copying the incrementing (and displaying) of the number
	// of messages to here. I'm also not sure if we want to
	// count all messages or just non-suppressed ones or what. But, at least this
	// change gets the counter incrementing on the display.
	++nMsgs;
	lcdMsgs->display(nMsgs);

	// test if the message is suppressed or throttled
	if (msg_throttled(msg))
	{
		lcdSuppressionCount->display(nSupMsgs);
		lcdThrottlingCount->display(nThrMsgs);
		return;
	}


	// push the message to the message pool
	{
		std::lock_guard<std::mutex> lk(msg_pool_mutex_);
		msg_pool_.emplace_back(msg);
	}
	trim_msg_pool();

	// update corresponding lists of index
	update_index(msg);

	// Update filtered displays
	for (size_t d = 0; d < msgFilters_.size(); ++d)
	{
		bool hostMatch =
		    msgFilters_[d].hostFilter.contains(msg->host(), Qt::CaseInsensitive) || msgFilters_[d].hostFilter.empty();
		bool appMatch =
		    msgFilters_[d].appFilter.contains(msg->app(), Qt::CaseInsensitive) || msgFilters_[d].appFilter.empty();
		bool catMatch =
		    msgFilters_[d].catFilter.contains(msg->cat(), Qt::CaseInsensitive) || msgFilters_[d].catFilter.empty();

		// Check to display the message
		if (hostMatch && appMatch && catMatch)
		{
				std::lock_guard<std::mutex> lk(filter_mutex_);
				msgFilters_[d].msgs.push_back(msg);
			if ((int)d == tabWidget->currentIndex())
			displayMsg(msg, d);
		}
	}
}

void msgViewerDlg::trim_msg_pool()
{
	bool host_list_update = false;
	bool app_list_update = false;
	bool cat_list_update = false;
	{
		std::lock_guard<std::mutex> lk(msg_pool_mutex_);
		while (maxMsgs > 0 && msg_pool_.size() > maxMsgs)
		{
			QString const& app = msg_pool_.front()->app();
			QString const& cat = msg_pool_.front()->cat();
			QString const& host = msg_pool_.front()->host();

			// Check if we can remove an app/host/category
			{
				auto catIter = std::find(cat_msgs_[cat].begin(), cat_msgs_[cat].end(), msg_pool_.front());
				auto hostIter = std::find(host_msgs_[host].begin(), host_msgs_[host].end(), msg_pool_.front());
				auto appIter = std::find(app_msgs_[app].begin(), app_msgs_[app].end(), msg_pool_.front());
	if (catIter != cat_msgs_[cat].end()) cat_msgs_[cat].erase(catIter);
	if (hostIter != host_msgs_[host].end()) host_msgs_[host].erase(hostIter);
	if (appIter != app_msgs_[app].end()) app_msgs_[app].erase(appIter);

	if (app_msgs_[app].empty())
	{
		app_msgs_.erase(app);
					app_list_update = true;
	}
	if (cat_msgs_[cat].empty())
	{
		cat_msgs_.erase(cat);
					cat_list_update = true;
	}
	if (host_msgs_[host].empty())
	{
		host_msgs_.erase(host);
					host_list_update = true;
		}
	}

			// Finally, remove the message from the pool so it doesn't appear in new filters
			msg_pool_.erase(msg_pool_.begin());
	++nDeleted;
		}
	}
	{
		std::lock_guard<std::mutex> lk(msg_classification_mutex_);
		if (host_list_update)
			updateList(lwHost, host_msgs_);
		if (app_list_update)
			updateList(lwApplication, app_msgs_);
		if (cat_list_update)
			updateList(lwCategory, cat_msgs_);
	}

	for (size_t d = 0; d < msgFilters_.size(); ++d)
	{
		{
			std::lock_guard<std::mutex> lk(filter_mutex_);
			while (msgFilters_[d].msgs.size() > maxMsgs)
			{
				if ((*msgFilters_[d].msgs.begin())->sev() >= msgFilters_[d].sevThresh)
					msgFilters_[d].nDisplayedDeletedMsgs++;
				msgFilters_[d].msgs.erase(msgFilters_[d].msgs.begin());
			}
		}

		if ((int)d == tabWidget->currentIndex())
		{
			if (maxDeletedMsgs > 0 && msgFilters_[d].nDisplayedDeletedMsgs > static_cast<int>(maxDeletedMsgs))
			{
				displayMsgs(d);
			}
			lcdDisplayedDeleted->display(msgFilters_[d].nDisplayedDeletedMsgs);
		}
	}

	lcdDeletedCount->display(nDeleted);
}

void msgViewerDlg::update_index(msg_ptr_t const& it)
{
	std::lock_guard<std::mutex> lk(msg_classification_mutex_);
	QString const& app = it->app();
	QString const& cat = it->cat();
	QString const& host = it->host();

	if (cat_msgs_.find(cat) == cat_msgs_.end())
	{
		cat_msgs_[cat].push_back(it);
		updateList(lwCategory, cat_msgs_);
	}
	else
	{
	cat_msgs_[cat].push_back(it);
	}

	if (host_msgs_.find(host) == host_msgs_.end())
	{
	host_msgs_[host].push_back(it);
		updateList(lwHost, host_msgs_);
	}
	else
	{
		host_msgs_[host].push_back(it);
	}

	if (app_msgs_.find(app) == app_msgs_.end())
	{
		app_msgs_[app].push_back(it);
		updateList(lwApplication, app_msgs_);
	}
	else
	{
		app_msgs_[app].push_back(it);
	}
}

void msgViewerDlg::displayMsg(msg_ptr_t const& it, int display)
{
	if (it->sev() < msgFilters_[display].sevThresh) return;

	msgFilters_[display].nDisplayMsgs++;
	if (display == tabWidget->currentIndex())
	{
		lcdDisplayedMsgs->display(msgFilters_[display].nDisplayMsgs);
	}

	auto txt = it->text(shortMode_);
	QStringList txts;
	txts.push_back(txt);
	UpdateTextAreaDisplay(txts, msgFilters_[display].txtDisplay);
}

void msgViewerDlg::displayMsgs(int display)
{
	int n = 0;
	msgFilters_[display].txtDisplay->clear();
	msgFilters_[display].nDisplayMsgs = 0;
	msgFilters_[display].nDisplayedDeletedMsgs = 0;

	QStringList txts;
	{
		std::lock_guard<std::mutex> lk(filter_mutex_);
	n = msgFilters_[display].msgs.size();

	QProgressDialog progress("Fetching data...", "Cancel", 0, n / 1000, this);

	progress.setWindowModality(Qt::WindowModal);
	progress.setMinimumDuration(2000);  // 2 seconds

	int i = 0, prog = 0;

		for (auto it = msgFilters_[display].msgs.begin(); it != msgFilters_[display].msgs.end(); ++it, ++i)
	{
			if ((*it)->sev() >= msgFilters_[display].sevThresh)
		{
				txts.push_back((*it)->text(shortMode_));
			++msgFilters_[display].nDisplayMsgs;
		}

		if (i == 1000)
		{
			i = 0;
			++prog;
			progress.setValue(prog);
		}

		if (progress.wasCanceled()) break;
	}

	if (display == tabWidget->currentIndex())
	{
		lcdDisplayedMsgs->display(msgFilters_[display].nDisplayMsgs);
	}
	}
	UpdateTextAreaDisplay(txts, msgFilters_[display].txtDisplay);
}

// https://stackoverflow.com/questions/13559990/how-to-append-text-to-qplaintextedit-without-adding-newline-and-keep-scroll-at
void msgViewerDlg::UpdateTextAreaDisplay(QStringList const& texts, QPlainTextEdit* widget)
{
	const QTextCursor old_cursor = widget->textCursor();
	const int old_scrollbar_value = widget->verticalScrollBar()->value();
	const bool is_scrolled_down =
	    old_scrollbar_value >= widget->verticalScrollBar()->maximum() * 0.95;  // At least 95% scrolled down

	if (!paused && !is_scrolled_down)
	{
		pause();
	}

	QTextCursor new_cursor = QTextCursor(widget->document());

	new_cursor.beginEditBlock();
	new_cursor.movePosition(QTextCursor::End);

	for (int i = 0; i < texts.size(); i++)
	{
		new_cursor.insertBlock();
		new_cursor.insertHtml(texts.at(i));
		if (!shortMode_) new_cursor.insertBlock();
	}
	new_cursor.endEditBlock();

	if (old_cursor.hasSelection() || paused)
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

void msgViewerDlg::scrollToBottom()
{
	int display = tabWidget->currentIndex();
	msgFilters_[display].txtDisplay->moveCursor(QTextCursor::End);
	msgFilters_[display].txtDisplay->verticalScrollBar()->setValue(
	    msgFilters_[display].txtDisplay->verticalScrollBar()->maximum());
	msgFilters_[display].txtDisplay->horizontalScrollBar()->setValue(0);
}

void msgViewerDlg::updateDisplays()
{
	for (size_t ii = 0; ii < msgFilters_.size(); ++ii)
	{
		displayMsgs(ii);
	}
}

bool msgViewerDlg::updateList(QListWidget* lw, msgs_map_t const& map)
{
	bool nonSelectedBefore = (lw->currentRow() == -1);
	bool nonSelectedAfter = true;

	QString item = nonSelectedBefore ? "" : lw->currentItem()->text();

	lw->clear();
	int row = 0;
	auto it = map.begin();

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

msgs_t msgViewerDlg::list_intersect(msgs_t const& l1, msgs_t const& l2)
{
	msgs_t output;
	auto it1 = l1.begin();
	auto it2 = l2.begin();

	while (it1 != l1.end() && it2 != l2.end())
	{
		if (*it1 < *it2)
		{
			++it1;
		}
		else if (*it2 < *it1)
		{
			++it2;
		}
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

	if (hostFilter.isEmpty() && appFilter.isEmpty() && catFilter.isEmpty())
	{
		return;
	}

	msgs_t result;
	QString catFilterExpression = "";
	QString hostFilterExpression = "";
	QString appFilterExpression = "";
	bool first = true;
	{
	for (auto app = 0; app < appFilter.size(); ++app)
	{  // app-sev index
		appFilterExpression += QString(first ? "" : " || ") + appFilter[app];
		first = false;
		}
		TLOG(10) << "setFilter: result contains %zu messages", result.size();

		first = true;
		if (!hostFilter.isEmpty())
		{
			msgs_t hostResult;
			for (auto host = 0; host < hostFilter.size(); ++host)
			{  // host index
				hostFilterExpression += QString(first ? "" : " || ") + hostFilter[host];
				first = false;
			}
		}

		first = true;
		if (!catFilter.isEmpty())
		{
			msgs_t catResult;
			for (auto cat = 0; cat < catFilter.size(); ++cat)
			{  // cat index
				catFilterExpression += QString(first ? "" : " || ") + catFilter[cat];
				first = false;
			}
		}
	}
	// Create the filter expression
	auto nFilterExpressions =
	    (appFilterExpression != "" ? 1 : 0) + (hostFilterExpression != "" ? 1 : 0) + (catFilterExpression != "" ? 1 : 0);
	QString filterExpression = "";
	if (nFilterExpressions == 1)
	{
		filterExpression = catFilterExpression + hostFilterExpression + appFilterExpression;
	}
	else
	{
		filterExpression = "(" + (catFilterExpression != "" ? catFilterExpression + ") && (" : "") + hostFilterExpression +
		                   (hostFilterExpression != "" && appFilterExpression != "" ? ") && (" : "") + appFilterExpression +
		                   ")";
	}

	for (size_t d = 0; d < msgFilters_.size(); ++d) {
		if (msgFilters_[d].filterExpression == filterExpression)
		{
			tabWidget->setCurrentIndex(d);
			return;
		}
	}

	{
		std::lock_guard<std::mutex> lk(msg_classification_mutex_);
		for (auto app = 0; app < appFilter.size(); ++app)
		{  // app-sev index
			auto it = app_msgs_.find(appFilter[app]);
		if (it != app_msgs_.end())
		{
				msgs_t temp(it->second);
			TLOG(10) << "setFilter: app " << appFilter[app].toStdString() << " has " << temp.size() << " messages";
			result.merge(temp);
		}
	}
	TLOG(10) << "setFilter: result contains %zu messages", result.size();

	if (!hostFilter.isEmpty())
	{
			msgs_t hostResult;
		for (auto host = 0; host < hostFilter.size(); ++host)
		{  // host index
			auto it = host_msgs_.find(hostFilter[host]);
			if (it != host_msgs_.end())
			{
					msgs_t temp(it->second);
				TLOG(10) << "setFilter: host " << hostFilter[host].toStdString() << " has " << temp.size() << " messages";
				hostResult.merge(temp);
			}
		}
		if (result.empty())
		{
			result = hostResult;
		}
		else
		{
			result = list_intersect(result, hostResult);
		}
		TLOG(10) << "setFilter: result contains " << result.size() << " messages";
	}

	if (!catFilter.isEmpty())
	{
			msgs_t catResult;
		for (auto cat = 0; cat < catFilter.size(); ++cat)
		{  // cat index
			auto it = cat_msgs_.find(catFilter[cat]);
			if (it != cat_msgs_.end())
			{
					msgs_t temp(it->second);
				TLOG(10) << "setFilter: cat " << catFilter[cat].toStdString() << " has " << temp.size() << " messages";
				catResult.merge(temp);
			}
		}
		if (result.empty())
		{
			result = catResult;
		}
		else
		{
			result = list_intersect(result, catResult);
		}
		TLOG(10) << "setFilter: result contains " << result.size() << " messages";
	}
	}

	// Add the tab and populate it

	auto newTabTitle = QString("Filter ") + QString::number(++nFilters);
	auto newTab = new QWidget();

	auto txtDisplay = new QPlainTextEdit(newTab);
	auto doc = new QTextDocument(txtDisplay);
	txtDisplay->setDocument(doc);

	auto layout = new QVBoxLayout();
	layout->addWidget(txtDisplay);
	layout->setContentsMargins(0, 0, 0, 0);
	newTab->setLayout(layout);

	MsgFilterDisplay filteredMessages;
	filteredMessages.msgs = result;
	filteredMessages.hostFilter = hostFilter;
	filteredMessages.appFilter = appFilter;
	filteredMessages.catFilter = catFilter;
	filteredMessages.filterExpression = filterExpression;
	filteredMessages.txtDisplay = txtDisplay;
	filteredMessages.nDisplayMsgs = result.size();
	filteredMessages.nDisplayedDeletedMsgs = 0;
	filteredMessages.sevThresh = SINFO;
	{
		std::lock_guard<std::mutex> lk(filter_mutex_);
	msgFilters_.push_back(filteredMessages);
	}
	tabWidget->addTab(newTab, newTabTitle);
	tabWidget->setTabToolTip(tabWidget->count() - 1, filterExpression);
	tabWidget->setCurrentIndex(tabWidget->count() - 1);

	displayMsgs(msgFilters_.size() - 1);
}

void msgViewerDlg::pause()
{
	if (!paused)
	{
		paused = true;
		btnPause->setText("Resume Scrolling");
		// QMessageBox::about(this, "About MsgViewer", "Message receiving paused ...");
	}
	else
	{
		paused = false;
		btnPause->setText("Pause Scrolling");
		scrollToBottom();
	}
}

void msgViewerDlg::exit() { close(); }

void msgViewerDlg::clear()
{
	int ret =
	    QMessageBox::question(this, tr("Message Viewer"), tr("Are you sure you want to clear all received messages?"),
	                          QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
	switch (ret)
	{
		case QMessageBox::Yes:
			nMsgs = 0;
			nSupMsgs = 0;
			nThrMsgs = 0;
			nDeleted = 0;
			{
				std::lock_guard<std::mutex> lk(msg_pool_mutex_);
			msg_pool_.clear();
			}
			{
				std::lock_guard<std::mutex> lk(msg_classification_mutex_);
			host_msgs_.clear();
			cat_msgs_.clear();
			app_msgs_.clear();
				updateList(lwApplication, app_msgs_);
				updateList(lwCategory, cat_msgs_);
				updateList(lwHost, host_msgs_);
			}
			for (auto& display : msgFilters_)
			{
				std::lock_guard<std::mutex> lk(filter_mutex_);
				display.txtDisplay->clear();
				display.msgs.clear();
				display.nDisplayMsgs = 0;
				display.nDisplayedDeletedMsgs = 0;
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
	auto display = tabWidget->currentIndex();
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

		default:
			setSevDebug();
	}

	displayMsgs(display);
}

void msgViewerDlg::setSevError()
{
	auto display = tabWidget->currentIndex();
	msgFilters_[display].sevThresh = SERROR;
	btnError->setChecked(true);
	btnWarning->setChecked(false);
	btnInfo->setChecked(false);
	btnDebug->setChecked(false);
	vsSeverity->setValue(SERROR);
}

void msgViewerDlg::setSevWarning()
{
	auto display = tabWidget->currentIndex();
	msgFilters_[display].sevThresh = SWARNING;
	btnError->setChecked(false);
	btnWarning->setChecked(true);
	btnInfo->setChecked(false);
	btnDebug->setChecked(false);
	vsSeverity->setValue(SWARNING);
}

void msgViewerDlg::setSevInfo()
{
	auto display = tabWidget->currentIndex();
	msgFilters_[display].sevThresh = SINFO;
	btnError->setChecked(false);
	btnWarning->setChecked(false);
	btnInfo->setChecked(true);
	btnDebug->setChecked(false);
	vsSeverity->setValue(SINFO);
}

void msgViewerDlg::setSevDebug()
{
	auto display = tabWidget->currentIndex();
	msgFilters_[display].sevThresh = SDEBUG;
	btnError->setChecked(false);
	btnWarning->setChecked(false);
	btnInfo->setChecked(false);
	btnDebug->setChecked(true);
	vsSeverity->setValue(SDEBUG);
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

	if (search.isEmpty()) return;

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
	auto sup = static_cast<suppress*>(act->data().value<void*>());
	sup->use(status);
}

void msgViewerDlg::setThrottling(QAction* act)
{
	bool status = act->isChecked();
	auto thr = static_cast<throttle*>(act->data().value<void*>());
	thr->use(status);
}

void msgViewerDlg::tabWidgetCurrentChanged(int newTab)
{
	displayMsgs(newTab);
	lcdDisplayedMsgs->display(msgFilters_[newTab].nDisplayMsgs);

	lwHost->setCurrentRow(-1, QItemSelectionModel::Clear);
	lwApplication->setCurrentRow(-1, QItemSelectionModel::Clear);
	lwCategory->setCurrentRow(-1, QItemSelectionModel::Clear);

	for (auto const& host : msgFilters_[newTab].hostFilter)
	{
		auto items = lwHost->findItems(host, Qt::MatchExactly);
		if (!items.empty())
		{
			items[0]->setSelected(true);
		}
	}
	for (auto const& app : msgFilters_[newTab].appFilter)
	{
		auto items = lwApplication->findItems(app, Qt::MatchExactly);
		if (!items.empty())
		{
			items[0]->setSelected(true);
		}
	}
	for (auto const& cat : msgFilters_[newTab].catFilter)
	{
		auto items = lwCategory->findItems(cat, Qt::MatchExactly);
		if (!items.empty())
		{
			items[0]->setSelected(true);
		}
	}

	switch (msgFilters_[newTab].sevThresh)
	{
		case SDEBUG:
			setSevDebug();
			break;
		case SINFO:
			setSevInfo();
			break;
		case SWARNING:
			setSevWarning();
			break;
		default:
			setSevError();
			break;
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

void msgViewerDlg::closeEvent(QCloseEvent* event) { event->accept(); }

QStringList msgViewerDlg::toQStringList(QList<QListWidgetItem*> in)
{
	QStringList out;

	for (auto i = 0; i < in.size(); ++i)
	{
		out << in[i]->text();
	}

	return out;
}
