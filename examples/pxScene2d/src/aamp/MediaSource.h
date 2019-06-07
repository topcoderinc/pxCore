#ifndef PXSCENE_MEDIASOURCE_H
#define PXSCENE_MEDIASOURCE_H


#include "AAMPBaseObject.h"
#include "SourceBufferList.h"
#include <vector>


/**
 * Media source class
 * https://developer.mozilla.org/en-US/docs/Web/API/MediaSource
 */
class MediaSource : public AAMPBaseObject {
public:
  MediaSource();

  rtDeclareObject(MediaSource, AAMPBaseObject);

  rtReadOnlyProperty(sourceBuffers, getSourceBuffers, rtObjectRef);

  rtReadOnlyProperty(activeSourceBuffers, getActiveSourceBuffers, rtObjectRef);

  rtReadOnlyProperty(readyState, getReadyState, int32_t);

  rtProperty(duration, getDuration, setDuration, double);

  rtMethod1ArgAndNoReturn("addSourceBuffer", addSourceBuffer, rtObjectRef);

  rtMethodNoArgAndNoReturn("clearLiveSeekableRange", clearLiveSeekableRange);

  rtMethod1ArgAndNoReturn("endOfStream", endOfStream, rtString);

  rtMethod1ArgAndNoReturn("removeSourceBuffer", removeSourceBuffer, rtObjectRef);

  rtMethod2ArgAndNoReturn("setLiveSeekableRange", setLiveSeekableRange, int32_t, int32_t);

  rtError getSourceBuffers(rtObjectRef &v) const;

  rtError getActiveSourceBuffers(rtObjectRef &v) const;

  rtError getReadyState(int32_t &v) const;

  rtError getDuration(double &v) const;

  rtError setDuration(double const &v);

  rtError addSourceBuffer(rtObjectRef buffer);

  rtError clearLiveSeekableRange();

  rtError endOfStream(rtString reason);

  rtError removeSourceBuffer(rtObjectRef buffer);

  rtError setLiveSeekableRange(int32_t start, int32_t end);


  SourceBuffer *getCurSourceBuffer() const;

  void load(const char *src);

  PlayerInstanceAAMP *getInstanceAamp() const;

  void setInstanceAamp(PlayerInstanceAAMP *instanceAamp);

protected:
  PlayerInstanceAAMP *instanceAamp;
  SourceBufferList *bufferList;
  SourceBuffer *curSourceBuffer;
};


#endif //PXSCENE_MEDIASOURCE_H
