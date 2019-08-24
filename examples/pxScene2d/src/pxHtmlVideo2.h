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

namespace WebCore 
{
class HTMLVideoElement;
}

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

  rtMethodNoArgAndNoReturn("detachMediaSource", detachMediaSource);
  rtError detachMediaSource();

  rtReadOnlyProperty(nodeName, getNodeName, rtString);
  rtError getNodeName(rtString &v) const;

  // HTMLMediaElement.idl
  // error state
  //readonly attribute MediaError error;
  //defineReadOnlyAttribute(error, &mError, rtObjectRef)
  rtReadOnlyProperty(error, getError, rtObjectRef);
  rtError getError(rtObjectRef &v) const;

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
  //defineReadWriteAttribute(preload, mPreload, rtString)
  rtProperty(preload, getPreload, setPreload, rtString);
  rtError getPreload(rtString &v) const;
  rtError setPreload(rtString const &v);

  //readonly attribute TimeRanges buffered;
  //defineReadOnlyAttribute(buffered, &mBuffered, rtObjectRef)
  rtReadOnlyProperty(buffered, getBuffered, rtObjectRef);
  rtError getBuffered(rtObjectRef &v) const;

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
  /*defineReadOnlyAttribute(readyState, mReadyState, int)*/
  rtReadOnlyProperty(readyState, getReadyState, int);
  rtError getReadyState(int &v) const;

  //readonly attribute boolean seeking;
  //defineReadWriteAttribute(seeking, mSeeking, bool)
  rtReadOnlyProperty(seeking, getSeeking, bool);
  rtError getSeeking(bool &v) const;

  // playback state
  //[ImplementedAs=currentTimeForBindings] attribute unrestricted double currentTime;
  //defineReadWriteAttribute(currentTime, mCurrentTime, double)
  rtProperty(currentTime, getCurrentTime, setCurrentTime, double);
  rtError getCurrentTime(double &v) const;
  rtError setCurrentTime(double const &v);

  //readonly attribute unrestricted double duration;
  //defineReadOnlyAttribute(duration, mDuration, double)
  rtReadOnlyProperty(duration, getDuration, double);
  rtError getDuration(double &v) const;

  //Date getStartDate();

  //readonly attribute boolean paused;
  //defineReadOnlyAttribute(paused, mPaused, bool)
  rtReadOnlyProperty(paused, getPaused, bool);
  rtError getPaused(bool &v) const;

  //attribute unrestricted double defaultPlaybackRate;
  defineReadWriteAttribute(defaultPlaybackRate, mDefaultPlaybackRate, double)

  //attribute unrestricted double playbackRate;
  //defineReadWriteAttribute(playbackRate, mPlaybackRate, double)
  rtProperty(playbackRate, getPlaybackRate, setPlaybackRate, double);
  rtError getPlaybackRate(double &v) const;
  rtError setPlaybackRate(double const &v);

  //readonly attribute TimeRanges played;
  //defineReadOnlyAttribute(played, &mPlayed, rtObjectRef)
  rtReadOnlyProperty(played, getPlayed, rtObjectRef);
  rtError getPlayed(rtObjectRef &v) const;

  //readonly attribute TimeRanges seekable;
  //defineReadOnlyAttribute(seekable, &mSeekable, rtObjectRef)
  rtReadOnlyProperty(seekable, getSeekable, rtObjectRef);
  rtError getSeekable(rtObjectRef &v) const;

  //readonly attribute boolean ended;
  defineReadOnlyAttribute(ended, mEnded, bool)

  //[CEReactions=NotNeeded, Reflect] attribute boolean autoplay;
  //defineReadWriteAttribute(autoplay, mAutoplay, bool)
  rtProperty(autoplay, getAutoplay, setAutoplay, bool);
  rtError getAutoplay(bool &v) const;
  rtError setAutoplay(bool const &v);

  //[CEReactions=NotNeeded, Reflect] attribute boolean loop;
  //defineReadWriteAttribute(loop, mLoop, bool)
  rtProperty(loop, getLoop, setLoop, bool);
  rtError getLoop(bool &v) const;
  rtError setLoop(bool const &v);

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
  //defineReadWriteAttribute(volume, mVolume, double)
  rtProperty(volume, getVolume, setVolume, double);
  rtError getVolume(double &v) const;
  rtError setVolume(double const &v);

  //attribute boolean muted;
  //defineReadWriteAttribute(muted, mMuted, bool)
  rtProperty(muted, getMuted, setMuted, bool);
  rtError getMuted(bool &v) const;
  rtError setMuted(bool const &v);

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
  rtError getWidth(int &v) const;
  rtError setWidth(int v);

  //[CEReactions=NotNeeded, Reflect] attribute unsigned long height;
  rtProperty(height, getHeight, setHeight, int);
  rtError getHeight(int &v) const;
  rtError setHeight(int v);

  //readonly attribute unsigned long videoWidth;
  rtReadOnlyProperty(videoWidth, getVideoWidth, int);
  rtError getVideoWidth(int &v) const;

  //readonly attribute unsigned long videoHeight;
  rtReadOnlyProperty(videoHeight, getVideoHeight, int);
  rtError getVideoHeight(int &v) const;

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

  WebCore::HTMLVideoElement &getWebKitVideoElement();

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
