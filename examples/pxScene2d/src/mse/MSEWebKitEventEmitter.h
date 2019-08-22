
#ifndef PXSCENE_MSEWEBKITEVENTEMITTER_H
#define PXSCENE_MSEWEBKITEVENTEMITTER_H

#include "rtObject.h"
#include <vector>

namespace WebCore
{
  class EventTarget;
}

class MSEWebkitEventEmitter {
public:
  void addWebkitEventListener(WebCore::EventTarget *eventTarget, const std::string &name);
  virtual void onWebkitEvent(const std::string &name) = 0;
};

#endif
