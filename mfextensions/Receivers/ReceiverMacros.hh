#ifndef mfextensions_Receivers_ReceiverMacros_hh
#define mfextensions_Receivers_ReceiverMacros_hh

#include "mfextensions/Receivers/MVReceiver.hh"
#include "fhiclcpp/fwd.h"

#include <memory>

namespace mfviewer
{
	typedef std::unique_ptr<mfviewer::MVReceiver>
	(makeFunc_t)(fhicl::ParameterSet const& ps);
}

#define DEFINE_MFVIEWER_RECEIVER(klass)                          \
  extern "C"                                                     \
  std::unique_ptr<mfviewer::MVReceiver>                          \
  make(fhicl::ParameterSet const & ps) {                         \
    return std::unique_ptr<mfviewer::MVReceiver>(new klass(ps)); \
  }
#endif /* mfextensions_Receivers_RecevierMacros_h */
