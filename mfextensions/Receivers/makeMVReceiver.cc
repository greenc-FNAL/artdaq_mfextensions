#include "mfextensions/Receivers/makeMVReceiver.hh"

#include "mfextensions/Receivers/ReceiverMacros.hh"
#include "fhiclcpp/ParameterSet.h"
#include "cetlib/BasicPluginFactory.h"

std::unique_ptr<mfviewer::MVReceiver>
mfviewer::makeMVReceiver(std::string const & receiver_plugin_spec,
                              fhicl::ParameterSet const & ps)
{
  static cet::BasicPluginFactory bpf("receiver", "make");
  
  return bpf.makePlugin<std::unique_ptr<mfviewer::MVReceiver>,
    fhicl::ParameterSet const &>(receiver_plugin_spec, ps);
}
