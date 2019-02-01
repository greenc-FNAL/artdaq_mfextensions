#ifndef mfextensions_Plugins_makeMVReceiver_hh
#define mfextensions_Plugins_makeMVReceiver_hh
// Using LibraryManager, find the correct library and return an instance
// of the specified generator.

#include "fhiclcpp/fwd.h"

#include <memory>
#include <string>

namespace mfviewer {
class MVReceiver;

std::unique_ptr<MVReceiver> makeMVReceiver(std::string const& receiver_plugin_spec, fhicl::ParameterSet const& ps);
}  // namespace mfviewer
#endif /* mfextensions_Plugins_makeMVReceiver_h */
