/*

 pxCore Copyright 2005-2018 John Robinson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

#ifndef PX_HTML_VIDEO2_H
#define PX_HTML_VIDEO2_H

#include "pxScene2d.h"
#include "pxObject.h"
#include "pxContext.h"

#include "mse/MSEUtils.h"
#include "mse/MSETimeRanges.h"
#include "mse/MSEBaseTrackList.h"
#include "mse/MSEWebKitEventEmitter.h"

#include <queue>

struct pxHtmlVideo2Impl;

class MediaError : public rtObject {
public:
  rtDeclareObject(MediaError, rtObject);

  MediaError(): mCode(0) {}

  rtReadOnlyProperty(error, getError, int);

  rtError getError(int &val) const { val = mCode; return RT_OK; }

  int mCode;
};

class pxHtmlVideo2 : public pxObject, public MSEWebkitEventEmitter {
public:
  rtDeclareObject(pxHtmlVideo2, pxObject);

  pxHtmlVideo2(pxScene2d *scene);

  virtual ~pxHtmlVideo2();

  rtMethod1ArgAndNoReturn("attachMediaSource", attachMediaSource, rtObjectRef);
  rtError attachMediaSource(rtObjectRef mediaSourceArg);

  // HTMLMediaElement.idl
  // error state
  //readonly attribute MediaError error;
  defineReadOnlyAttribute(error, &mError, rtObjectRef)

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
  defineReadOnlyAttribute(buffered, &mBuffered, rtObjectRef)

  rtMethodNoArgAndNoReturn("load", load)
  rtError load();

  //DOMString canPlayType(DOMString type);
  rtMethod1ArgAndReturn("canPlayType", canPlayType, rtString, bool)
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
  defineReadOnlyAttribute(played, &mPlayed, rtObjectRef)

  //readonly attribute TimeRanges seekable;
  defineReadOnlyAttribute(seekable, &mSeekable, rtObjectRef)

  //readonly attribute boolean ended;
  defineReadOnlyAttribute(ended, mEnded, bool)

  //[CEReactions=NotNeeded, Reflect] attribute boolean autoplay;
  defineReadWriteAttribute(autoplay, mAutoplay, bool)

  //[CEReactions=NotNeeded, Reflect] attribute boolean loop;
  defineReadWriteAttribute(loop, mLoop, bool)

  rtMethodNoArgAndNoReturn("play", play)
  rtError play();

  rtMethodNoArgAndNoReturn("pause", pause)
    rtError pause();

  rtMethod1ArgAndNoReturn("fastSeek", fastSeek, double)
    rtError fastSeek(double time);

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
  defineReadOnlyAttribute(audioTracks, &mAudioTracks, rtObjectRef)

  //[Conditional=VIDEO_TRACK, ImplementedAs=ensureVideoTracks] readonly attribute VideoTrackList videoTracks;
  defineReadOnlyAttribute(videoTracks, &mVideoTracks, rtObjectRef)

  //[Conditional=VIDEO_TRACK, ImplementedAs=ensureTextTracks] readonly attribute TextTrackList textTracks;
  defineReadOnlyAttribute(textTracks, &mTextTracks, rtObjectRef)

  //[Conditional=VIDEO_TRACK, MayThrowException] TextTrack addTextTrack(DOMString kind, optional DOMString label = "", optional DOMString language = "");

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

  rtMethod2ArgAndNoReturn("addEventListener", addListener1, rtString, rtFunctionRef);
  rtMethod2ArgAndNoReturn("removeEventListener", delListener1, rtString, rtFunctionRef);

  rtError addListener1(rtString eventName, const rtFunctionRef& f)
  {
    return mEmit->addListener(eventName, f);
  }

  rtError delListener1(rtString  eventName, const rtFunctionRef& f)
  {
    return mEmit->delListener(eventName, f);
  }

  void onWebkitEvent(const std::string &name);

  virtual void draw();

  virtual void dispose(bool pumpJavascript);

  void onNewFrame(void *buffer, int w, int h, int stride, int pixel_stride);

  static void onRepaint(void *context, void *data);

private:

  rtMutex mFrameMutex;
  std::queue<pxOffscreen *> frames;
  pxOffscreen *previousFrame;

  pxHtmlVideo2Impl *mVideoImpl = nullptr;

  // idl
  MediaError mError;
  rtString   mSrc;
  rtString   mCurrentSrc;
  rtString   mCrossOrigin;
  int        mNetworkState;
  rtString   mPreload;
  MSETimeRanges mBuffered;
  int        mReadyState;
  bool       mSeeking;
  double     mCurrentTime;
  double     mDuration;
  bool       mPaused;
  double     mDefaultPlaybackRate;
  double     mPlaybackRate;
  MSETimeRanges mPlayed;
  MSETimeRanges mSeekable;
  bool       mEnded;
  bool       mAutoplay;
  bool       mLoop;
  bool       mControls;
  bool       mVolume;
  bool       mMuted;
  bool       mDefaultMuted;
  MSEAudioTrackList mAudioTracks;
  MSEVideoTrackList mVideoTracks;
  MSETextTrackList  mTextTracks;
  int mWidth;
  int mHeight;
  int mVideoWidth;
  int mVideoHeight;
  rtString mPoster;

public:
  static pxHtmlVideo2 *pxHtmlVideoObj; //This object
};

#endif // PX_HTML_VIDEO2_H
