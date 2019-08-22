

#include "MSEBaseObject.h"

MSEBaseObject::MSEBaseObject() : mEmit(new rtEmit)
{
}

rtDefineObject(MSEBaseObject, rtObject)
rtDefineMethod(MSEBaseObject, addListener)
rtDefineMethod(MSEBaseObject, addListener1)
rtDefineMethod(MSEBaseObject, delListener)
rtDefineMethod(MSEBaseObject, delListener1)
