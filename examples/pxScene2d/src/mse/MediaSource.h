#ifndef PXSCENE_MEDIASOURCE_H
#define PXSCENE_MEDIASOURCE_H


#include "MSEBaseObject.h"
#include "SourceBufferList.h"
#include <vector>


/**
 * Media source class
 * https://developer.mozilla.org/en-US/docs/Web/API/MediaSource
 */
class MediaSource : public MSEBaseObject {
public:
  MediaSource();

  rtDeclareObject(MediaSource, MSEBaseObject);

  //MediaSource.idl
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
  rtMethod1ArgAndReturn(addSourceBuffer, addSourceBuffer, rtString, rtObjectRef);
  rtError addSourceBuffer(rtString type, rtObjectRef &buffer);

  //[MayThrowException] void removeSourceBuffer(SourceBuffer buffer);
  rtMethod1ArgAndNoReturn(removeSourceBuffer, removeSourceBuffer, rtObjectRef);
  rtError removeSourceBuffer(rtObjectRef buffer);

  //readonly attribute ReadyState readyState;
  rtReadOnlyProperty(readyState, getReadyState, rtString);
  rtError getReadyState(rtString &v) const;

  //[MayThrowException] void endOfStream(optional EndOfStreamError error);
  rtMethod1ArgAndNoReturn(endOfStream, endOfStream, rtString);
  rtError endOfStream(rtString reason);

  //static boolean isTypeSupported (DOMString type);
  rtMethod1ArgAndReturn(isTypeSupported, isTypeSupported, rtString, bool);
  rtError isTypeSupported(rtString reason, bool &ret);

  //[MayThrowException] void setLiveSeekableRange(double start, double end);
  rtMethod2ArgAndNoReturn(setLiveSeekableRange, setLiveSeekableRange, double, double);
  rtError setLiveSeekableRange(double start, double end);

  //[MayThrowException] void clearLiveSeekableRange();
  rtMethodNoArgAndNoReturn(clearLiveSeekableRange, clearLiveSeekableRange);
  rtError clearLiveSeekableRange();

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

protected:
  //GStreamPlayer *gstPlayer;
  //SourceBuffer *curSourceBuffer;

  SourceBufferList mBufferList;
  SourceBufferList mActiveBufferList;
  double mDuration;
  rtString mReadyState;
};


#endif //PXSCENE_MEDIASOURCE_H
