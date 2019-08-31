#ifndef PXSCENE_MSEMediaSource_H
#define PXSCENE_MSEMediaSource_H


#include "MSEBaseObject.h"
#include "MSESourceBufferList.h"
#include "MSEWebKitEventEmitter.h"
#include <vector>

struct MSEMediaSourceImpl;

namespace WebCore
{
  class MediaSource;
}

/**
 * Media source class
 * https://developer.mozilla.org/en-US/docs/Web/API/MSEMediaSource
 */
class MSEMediaSource : public MSEBaseObject, public MSEWebkitEventEmitter {
public:
  MSEMediaSource();
  ~MSEMediaSource();

  rtDeclareObject(MSEMediaSource, MSEBaseObject);

  //MSEMediaSource.idl
  // All the source buffers created by this object.
  //readonly attribute SourceBufferList sourceBuffers;
  rtReadOnlyProperty(sourceBuffers, getSourceBuffers, rtObjectRef);
  rtError getSourceBuffers(rtObjectRef &v) const;

  // Subset of sourceBuffers that provide data for the selected/enabled tracks.
  // readonly attribute SourceBufferList activeSourceBuffers;
  rtReadOnlyProperty(activeSourceBuffers, getActiveSourceBuffers, rtObjectRef);
  rtError getActiveSourceBuffers(rtObjectRef &v) const;

  //attribute unrestricted double duration;
  rtProperty(duration, getDuration, setDuration, double);
  rtError getDuration(double &v) const;
  rtError setDuration(double const &v);

  //[NewObject, MayThrowException] SourceBuffer addSourceBuffer(DOMString type);
  rtMethod1ArgAndReturn("addSourceBuffer", addSourceBuffer, rtString, rtObjectRef);
  rtError addSourceBuffer(rtString type, rtObjectRef &buffer);

  //[MayThrowException] void removeSourceBuffer(SourceBuffer buffer);
  rtMethod1ArgAndNoReturn("removeSourceBuffer", removeSourceBuffer, rtObjectRef);
  rtError removeSourceBuffer(rtObjectRef buffer);

  //readonly attribute ReadyState readyState;
  rtReadOnlyProperty(readyState, getReadyState, rtString);
  rtError getReadyState(rtString &v) const;

  //[MayThrowException] void endOfStream(optional EndOfStreamError error);
  rtMethod1ArgAndNoReturn("endOfStream", endOfStream, rtString);
  rtError endOfStream(rtString reason);

  //static boolean isTypeSupported (DOMString type);
  rtMethod1ArgAndReturn("isTypeSupported", isTypeSupported, rtString, bool);
  rtError isTypeSupported(rtString type, bool &ret);

  //[MayThrowException] void setLiveSeekableRange(double start, double end);
  rtMethod2ArgAndNoReturn("setLiveSeekableRange", setLiveSeekableRange, double, double);
  rtError setLiveSeekableRange(double start, double end);

  //[MayThrowException] void clearLiveSeekableRange();
  rtMethodNoArgAndNoReturn("clearLiveSeekableRange", clearLiveSeekableRange);
  rtError clearLiveSeekableRange();

  void onWebkitEvent(const std::string &name);

  //attribute EventHandler onsourceopen;
  void onSourceOpen();

  //attribute EventHandler onsourceended;
  void onSourceEnded();

  //attribute EventHandler onsourceclose;
  void onSourceClose();

  //SourceBuffer *getCurSourceBuffer() const;

  //void load(const char *src);

  //GStreamPlayer *getGstPlayer() const;

  //void setGstPlayer(GStreamPlayer *gstPlayer);

  //void onEvent(const char *event);

  WebCore::MediaSource &getWebKitMediaSource() const;

protected:
  MSEMediaSourceImpl *mMediaSourceImpl;
};


#endif //PXSCENE_MSEMediaSource_H
