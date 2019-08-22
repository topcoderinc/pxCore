

#ifndef PXSCENE_MSEBASEOBJECT_H
#define PXSCENE_MSEBASEOBJECT_H

#include "rtObject.h"

/**
 * MSE video base object, this is like event emit class
 */
class MSEBaseObject : public rtObject {
public:
  rtDeclareObject(MSEBaseObject, rtObject);

  MSEBaseObject();

  // add event
  rtMethod2ArgAndNoReturn("on", addListener, rtString, rtFunctionRef);
  rtMethod2ArgAndNoReturn("addEventListener", addListener1, rtString, rtFunctionRef);

  // remove event
  rtMethod2ArgAndNoReturn("delListener", delListener, rtString, rtFunctionRef);
  rtMethod2ArgAndNoReturn("removeEventListener", delListener1, rtString, rtFunctionRef);

  rtError addListener(rtString eventName, const rtFunctionRef &f)
  {
    return mEmit->addListener(eventName, f);
  }

  rtError addListener1(rtString eventName, const rtFunctionRef &f)
  {
    return mEmit->addListener(eventName, f);
  }

  rtError delListener(rtString eventName, const rtFunctionRef &f)
  {
    return mEmit->delListener(eventName, f);
  }

  rtError delListener1(rtString eventName, const rtFunctionRef &f)
  {
    return mEmit->delListener(eventName, f);
  }

protected:
  rtEmitRef mEmit;
};

#endif
