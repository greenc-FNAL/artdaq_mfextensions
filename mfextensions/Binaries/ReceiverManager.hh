#ifndef RECEIVER_MANAGER_H
#define RECEIVER_MANAGER_H

#include "fhiclcpp/fwd.h"
#include <QObject>
#include "mfextensions/Receivers/MVReceiver.hh"

namespace mfviewer
{
	/// <summary>
	/// The ReceiverManager loads one or more receiver plugins and displays messages received by those plugins on the Message Viewer dialog
	/// </summary>
	class ReceiverManager : public QObject
	{
		Q_OBJECT

	public:
		ReceiverManager(fhicl::ParameterSet pset);

		virtual ~ReceiverManager();

		void start();

		void stop();

		signals :
		void newMessage(qt_mf_msg const&);

	private slots:
		void onNewMessage(qt_mf_msg const& mfmsg);

	private:
		std::vector<std::unique_ptr<mfviewer::MVReceiver>> receivers_;
	};
}

#endif
