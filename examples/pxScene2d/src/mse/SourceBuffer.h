#ifndef PXSCENE_SOURCEBUFFER_H
#define PXSCENE_SOURCEBUFFER_H

#include "MSEBaseObject.h"
#include "AudioTrack.h"
#include "TextTrack.h"
#include "VideoTrack.h"
#include "GStreamPlayer.h"
#include <vector>
#include <string>


/**
 * Source buffer class
 * https://developer.mozilla.org/en-US/docs/Web/API/SourceBuffer
 */
class SourceBuffer : public MSEBaseObject {

public:
  rtDeclareObject(SourceBuffer, MSEBaseObject);

  SourceBuffer();

  virtual ~SourceBuffer();

  rtProperty(appendWindowEnd, getAppendWindowEnd, setAppendWindowEnd, float);

  rtProperty(appendWindowStart, getAppendWindowStart, setAppendWindowStart, float);

  rtReadOnlyProperty(audioTracks, getAudioTracks, rtObjectRef);

  rtReadOnlyProperty(buffered, getBuffered, float);

  rtProperty(mode, getMode, setMode, rtString);

  rtReadOnlyProperty(textTracks, getTextTracks, rtObjectRef);

  rtReadOnlyProperty(timestampOffset, getTimestampOffset, float);

  rtProperty(trackDefaults, getTrackDefaults, setTrackDefaults, rtObjectRef);

  rtReadOnlyProperty(updating, getUpdating, bool);

  rtReadOnlyProperty(videoTracks, getVideoTracks, rtObjectRef);

  rtMethodNoArgAndNoReturn("abort", abort);

  rtMethod1ArgAndNoReturn("appendBuffer", appendBuffer, rtObjectRef);

  rtMethod2ArgAndNoReturn("appendStream", appendStream, rtObjectRef, int32_t);

  rtMethod1ArgAndNoReturn("changeType", changeType, rtString);

  rtMethod2ArgAndNoReturn("remove", remove, int32_t, int32_t);


  rtError appendBuffer(rtObjectRef buffer);

  rtError abort();

  rtError appendStream(rtObjectRef stream, int32_t maxSize);

  rtError changeType(rtString type);

  rtError remove(int32_t start, int32_t end);

  rtError getAppendWindowEnd(float &v) const;

  rtError setAppendWindowEnd(float const &v);

  rtError getAppendWindowStart(float &v) const;

  rtError setAppendWindowStart(float const &v);

  rtError getAudioTracks(rtObjectRef &v) const;

  rtError getBuffered(float &v) const;

  rtError getMode(rtString &v) const;

  rtError setMode(rtString const &v);

  rtError getTextTracks(rtObjectRef &v) const;

  rtError getVideoTracks(rtObjectRef &v) const;


  rtError getTrackDefaults(rtObjectRef &v) const;

  rtError setTrackDefaults(rtObjectRef const &v);

  rtError getTimestampOffset(float &v) const;

  rtError getUpdating(bool &v) const;

  GStreamPlayer *getGstPlayer() const;

  void setGstPlayer(GStreamPlayer *gstPlayer);

protected:
  GStreamPlayer *gstPlayer;

  float mAppendWindowEnd;
  float mAppendWindowStart;
  std::vector<MSEBaseObject *> mAudioTracks;
  float mBuffered;
  rtString mMode;
  std::vector<MSEBaseObject *> mTextTracks;
  float mTimestampOffset;
  std::vector<MSEBaseObject *> mTrackDefaults;
  bool mUpdating;
  std::vector<MSEBaseObject *> mVideoTracks;

};


#endif //PXSCENE_SOURCEBUFFER_H
