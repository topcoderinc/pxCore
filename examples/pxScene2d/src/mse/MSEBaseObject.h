

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

  // remove event
  rtMethod2ArgAndNoReturn("delListener", delListener, rtString, rtFunctionRef);

  // clear event
  rtMethod1ArgAndNoReturn("clearListeners", clearListeners, rtString);

  rtError addListener(rtString eventName, const rtFunctionRef &f)
  {
    return mEmit->addListener(eventName, f);
  }

  rtError delListener(rtString eventName, const rtFunctionRef &f)
  {
    return mEmit->delListener(eventName, f);
  }

  rtError clearListeners(rtString eventName)
  {
    return mEmit->clearListeners(eventName);
  }

protected:
  rtEmitRef mEmit;
};


#endif
