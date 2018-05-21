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
		/// <summary>
		/// ReceiverManager Constructor
		/// </summary>
		/// <param name="pset">ParameterSet used to configure ReceiverManager</param>
		explicit ReceiverManager(fhicl::ParameterSet pset);

		/// <summary>
		/// ReceiverManager Destructor
		/// </summary>
		virtual ~ReceiverManager();

		/// <summary>
		/// Start all receivers
		/// </summary>
		void start();

		/// <summary>
		/// Stop all receivers
		/// </summary>
		void stop();

	signals:
		/// <summary>
		/// Signal raised on new message
		/// </summary>
		/// <param name="msg">Message just received</param>
		void newMessage(qt_mf_msg const& msg);

		private slots:
		/// <summary>
		/// Slot connected to receivers' newMessage signal
		/// </summary>
		/// <param name="mfmsg">Message received by receiver</param>
		void onNewMessage(qt_mf_msg const& mfmsg);

	private:
		std::vector<std::unique_ptr<mfviewer::MVReceiver>> receivers_;
	};
}

#endif
