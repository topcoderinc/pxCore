

#ifndef PXSCENE_MEDIA_ELEMENT_H
#define PXSCENE_MEDIA_ELEMENT_H

#include "rtObject.h"
#include "MSEUtils.h"
#include "TimeRanges.h"

class MediaError : public rtObject {
public:
  rtDeclareObject(MediaError, rtObject);

  MediaError(): mCode(0) {}

  rtReadOnlyProperty(error, getError, int);

  rtError getError(rtValue &val) const { val = mCode; return RT_OK; }

  int mCode;
};

class MediaElement : public rtObject {
public:

  rtDeclareObject(MediaElement, rtObject);

  MediaElement();

  // HTMLMediaElement.idl
  // error state
  //readonly attribute MediaError error;
  defineReadOnlyAttribute(error, mError, rtObjectRef)

  // network state
  //[CEReactions=NotNeeded, Reflect, URL] attribute USVString src;
  defineReadWriteAttribute(src, mSrc, rtString)

  //attribute MediaProvider? srcObject;

  //[URL] readonly attribute USVString currentSrc;
  defineReadOnlyAttribute(currentSrc, mCurrentSrc, rtString)

  //[CEReactions=NotNeeded] attribute DOMString? crossOrigin;
  defineReadWriteAttribute(crossOrigin, mCrossOrigin, rtString)

  //const unsigned short NETWORK_EMPTY = 0;
  //const unsigned short NETWORK_IDLE = 1;
  //const unsigned short NETWORK_LOADING = 2;
  //const unsigned short NETWORK_NO_SOURCE = 3;
  //readonly attribute unsigned short networkState;
  defineReadOnlyAttribute(networkState, mNetworkState, int)

  //[CEReactions=NotNeeded] attribute DOMString preload;
  defineReadWriteAttribute(preload, mPreload, rtString)

  //readonly attribute TimeRanges buffered;
  defineReadWriteAttribute(buffered, mBuffered, TimeRanges)

  rtMethodNoArgAndNoReturn(load, load)
  void load();

  //DOMString canPlayType(DOMString type);
  rtMethod1ArgAndReturn(canPlayType, canPlayType, rtString, bool)
  rtError canPlayType(const rtString &type, bool &ret);

  // ready state
  //const unsigned short HAVE_NOTHING = 0;
  //const unsigned short HAVE_METADATA = 1;
  //const unsigned short HAVE_CURRENT_DATA = 2;
  //const unsigned short HAVE_FUTURE_DATA = 3;
  //const unsigned short HAVE_ENOUGH_DATA = 4;
  //readonly attribute unsigned short readyState;
  defineReadOnlyAttribute(readyState, mReadyState, int)

  //readonly attribute boolean seeking;
  defineReadWriteAttribute(seeking, mSeeking, bool)

  // playback state
  //[ImplementedAs=currentTimeForBindings] attribute unrestricted double currentTime;
  defineReadWriteAttribute(currentTime, mCurrentTime, double)

  //readonly attribute unrestricted double duration;
  defineReadOnlyAttribute(duration, mDuration, double)

  //Date getStartDate();

  //readonly attribute boolean paused;
  defineReadOnlyAttribute(paused, mPaused, bool)

  //attribute unrestricted double defaultPlaybackRate;
  defineReadWriteAttribute(defaultPlaybackRate, mDefaultPlaybackRate, double)

  //attribute unrestricted double playbackRate;
  defineReadWriteAttribute(playbackRate, mPlaybackRate, double)

  //readonly attribute TimeRanges played;
  defineReadOnlyAttribute(played, mPlayed, TimeRanges)

  //readonly attribute TimeRanges seekable;
  defineReadOnlyAttribute(seekable, mSeekable, TimeRanges)

  //readonly attribute boolean ended;
  defineReadOnlyAttribute(ended, mEnded, bool)

  //[CEReactions=NotNeeded, Reflect] attribute boolean autoplay;
  defineReadWriteAttribute(autoplay, mAutoplay, bool)

  //[CEReactions=NotNeeded, Reflect] attribute boolean loop;
  defineReadWriteAttribute(loop, mLoop, bool)

  rtMethodNoArgAndNoReturn(play, play)
  void play();

  rtMethodNoArgAndNoReturn(pause, pause)
  void pause();

  rtMethod1ArgAndNoReturn(fastSeek, fastSeek, double)
  void fastSeek(double time);

  // controls
  //[CEReactions=NotNeeded] attribute boolean controls;
  defineReadWriteAttribute(controls, mControls, bool)

  //attribute double volume;
  defineReadWriteAttribute(volume, mVolume, double)

  //attribute boolean muted;
  defineReadWriteAttribute(muted, mMuted, bool)

  //[CEReactions=NotNeeded, Reflect=muted] attribute boolean defaultMuted;
  defineReadWriteAttribute(defaultMuted, mDefaultMuted, bool)

  //[Conditional=VIDEO_TRACK, ImplementedAs=ensureAudioTracks] readonly attribute AudioTrackList audioTracks;
  defineReadOnlyAttribute(audioTracks, mAudioTracks, AudioTrackList)

  //[Conditional=VIDEO_TRACK, ImplementedAs=ensureVideoTracks] readonly attribute VideoTrackList videoTracks;
  defineReadOnlyAttribute(videoTracks, mVideoTracks, VideoTrackList)

  //[Conditional=VIDEO_TRACK, ImplementedAs=ensureTextTracks] readonly attribute TextTrackList textTracks;
  defineReadOnlyAttribute(textTracks, mTextTracks, TextTrackList)

  //[Conditional=VIDEO_TRACK, MayThrowException] TextTrack addTextTrack(DOMString kind, optional DOMString label = "", optional DOMString language = "");

  defineEventDefinition(loadstart)
  defineEventDefinition(progress)
  defineEventDefinition(suspend)
  defineEventDefinition(abort)
  defineEventDefinition(error)
  defineEventDefinition(emptied)
  defineEventDefinition(stalled)
  defineEventDefinition(loadedmetadata)
  defineEventDefinition(loadeddata)
  defineEventDefinition(canplay)
  defineEventDefinition(canplaythrough)
  defineEventDefinition(playing)
  defineEventDefinition(waiting)
  defineEventDefinition(seeking)
  defineEventDefinition(seeked)
  defineEventDefinition(ended)
  defineEventDefinition(durationchange)
  defineEventDefinition(timeupdate)
  defineEventDefinition(play)
  defineEventDefinition(pause)
  defineEventDefinition(ratechange)
  defineEventDefinition(resize)
  defineEventDefinition(volumechange)

protected:
  MediaError mError;
  rtString   mSrc;
  rtString   mCurrentSrc;
  int        mNetworkState;
  rtString   mPreload;
  TimeRanges mBuffered;
  int        mReadyState;
  bool       mSeeking;
  double     mCurrentTime;
  double     mDuration;
  bool       mPaused;
  double     mDefaultPlaybackRate;
  double     mPlaybackRate;
  TimeRanges mPlayed;
  TimeRanges mSeekable;
  bool       mEnded;
  bool       mAutoplay;
  bool       mLoop;
  bool       mControls;
  bool       mVolume;
  bool       mMuted;
  bool       mDefaultMuted;
  AudioTrackList mAudioTracks;
  VideoTrackList mVideoTracks;
  TextTrackList  mTextTracks;
};

class AudioElement: public MediaElement {
public:
  rtDeclareObject(AudioElement, MediaElement);
};

class VideoElement: public MediaElement {
public:
  rtDeclareObject(VideoElement, MediaElement);

  VideoElement();

  //HTMLVideoElement.idl
  //[CEReactions=NotNeeded, Reflect] attribute unsigned long width;
  rtProperty(width, getWidth, setWidth, int);
  rtError getWidth(int &v) const { v = mWidth; return RT_OK; }
  rtError setWidth(int v) { mWidth = v; return RT_OK; }

  //[CEReactions=NotNeeded, Reflect] attribute unsigned long height;
  rtProperty(height, getHeight, setHeight, int);
  rtError getHeight(int &v) const { v = mHeight; return RT_OK; }
  rtError setHeight(int v) { mHeight = v; return RT_OK; }

  //readonly attribute unsigned long videoWidth;
  rtReadOnlyProperty(videoWidth, getVideoWidth, int);
  rtError getVideoWidth(int &v) const { v = mVideoWidth; return RT_OK; }

  //readonly attribute unsigned long videoHeight;
  rtReadOnlyProperty(videoHeight, getVideoHeight, int);
  rtError getVideoHeight(int &v) const { v = mVideoHeight; return RT_OK; }

  //[CEReactions=NotNeeded, Reflect, URL] attribute USVString poster;
  rtProperty(poster, getPoster, setPoster, rtString);
  rtError getPoster(rtString &v) const { v = mPoster; return RT_OK; }
  rtError setPoster(const rtString &v) { mPoster = v; return RT_OK; }

private:
  int mWidth;
  int mHeight;
  int mVideoWidth;
  int mVideoHeight;
  rtString mPoster;
};


#endif
