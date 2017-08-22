#ifndef MFVIEWER_MVRECEIVER_H
#define MFVIEWER_MVRECEIVER_H

#include <string>

#include "fhiclcpp/ParameterSet.h"

#include "messagefacility/Utilities/MessageFacilityMsg.h"

#include <QtCore/QThread>
#include <iostream>

namespace mfviewer
{
	class MVReceiver : public QThread
	{
		Q_OBJECT

	public:
		explicit MVReceiver(fhicl::ParameterSet pset);

		virtual ~MVReceiver() { ; }

		void stop() { stopRequested_ = true; }
	protected:
		bool stopRequested_;
		signals :
		void NewMessage(mf::MessageFacilityMsg const&);
	};
}

#endif //MVRECEIVER_H
