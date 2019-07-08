

#include "AAMPBaseObject.h"

AAMPBaseObject::AAMPBaseObject() : mEmit(new rtEmit)
{
}

rtDefineObject(AAMPBaseObject, rtObject)
rtDefineMethod(AAMPBaseObject, addListener)
rtDefineMethod(AAMPBaseObject, delListener)