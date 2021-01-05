#ifndef MSGVIEWERDLG_H
#define MSGVIEWERDLG_H

#include "mfextensions/Extensions/suppress.hh"
#include "mfextensions/Extensions/throttle.hh"
#include "mfextensions/Receivers/ReceiverManager.hh"
#include "mfextensions/Receivers/qt_mf_msg.hh"
#include "ui_msgviewerdlgui.h"

#include <QtCore/QMutex>
#include <QtCore/QTimer>

#include <boost/regex.hpp>

#include <list>
#include <map>
#include <string>
#include <vector>

namespace fhicl {
class ParameterSet;
}

/// <summary>
/// Message Viewer Dialog Window
/// </summary>
class msgViewerDlg : public QDialog, private Ui::MsgViewerDlg
{
	Q_OBJECT

public:
	/**
   * \brief Message Viewer Dialog Constructor
   * \param conf Configuration filename (fhicl document)
   * \param parent Parent Qt window
   */
	msgViewerDlg(std::string const& conf, QDialog* parent = nullptr);

	virtual ~msgViewerDlg();

public slots:

	/// Pause message receiving
	void pause();

	/// Exit the program
	void exit();

	/// Clear the message buffer
	void clear();

	/// Switch to/from Short message mode
	void shortMode();

	/// Change the severity threshold
	void changeSeverity(int sev);

protected:
	/**
   * \brief Perform actions on window close
   * \param event QCloseEvent data
   */
	void closeEvent(QCloseEvent* event);

private slots:

	void onNewMsg(msg_ptr_t const& mfmsg);

	void setFilter();

	void renderMode();

	void setSevError();

	void setSevWarning();

	void setSevInfo();

	void setSevDebug();

	void searchMsg();

	void searchClear();

	void setSuppression(QAction* act);

	void setThrottling(QAction* act);

	void tabWidgetCurrentChanged(int newTab);

	void tabCloseRequested(int tabIndex);

	void scrollToBottom();

	//---------------------------------------------------------------------------

private:
	msgViewerDlg(msgViewerDlg const&) = delete;
	msgViewerDlg(msgViewerDlg&&) = delete;
	msgViewerDlg& operator=(msgViewerDlg const&) = delete;
	msgViewerDlg& operator=(msgViewerDlg&&) = delete;

	// Display all messages stored in the buffer
	void displayMsgs(int display);

	void UpdateTextAreaDisplay(QStringList const& texts, QPlainTextEdit* widget);

	void updateDisplays();

	void trim_msg_pool();

	// test if the message is suppressed or throttled
	bool msg_throttled(msg_ptr_t const& mfmsg);

	void update_index(msg_ptr_t const& msg);

	// Update the list. Returns true if there's a change in the selection
	// before and after the update. e.g., the selected entry has been deleted
	// during the process of updateMap(); otherwise it returns a false.
	bool updateList(QListWidget* lw, msgs_map_t const& map);

	void displayMsg(msg_ptr_t const& msg, int display);

	void readSettings();

	void writeSettings();

	void parseConf(fhicl::ParameterSet const& conf);

	QStringList toQStringList(QList<QListWidgetItem*> in);

	msgs_t list_intersect(msgs_t const& l1, msgs_t const& l2);

	//---------------------------------------------------------------------------

private:
	bool paused;
	bool shortMode_;

	// # of received messages
	int nMsgs;
	int nSupMsgs;  // suppressed msgs
	int nThrMsgs;  // throttled msgs
	int nFilters;
	size_t maxMsgs;         // Maximum number of messages to store
	size_t maxDeletedMsgs;  // Maximum number of deleted messages to display
	int nDeleted;

	// Rendering messages in speed mode or full mode
	bool simpleRender;

	// suppression regex
	std::vector<suppress> e_sup_host;
	std::vector<suppress> e_sup_app;
	std::vector<suppress> e_sup_cat;

	// throttling regex
	std::vector<throttle> e_thr_host;
	std::vector<throttle> e_thr_app;
	std::vector<throttle> e_thr_cat;

	// search string
	QString searchStr;

	// msg pool storing the formatted text body
	mutable std::mutex msg_pool_mutex_;
	msgs_t msg_pool_;

	// map of a key to a list of msg iters
	mutable std::mutex msg_classification_mutex_;
	msgs_map_t host_msgs_;
	msgs_map_t cat_msgs_;
	msgs_map_t app_msgs_;

	// context menu for "suppression" and "throttling" button
	QMenu* sup_menu;
	QMenu* thr_menu;

	// Receiver Plugin Manager
	mfviewer::ReceiverManager receivers_;

	mutable std::mutex filter_mutex_;
	struct MsgFilterDisplay
	{
		int nDisplayMsgs;
		int nDisplayedDeletedMsgs;
		msgs_t msgs;
		QStringList hostFilter;
		QStringList appFilter;
		QStringList catFilter;
		QString filterExpression;
		QPlainTextEdit* txtDisplay;

		// severity threshold
		sev_code_t sevThresh;
	};
	std::vector<MsgFilterDisplay> msgFilters_;
};

enum list_mask_t
{
	LIST_APP = 0x01,
	LIST_CAT = 0x02,
	LIST_HOST = 0x04
};

#endif
