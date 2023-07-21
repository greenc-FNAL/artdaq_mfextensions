/**
 * @file MVReceiver.hh
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
#ifndef MFEXTENSIONS_RECEIVERS_MVRECEIVER_HH_
#define MFEXTENSIONS_RECEIVERS_MVRECEIVER_HH_

#include <string>

#include "fhiclcpp/ParameterSet.h"

#include "mfextensions/Receivers/qt_mf_msg.hh"

#include <QtCore/QThread>
#include <iostream>

namespace mfviewer {
/// <summary>
/// A MVReceiver class listens for messages and raises a signal when one arrives
/// </summary>
class MVReceiver : public QThread
{
	Q_OBJECT

public:
	/// <summary>
	/// Construct a MVReceiver using the given ParameterSet
	/// </summary>
	/// <param name="pset">ParameterSet used to construct MVReceiver</param>
	explicit MVReceiver(fhicl::ParameterSet const& pset);

	/// <summary>
	/// MVReceiver destructor
	/// </summary>
	virtual ~MVReceiver() {}

	/// <summary>
	/// Stop the MVReceiver thread
	/// </summary>
	void stop() { stopRequested_ = true; }

protected:
	/// <summary>
	/// Whether the MVRecevier should stop
	/// </summary>
	std::atomic<bool> stopRequested_;
signals:
	/// <summary>
	/// When a message is received by the MVReceiver, this signal should be raised so that the connected listener can
	/// process it.
	/// </summary>
	/// <param name="msg">Received message</param>
	void NewMessage(msg_ptr_t const& msg);

private:
	MVReceiver(MVReceiver const&) = delete;
	MVReceiver(MVReceiver&&) = delete;
	MVReceiver& operator=(MVReceiver const&) = delete;
	MVReceiver& operator=(MVReceiver&&) = delete;
};
}  // namespace mfviewer

#endif  // MFEXTENSIONS_RECEIVERS_MVRECEIVER_HH_
