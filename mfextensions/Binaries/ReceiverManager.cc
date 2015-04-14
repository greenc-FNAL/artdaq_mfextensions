#include "ReceiverManager.h"

#include "mfextensions/Receivers/makeMVReceiver.h"
#include <fhiclcpp/ParameterSet.h>
#include <iostream>

mfviewer::ReceiverManager::ReceiverManager(fhicl::ParameterSet pset)
{
  std::vector<std::string> names = pset.get_pset_keys();
  for(auto name : names)
    {
      try {
	fhicl::ParameterSet plugin_pset = pset.get<fhicl::ParameterSet>(name);
	std::unique_ptr<mfviewer::MVReceiver> rcvr = makeMVReceiver(plugin_pset.get<std::string>("receiverType",""), plugin_pset);
        connect(rcvr.get(), SIGNAL(newMessage(mf::MessageFacilityMsg const &)), 
                this, SLOT(onNewMessage(mf::MessageFacilityMsg const &)));
        connect(rcvr.get(), SIGNAL(newSysMessage(mfviewer::SysMsgCode, QString const & )), 
                this, SLOT(onNewSysMessage(mfviewer::SysMsgCode, QString const & )));
        receivers_.push_back(std::move(rcvr));
        
      }
      catch(...) {
	std::cerr << "ReceiverManager: Unable to load receiver plugin with name " << name << std::endl;
      }
    }
}

mfviewer::ReceiverManager::~ReceiverManager()
{
  stop();
  for(auto & i : receivers_)
    i.reset(nullptr);
}

void mfviewer::ReceiverManager::stop()
{
  for(auto & receiver : receivers_)
    {
      receiver->stop();
    }
}

void mfviewer::ReceiverManager::start()
{
  for(auto & receiver : receivers_)
    {
      receiver->start();
    }
}

void mfviewer::ReceiverManager::onNewMessage(mf::MessageFacilityMsg const & mfmsg)
{
  emit newMessage(mfmsg);
}

void mfviewer::ReceiverManager::onNewSysMessage(mfviewer::SysMsgCode code, QString const & msg)
{
  emit newSysMessage(code, msg);
}
