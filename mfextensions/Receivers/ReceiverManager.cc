#include "mfextensions/Receivers/ReceiverManager.hh"

#include <iostream>
#include "fhiclcpp/ParameterSet.h"
#include "mfextensions/Receivers/makeMVReceiver.hh"

mfviewer::ReceiverManager::ReceiverManager(const fhicl::ParameterSet& pset)
{
	qRegisterMetaType<qt_mf_msg>("qt_mf_msg");
	qRegisterMetaType<msg_ptr_t>("msg_ptr_t");
	std::vector<std::string> names = pset.get_pset_names();
	for (const auto& name : names)
	{
		std::string pluginType = "unknown";
		try
		{
			auto plugin_pset = pset.get<fhicl::ParameterSet>(name);
			pluginType = plugin_pset.get<std::string>("receiverType", "unknown");
			std::unique_ptr<mfviewer::MVReceiver> rcvr = makeMVReceiver(pluginType, plugin_pset);
			connect(rcvr.get(), SIGNAL(NewMessage(msg_ptr_t)), this, SLOT(onNewMessage(msg_ptr_t)));
			receivers_.push_back(std::move(rcvr));
		}
		catch (...)
		{
			std::cerr << "ReceiverManager: Unable to load receiver plugin with name " << name << " and plugin type "
			          << pluginType << std::endl;
		}
	}
}

mfviewer::ReceiverManager::~ReceiverManager()
{
	stop();
	for (auto& i : receivers_)
	{
		i.reset(nullptr);
	}
}

void mfviewer::ReceiverManager::stop()
{
	for (auto& receiver : receivers_)
	{
		receiver->stop();
	}
	for (auto& receiver : receivers_)
	{
		receiver->wait();
	}
}

void mfviewer::ReceiverManager::start()
{
	for (auto& receiver : receivers_)
	{
		receiver->start();
	}
}

void mfviewer::ReceiverManager::onNewMessage(msg_ptr_t const& mfmsg) { emit newMessage(mfmsg); }
