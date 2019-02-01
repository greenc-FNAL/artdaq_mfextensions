#ifndef mfextensions_Receivers_ReceiverMacros_hh
#define mfextensions_Receivers_ReceiverMacros_hh

#include "fhiclcpp/fwd.h"
#include "mfextensions/Receivers/MVReceiver.hh"

#include <memory>
#include "cetlib/compiler_macros.h"

namespace mfviewer {
/**
 * \brief Constructs a MVReceiver instance and returns a pointer to it
 * \param ps Parameter set for initializing the CommandableFragmentGenerator
 * \return A smart pointer to the CommandableFragmentGenerator
 */
typedef std::unique_ptr<mfviewer::MVReceiver> makeFunc_t(fhicl::ParameterSet const& ps);
}  // namespace mfviewer

#ifndef EXTERN_C_FUNC_DECLARE_START
#define EXTERN_C_FUNC_DECLARE_START extern "C" {
#endif

#define DEFINE_MFVIEWER_RECEIVER(klass)                                       \
  EXTERN_C_FUNC_DECLARE_START                                                 \
  std::unique_ptr<mfviewer::MVReceiver> make(fhicl::ParameterSet const& ps) { \
    return std::unique_ptr<mfviewer::MVReceiver>(new klass(ps));              \
  }                                                                           \
  }
#endif /* mfextensions_Receivers_RecevierMacros_h */
