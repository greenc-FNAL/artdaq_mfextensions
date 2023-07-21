/**
 * @file ReceiverManager.hh
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
#ifndef MFEXTENSIONS_RECEIVERS_RECEIVERMANAGER_HH_
#define MFEXTENSIONS_RECEIVERS_RECEIVERMANAGER_HH_

#include "mfextensions/Receivers/MVReceiver.hh"

#include "fhiclcpp/fwd.h"

#include <QObject>

#include <memory>
#include <vector>

namespace mfviewer {
/// <summary>
/// The ReceiverManager loads one or more receiver plugins and displays messages received by those plugins on the
/// Message Viewer dialog
/// </summary>
class ReceiverManager : public QObject
{
	Q_OBJECT

public:
	/// <summary>
	/// ReceiverManager Constructor
	/// </summary>
	/// <param name="pset">ParameterSet used to configure ReceiverManager</param>
	explicit ReceiverManager(fhicl::ParameterSet const& pset);

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
	void newMessage(msg_ptr_t const& msg);

private slots:
	/// <summary>
	/// Slot connected to receivers' newMessage signal
	/// </summary>
	/// <param name="mfmsg">Message received by receiver</param>
	void onNewMessage(msg_ptr_t const& mfmsg);

private:
	ReceiverManager(ReceiverManager const&) = delete;
	ReceiverManager(ReceiverManager&&) = delete;
	ReceiverManager& operator=(ReceiverManager const&) = delete;
	ReceiverManager& operator=(ReceiverManager&&) = delete;

	std::vector<std::unique_ptr<mfviewer::MVReceiver>> receivers_;
};
}  // namespace mfviewer

#endif  // MFEXTENSIONS_RECEIVERS_RECEIVERMANAGER_HH_
