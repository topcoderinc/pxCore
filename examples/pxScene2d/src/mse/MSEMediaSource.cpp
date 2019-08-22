#include "MSEWebKitDocument.h"
#include "MSEMediaSource.h"
#include "MSESourceBuffer.h"
#include "MSEWebKitEventEmitter.h"
#include "MSEUtils.h"

#include "pxOffscreen.h"

#include <gst/gst.h>
#include <gst/video/video.h>

// webkit-includes
#include "WebCore/config.h"
#include "MediaSource.h"
#include "HTMLVideoElement.h"
#include "WebCore/fileapi/Blob.h"
#include "WebCore/dom/Event.h"
#include "WebCore/dom/EventNames.h"


struct MSEMediaSourceImpl {
  MSEMediaSourceImpl(WebCore::MediaSource &mediaSource)
    : mMediaSource(mediaSource)
  {
  }

  Ref<WebCore::MediaSource> mMediaSource;
};


MSEMediaSource::MSEMediaSource(): mMediaSourceImpl(NULL)
{
  Ref<WebCore::MediaSource> webkitMediaSource = WebCore::MediaSource::create(MSEWebkitDocument::get().getDocument());

  addWebkitEventListener(&webkitMediaSource.get(), "sourceopen");

  mMediaSourceImpl = new MSEMediaSourceImpl(webkitMediaSource.get());
}

MSEMediaSource::~MSEMediaSource()
{
  if (mMediaSourceImpl) {
    delete mMediaSourceImpl;
    mMediaSourceImpl = NULL;
  }
}

rtError MSEMediaSource::getSourceBuffers(rtObjectRef &v)
{
  v = &mBufferList;
  return RT_OK;
}

rtError MSEMediaSource::getActiveSourceBuffers(rtObjectRef &v)
{
  v = &mActiveBufferList;
  return RT_OK;
}

rtError MSEMediaSource::getReadyState(rtString &v) const
{
  v = mReadyState;
  return RT_OK;
}

rtError MSEMediaSource::getDuration(double &v) const
{
  v = mDuration;
  return RT_OK;
}

rtError MSEMediaSource::setDuration(double const &v)
{
  mDuration = v;
  return RT_OK;
}

rtError MSEMediaSource::addSourceBuffer(rtString type, rtObjectRef &buffer)
{
  MSESourceBuffer *sourceBuffer = new MSESourceBuffer(mMediaSourceImpl->mMediaSource.get(), type);
  buffer = sourceBuffer;
  return RT_OK;
}

rtError MSEMediaSource::clearLiveSeekableRange()
{
  // TODO
  return RT_OK;
}

rtError MSEMediaSource::endOfStream(rtString reason)
{
  rtLogWarn("endOfStream with reason = %s", reason.cString());
  return RT_OK;
}

rtError MSEMediaSource::removeSourceBuffer(rtObjectRef buffer)
{
  // TODO
  return RT_OK;
}

rtError MSEMediaSource::setLiveSeekableRange(double start, double end)
{
  // TODO
  return RT_OK;
}

rtError MSEMediaSource::isTypeSupported(rtString reason, bool &ret)
{
  ret = false;
  return RT_OK;
}

void MSEMediaSource::onSourceOpen()
{
  mEmit.send("sourceopen");
}

void MSEMediaSource::onSourceEnded()
{
  mEmit.send("sourceended");
}

void MSEMediaSource::onSourceClose()
{
  mEmit.send("sourceclose");
}

WebCore::MediaSource &MSEMediaSource::getWebKitMediaSource()
{
  return mMediaSourceImpl->mMediaSource.get();
}

void MSEMediaSource::onWebkitEvent(const std::string &name)
{
  mEmit.send(rtString(name.c_str()));
}

//SourceBuffer *MSEMediaSource::getCurSourceBuffer() const
//{
//  return curSourceBuffer;
//}
//
//void MSEMediaSource::load(const char *src)
//{
//  rtLogError("start load src = %s", src);
//  gstPlayer->loadFile(src);
//}
//
//
//GStreamPlayer *MSEMediaSource::getGstPlayer() const
//{
//  return gstPlayer;
//}
//
//void MSEMediaSource::setGstPlayer(GStreamPlayer *gstPlayer)
//{
//  MSEMediaSource::gstPlayer = gstPlayer;
//}
//
//void MSEMediaSource::onEvent(const char *event)
//{
//  if (!strcmp(event, "loadeddata")) {
//    mEmit.send("sourceopen", rtValue("sourceopen"));
//  } else if (!strcmp(event, "ended")) {
//    mEmit.send("sourceended", rtValue("sourceended"));
//  } else if (!strcmp(event, "closed")) {
//    mEmit.send("sourceclose", rtValue("sourceclose"));
//  }
//}

rtDefineObject(MSEMediaSource, MSEBaseObject)
rtDefineProperty(MSEMediaSource, sourceBuffers)
rtDefineProperty(MSEMediaSource, activeSourceBuffers)
rtDefineProperty(MSEMediaSource, duration)
rtDefineProperty(MSEMediaSource, readyState)
rtDefineMethod(MSEMediaSource, addSourceBuffer)
rtDefineMethod(MSEMediaSource, clearLiveSeekableRange)
rtDefineMethod(MSEMediaSource, endOfStream)
rtDefineMethod(MSEMediaSource, removeSourceBuffer)
rtDefineMethod(MSEMediaSource, setLiveSeekableRange)
rtDefineMethod(MSEMediaSource, isTypeSupported)
