#ifndef MFVIEWER_MVRECEIVER_H
#define MFVIEWER_MVRECEIVER_H

#include <string>

#include "fhiclcpp/ParameterSet.h"

#include "mfextensions/Receivers/qt_mf_msg.hh"

#include <QtCore/QThread>
#include <iostream>

namespace mfviewer
{
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
		explicit MVReceiver(fhicl::ParameterSet pset);

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
		bool stopRequested_;
	signals :
		/// <summary>
		/// When a message is received by the MVReceiver, this signal should be raised so that the connected listener can process it.
		/// </summary>
		/// <param name="msg">Received message</param>
		void NewMessage(qt_mf_msg const& msg);
	};
}

#endif //MVRECEIVER_H
