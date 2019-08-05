#include "MediaElement.h"

rtDefineObject(MediaError, rtObject)

rtDefineObject(MediaElement, MSEBaseObject)

MediaElement::MediaElement():
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

rtError MediaElement::load()
{
  return RT_OK;
}

rtError MediaElement::canPlayType(const rtString &type, bool &ret)
{
  ret = false;
  return RT_OK;
}

rtError MediaElement::play()
{
  return RT_OK;
}

rtError MediaElement::pause()
{
  return RT_OK;
}

rtError MediaElement::fastSeek(double time)
{
  return RT_OK;
}

rtDefineProperty(MediaElement, error)
rtDefineProperty(MediaElement, src)
rtDefineProperty(MediaElement, currentSrc)
rtDefineProperty(MediaElement, crossOrigin)
rtDefineProperty(MediaElement, networkState)
rtDefineProperty(MediaElement, preload)
rtDefineProperty(MediaElement, buffered)
rtDefineProperty(MediaElement, readyState)
rtDefineProperty(MediaElement, seeking)
rtDefineProperty(MediaElement, currentTime)
rtDefineProperty(MediaElement, duration)
rtDefineProperty(MediaElement, paused)
rtDefineProperty(MediaElement, defaultPlaybackRate)
rtDefineProperty(MediaElement, playbackRate)
rtDefineProperty(MediaElement, played)
rtDefineProperty(MediaElement, seekable)
rtDefineProperty(MediaElement, ended)
rtDefineProperty(MediaElement, autoplay)
rtDefineProperty(MediaElement, loop)
rtDefineProperty(MediaElement, controls)
rtDefineProperty(MediaElement, volume)
rtDefineProperty(MediaElement, muted)
rtDefineProperty(MediaElement, defaultMuted)
rtDefineProperty(MediaElement, audioTracks)
rtDefineProperty(MediaElement, videoTracks)
rtDefineProperty(MediaElement, textTracks)
rtDefineMethod(MediaElement, load)
rtDefineMethod(MediaElement, canPlayType)
rtDefineMethod(MediaElement, play)
rtDefineMethod(MediaElement, pause)
rtDefineMethod(MediaElement, fastSeek)

rtDefineObject(AudioElement, MediaElement)

VideoElement::VideoElement() : mWidth(0), mHeight(0), mVideoWidth(0), mVideoHeight(0)
{
}

rtDefineObject(VideoElement, MediaElement)
rtDefineProperty(VideoElement, width)
rtDefineProperty(VideoElement, height)
rtDefineProperty(VideoElement, videoWidth)
rtDefineProperty(VideoElement, videoHeight)
rtDefineProperty(VideoElement, poster)