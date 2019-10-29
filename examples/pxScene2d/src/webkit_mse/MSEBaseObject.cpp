


#include "MSEBaseObject.h"

namespace WebCore {
  MSEBaseObject::MSEBaseObject() : mEmit(new rtEmit)
  {
  }

  rtDefineObject(MSEBaseObject, rtObject)
  rtDefineMethod(MSEBaseObject, addListener)
  rtDefineMethod(MSEBaseObject, delListener)
  rtDefineMethod(MSEBaseObject, clearListeners)
}