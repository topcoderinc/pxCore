#ifndef PXSCENE_MSESourceBuffer_H
#define PXSCENE_MSESourceBuffer_H

#include "MSEBaseObject.h"
#include "MSEAudioTrack.h"
#include "MSETextTrack.h"
#include "MSEVideoTrack.h"
#include "MSEBaseTrackList.h"
#include "MSETimeRanges.h"
#include "MSEWebKitEventEmitter.h"
#include <vector>
#include <string>

namespace WebCore
{
class MediaSource;
class SourceBuffer;
}

struct MSESourceBufferImpl;

/**
 * Source buffer class
 * https://developer.mozilla.org/en-US/docs/Web/API/SourceBuffer
 */
class MSESourceBuffer : public MSEBaseObject, public MSEWebkitEventEmitter {

public:
  rtDeclareObject(MSESourceBuffer, MSEBaseObject);

  MSESourceBuffer(WebCore::MediaSource &mediaSource, WebCore::SourceBuffer &sourceBuffer);
  MSESourceBuffer(WebCore::MediaSource &mediaSource, rtString type);

  virtual ~MSESourceBuffer();

  // MSESourceBuffer.idl
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
  rtProperty(timestampOffset, getTimestampOffset, setTimestampOffset, float);
  rtError getTimestampOffset(float &v) const;
  rtError setTimestampOffset(const float &v);

  // [Conditional=VIDEO_TRACK] readonly attribute MSEAudioTrackList audioTracks;
  rtReadOnlyProperty(audioTracks, getAudioTracks, rtObjectRef);
  rtError getAudioTracks(rtObjectRef &v) const;

  // [Conditional=VIDEO_TRACK] readonly attribute MSEVideoTrackList videoTracks;
  rtReadOnlyProperty(videoTracks, getVideoTracks, rtObjectRef);
  rtError getVideoTracks(rtObjectRef &v) const;

  // [Conditional=VIDEO_TRACK] readonly attribute MSETextTrackList textTracks;
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
  rtMethod1ArgAndNoReturn("appendBuffer", appendBuffer, rtBuffer);
  rtError appendBuffer(const rtBuffer &buffer);

  // Abort the current segment append sequence.
  //[MayThrowException] void abort();
  rtMethodNoArgAndNoReturn("abort", abort);
  rtError abort();

  //[MayThrowException] void remove(double start, unrestricted double end);
  rtMethod2ArgAndNoReturn("remove", remove, double, double);
  rtError remove(double start, double end);

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

  void onWebkitEvent(const std::string &name);

  // [EnabledBySetting=MSESourceBufferChangeType, MayThrowException] void changeType(DOMString type);
  rtMethod1ArgAndNoReturn("changeType", changeType, rtString);
  rtError changeType(const rtString &type);

  WebCore::SourceBuffer &getWebkitSourceBuffer();

protected:
  float mAppendWindowEnd;
  float mAppendWindowStart;
  MSETimeRanges mBuffered;
  rtString mMode;
  MSEAudioTrackList mAudioTrackList;
  MSETextTrackList mTextTrackList;
  MSEVideoTrackList mVideoTrackList;
  MSESourceBufferImpl *mSourceBufferImpl;
};


#endif //PXSCENE_MSESourceBuffer_H
