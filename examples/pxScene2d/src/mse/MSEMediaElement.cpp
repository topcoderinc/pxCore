#include "MSEMediaElement.h"

rtDefineObject(MediaError, rtObject)

rtDefineObject(MSEMediaElement, MSEBaseObject)

MSEMediaElement::MSEMediaElement():
  mNetworkState(0),
  mReadyState(0),
  mSeeking(0),
  mCurrentTime(0),
  mDuration(0),
  mPaused(false),
  mDefaultPlaybackRate(0),
  mPlaybackRate(0),
  mEnded(false),
  mAutoplay(false),
  mLoop(false),
  mControls(false),
  mVolume(false),
  mMuted(false),
  mDefaultMuted(false)
{
}

rtError MSEMediaElement::load()
{
  return RT_OK;
}

rtError MSEMediaElement::canPlayType(const rtString &type, bool &ret)
{
  ret = false;
  return RT_OK;
}

rtError MSEMediaElement::play()
{
  return RT_OK;
}

rtError MSEMediaElement::pause()
{
  return RT_OK;
}

rtError MSEMediaElement::fastSeek(double time)
{
  return RT_OK;
}

rtDefineProperty(MSEMediaElement, error)
rtDefineProperty(MSEMediaElement, src)
rtDefineProperty(MSEMediaElement, currentSrc)
rtDefineProperty(MSEMediaElement, crossOrigin)
rtDefineProperty(MSEMediaElement, networkState)
rtDefineProperty(MSEMediaElement, preload)
rtDefineProperty(MSEMediaElement, buffered)
rtDefineProperty(MSEMediaElement, readyState)
rtDefineProperty(MSEMediaElement, seeking)
rtDefineProperty(MSEMediaElement, currentTime)
rtDefineProperty(MSEMediaElement, duration)
rtDefineProperty(MSEMediaElement, paused)
rtDefineProperty(MSEMediaElement, defaultPlaybackRate)
rtDefineProperty(MSEMediaElement, playbackRate)
rtDefineProperty(MSEMediaElement, played)
rtDefineProperty(MSEMediaElement, seekable)
rtDefineProperty(MSEMediaElement, ended)
rtDefineProperty(MSEMediaElement, autoplay)
rtDefineProperty(MSEMediaElement, loop)
rtDefineProperty(MSEMediaElement, controls)
rtDefineProperty(MSEMediaElement, volume)
rtDefineProperty(MSEMediaElement, muted)
rtDefineProperty(MSEMediaElement, defaultMuted)
rtDefineProperty(MSEMediaElement, audioTracks)
rtDefineProperty(MSEMediaElement, videoTracks)
rtDefineProperty(MSEMediaElement, textTracks)
rtDefineMethod(MSEMediaElement, load)
rtDefineMethod(MSEMediaElement, canPlayType)
rtDefineMethod(MSEMediaElement, play)
rtDefineMethod(MSEMediaElement, pause)
rtDefineMethod(MSEMediaElement, fastSeek)

rtDefineObject(MSEAudioElement, MSEMediaElement)

MSEVideoElement::MSEVideoElement() : mWidth(0), mHeight(0), mVideoWidth(0), mVideoHeight(0)
{
}

rtDefineObject(MSEVideoElement, MSEMediaElement)
rtDefineProperty(MSEVideoElement, width)
rtDefineProperty(MSEVideoElement, height)
rtDefineProperty(MSEVideoElement, videoWidth)
rtDefineProperty(MSEVideoElement, videoHeight)
rtDefineProperty(MSEVideoElement, poster)