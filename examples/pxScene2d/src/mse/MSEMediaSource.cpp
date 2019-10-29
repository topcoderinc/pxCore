#include "MSEWebKitDocument.h"
#include "MSEMediaSource.h"
#include "MSESourceBuffer.h"
#include "MSESourceBufferList.h"
#include "MSEWebKitEventEmitter.h"
#include "MSEUtils.h"

#include "pxOffscreen.h"

#include <gst/gst.h>
#include <gst/video/video.h>

// webkit-includes
#include "WebCore/config.h"
#include "MediaSource.h"
#include "SourceBufferList.h"
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

rtError MSEMediaSource::getSourceBuffers(rtObjectRef &v) const
{
  auto ret = new MSESourceBufferList();
  auto sourceBufferList = getWebKitMediaSource().sourceBuffers();
  for (int i = 0; i < sourceBufferList->length(); ++i) {
    ret->add(new MSESourceBuffer(getWebKitMediaSource(), *sourceBufferList->item(i)));
  }
  v = ret;
  return RT_OK;
}

rtError MSEMediaSource::getActiveSourceBuffers(rtObjectRef &v) const
{
  //ASSERT_FIELD_ACCESS_NOT_IMPLEMENTED(activeSourceBufferList);
  //v = &mActiveBufferList;
  return RT_OK;
}

rtError MSEMediaSource::getReadyState(rtString &v) const
{
  auto val = getWebKitMediaSource().readyState();
  switch (val) {
  case WebCore::MediaSource::ReadyState::Closed: v = "closed"; break;
  case WebCore::MediaSource::ReadyState::Open: v = "open"; break;
  case WebCore::MediaSource::ReadyState::Ended: v = "ended"; break;
  }
  return RT_OK;
}

rtError MSEMediaSource::getDuration(double &v) const
{
  v = getWebKitMediaSource().duration().toDouble();
  return RT_OK;
}

rtError MSEMediaSource::setDuration(double const &v)
{
  getWebKitMediaSource().setDuration(v);
  return RT_OK;
}

rtError MSEMediaSource::addSourceBuffer(rtString type, rtObjectRef &buffer)
{
  MSESourceBuffer *sourceBuffer = new MSESourceBuffer(getWebKitMediaSource(), type);
  buffer = sourceBuffer;
  return RT_OK;
}

rtError MSEMediaSource::clearLiveSeekableRange()
{
  getWebKitMediaSource().clearLiveSeekableRange();
  return RT_OK;
}

rtError MSEMediaSource::endOfStream(rtString reason)
{
  rtLogWarn("endOfStream with reason = %s", reason.cString());
  auto code = reason == "decode" ? WebCore::MediaSource::EndOfStreamError::Decode : WebCore::MediaSource::EndOfStreamError::Network;
  getWebKitMediaSource().endOfStream(code);
  return RT_OK;
}

rtError MSEMediaSource::removeSourceBuffer(rtObjectRef buffer)
{
  MSESourceBuffer *sourceBuffer = dynamic_cast<MSESourceBuffer*>(buffer.ptr());
  getWebKitMediaSource().removeSourceBuffer(sourceBuffer->getWebkitSourceBuffer());
  return RT_OK;
}

rtError MSEMediaSource::setLiveSeekableRange(double start, double end)
{
  getWebKitMediaSource().setLiveSeekableRange(start, end);
  return RT_OK;
}

rtError MSEMediaSource::isTypeSupported(rtString type, bool &ret)
{
  ret = WebCore::MediaSource::isTypeSupported(type.cString());
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

WebCore::MediaSource &MSEMediaSource::getWebKitMediaSource() const
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
