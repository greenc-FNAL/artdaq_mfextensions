#ifndef RECEIVER_MANAGER_H
#define RECEIVER_MANAGER_H

#include "fhiclcpp/fwd.h"
#include <QObject>
#ifdef NO_MF_UTILITIES
#include "messagefacility/MessageLogger/MessageFacilityMsg.h"
#else
#include "messagefacility/Utilities/MessageFacilityMsg.h"
#endif
#include "mfextensions/Receivers/MVReceiver.hh"

namespace mfviewer
{
	class ReceiverManager : public QObject
	{
		Q_OBJECT

	public:
		ReceiverManager(fhicl::ParameterSet pset);

		virtual ~ReceiverManager();

		void start();

		void stop();

		signals :
		void newMessage(mf::MessageFacilityMsg const&);

	private slots:
		void onNewMessage(mf::MessageFacilityMsg const& mfmsg);

	private:
		std::vector<std::unique_ptr<mfviewer::MVReceiver>> receivers_;
	};
}

#endif
