
#include "MSEWebKitEventEmitter.h"

#include "WebCore/config.h"
#include "MediaSource.h"
#include "HTMLVideoElement.h"
#include "WebCore/fileapi/Blob.h"
#include "WebCore/dom/Event.h"
#include "WebCore/dom/EventNames.h"


class PxCoreEventListener: public WebCore::EventListener {
public:
  PxCoreEventListener(MSEWebkitEventEmitter *emitter, const std::string &name): 
    WebCore::EventListener(WebCore::EventListener::JSEventListenerType),
    mEmitter(emitter), mEventName(name)
  {
  }

  virtual bool operator==(const WebCore::EventListener&) const { return false; }
  virtual void handleEvent(WebCore::ScriptExecutionContext&, WebCore::Event&)
  {
    mEmitter->onWebkitEvent(mEventName);
  }

  static Ref<PxCoreEventListener> create(MSEWebkitEventEmitter *emitter, const std::string &name) 
  { return adoptRef(*new PxCoreEventListener(emitter, name)); }

private:
  MSEWebkitEventEmitter *mEmitter;
  std::string            mEventName;
};

void MSEWebkitEventEmitter::addWebkitEventListener(WebCore::EventTarget *eventTarget, const std::string &name)
{
  eventTarget->addEventListener(name.c_str(), PxCoreEventListener::create(this, name));
}