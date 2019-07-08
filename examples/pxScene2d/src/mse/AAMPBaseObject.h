

#ifndef PXSCENE_AAMPBASEOBJECT_H
#define PXSCENE_AAMPBASEOBJECT_H

#include "rtObject.h"

/**
 * AAMP video base object, this is like event emit class
 */
class AAMPBaseObject : public rtObject {
public:
  rtDeclareObject(AAMPBaseObject, rtObject);

  AAMPBaseObject();

  // add event
  rtMethod2ArgAndNoReturn("on", addListener, rtString, rtFunctionRef);

  // remove event
  rtMethod2ArgAndNoReturn("delListener", delListener, rtString, rtFunctionRef);

  rtError addListener(rtString eventName, const rtFunctionRef &f)
  {
    return mEmit->addListener(eventName, f);
  }

  rtError delListener(rtString eventName, const rtFunctionRef &f)
  {
    return mEmit->delListener(eventName, f);
  }

protected:
  rtEmitRef mEmit;
};


#endif
