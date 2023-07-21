/**
 * @file makeMVReceiver.hh
 * Provides utility functions for connecting TCP sockets
 *
 * This is part of the artdaq Framework, copyright 2023.
 * Licensing/copyright details are in the LICENSE file that you should have
 * received with this code.
 */
#ifndef MFEXTENSIONS_RECEIVERS_MAKEMVRECEIVER_HH_
#define MFEXTENSIONS_RECEIVERS_MAKEMVRECEIVER_HH_
// Using LibraryManager, find the correct library and return an instance
// of the specified generator.

#include "fhiclcpp/fwd.h"

#include <memory>
#include <string>

namespace mfviewer {
class MVReceiver;

std::unique_ptr<MVReceiver> makeMVReceiver(std::string const& receiver_plugin_spec, fhicl::ParameterSet const& ps);
}  // namespace mfviewer
#endif  // MFEXTENSIONS_RECEIVERS_MAKEMVRECEIVER_HH_
