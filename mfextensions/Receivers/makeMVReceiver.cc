#include "mfextensions/Receivers/makeMVReceiver.hh"

#include "cetlib/BasicPluginFactory.h"
#include "fhiclcpp/ParameterSet.h"
#include "mfextensions/Receivers/ReceiverMacros.hh"

std::unique_ptr<mfviewer::MVReceiver> mfviewer::makeMVReceiver(std::string const& receiver_plugin_spec,
                                                               fhicl::ParameterSet const& ps) {
  static cet::BasicPluginFactory bpf("receiver", "make");

  return bpf.makePlugin<std::unique_ptr<mfviewer::MVReceiver>, fhicl::ParameterSet const&>(receiver_plugin_spec, ps);
}
