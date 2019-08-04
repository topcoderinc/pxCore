#include "MediaElement.h"

rtDefineObject(MediaError, rtObject)

rtDefineObject(MediaElement, rtObject)

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

void MediaElement::load()
{
}

rtError MediaElement::canPlayType(const rtString &type, bool &ret)
{
  ret = false;
  return RT_OK;
}

void MediaElement::play()
{
}

void MediaElement::pause()
{
}

void MediaElement::fastSeek(double time)
{
}

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