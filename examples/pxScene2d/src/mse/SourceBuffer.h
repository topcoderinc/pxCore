#ifndef PXSCENE_SOURCEBUFFER_H
#define PXSCENE_SOURCEBUFFER_H

#include "MSEBaseObject.h"
#include "AudioTrack.h"
#include "TextTrack.h"
#include "VideoTrack.h"
#include "MSEBaseTrackList.h"
#include "TimeRanges.h"
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

  // SourceBuffer.idl
  //attribute AppendMode mode;
  rtProperty(mode, getMode, setMode, rtString);
  rtError getMode(rtString &v) const;
  rtError setMode(rtString const &v);

  //readonly attribute boolean updating;
  rtReadOnlyProperty(updating, getUpdating, bool);
  rtError getUpdating(bool &v) const;

  // Returns the time ranges buffered.
  //readonly attribute TimeRanges buffered;
  rtReadOnlyProperty(buffered, getBuffered, rtObjectRef);
  rtError getBuffered(rtObjectRef &v) const;

  // Applies an offset to media segment timestamps.
  //attribute double timestampOffset;
  rtReadOnlyProperty(timestampOffset, getTimestampOffset, float);
  rtError getTimestampOffset(float &v) const;

  // [Conditional=VIDEO_TRACK] readonly attribute AudioTrackList audioTracks;
  rtReadOnlyProperty(audioTracks, getAudioTracks, rtObjectRef);
  rtError getAudioTracks(rtObjectRef &v) const;

  // [Conditional=VIDEO_TRACK] readonly attribute VideoTrackList videoTracks;
  rtReadOnlyProperty(videoTracks, getVideoTracks, rtObjectRef);
  rtError getVideoTracks(rtObjectRef &v) const;

  // [Conditional=VIDEO_TRACK] readonly attribute TextTrackList textTracks;
  rtReadOnlyProperty(textTracks, getTextTracks, rtObjectRef);
  rtError getTextTracks(rtObjectRef &v) const;

  //attribute double appendWindowStart;
  rtProperty(appendWindowStart, getAppendWindowStart, setAppendWindowStart, float);
  rtError getAppendWindowStart(float &v) const;
  rtError setAppendWindowStart(float const &v);

  //attribute unrestricted double appendWindowEnd;
  rtProperty(appendWindowEnd, getAppendWindowEnd, setAppendWindowEnd, float);
  rtError getAppendWindowEnd(float &v) const;
  rtError setAppendWindowEnd(float const &v);

  // Append segment data.
  //[MayThrowException] void appendBuffer(BufferSource data);
  rtMethod1ArgAndNoReturn(appendBuffer, appendBuffer, rtObjectRef);
  rtError appendBuffer(rtObjectRef buffer);

  // Abort the current segment append sequence.
  //[MayThrowException] void abort();
  rtMethodNoArgAndNoReturn(abort, abort);
  rtError abort();

  //[MayThrowException] void remove(double start, unrestricted double end);
  rtMethod2ArgAndNoReturn(remove, remove, int32_t, int32_t);
  rtError remove(double start, int32_t end);

  //attribute EventHandler onupdatestart;
  void onUpdateStart();
  //attribute EventHandler onupdate;
  void onUpdate();
  //attribute EventHandler onupdateend;
  void onUpdateEnd();
  //attribute EventHandler onerror;
  void onError();
  //attribute EventHandler onabort;
  void onAbort();

  // [EnabledBySetting=SourceBufferChangeType, MayThrowException] void changeType(DOMString type);
  rtMethod1ArgAndNoReturn(changeType, changeType, rtString);
  rtError changeType(rtString type);

  GStreamPlayer *getGstPlayer() const;
  void setGstPlayer(GStreamPlayer *gstPlayer);

protected:
  GStreamPlayer *gstPlayer;

  float mAppendWindowEnd;
  float mAppendWindowStart;
  TimeRanges mBuffered;
  rtString mMode;
  AudioTrackList mAudioTrackList;
  TextTrackList mTextTrackList;
  VideoTrackList mVideoTrackList;
  float mTimestampOffset;
  bool mUpdating;
};


#endif //PXSCENE_SOURCEBUFFER_H
