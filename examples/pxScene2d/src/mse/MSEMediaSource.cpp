#include "MSEMediaSource.h"
#include "MSEUtils.h"

#include <gst/gst.h>

// webkit-includes
#include "WebCore/config.h"
#include "MediaSource.h"
#include "HTMLVideoElement.h"
#include "WebCore/fileapi/Blob.h"

struct MSEMediaSourceImpl {
  MSEMediaSourceImpl(WebCore::Document &doc, WebCore::MediaSource &mediaSource,
     WebCore::HTMLVideoElement &videoElement)
    : mDocument(doc), mMediaSource(mediaSource), mVideoElement(videoElement)
  {

    mVideoElement->setSrcObject(WebCore::MediaProvider(RefPtr(&mediaSource)));

  }

  Ref<WebCore::Document> mDocument;
  Ref<WebCore::MediaSource> mMediaSource;
  Ref<WebCore::HTMLVideoElement> mVideoElement;
};

MSEMediaSource::MSEMediaSource(): mMediaSourceImpl(NULL)
{
  WTF::URL url;
  Ref<WebCore::Document> document = WebCore::Document::create(url);
  Ref<WebCore::MediaSource> mediaSource = WebCore::MediaSource::create(document.get());
  Ref<WebCore::HTMLVideoElement> videoElement = WebCore::HTMLVideoElement::create(document.get());
  mMediaSourceImpl = new MSEMediaSourceImpl(document.get(), mediaSource.get(), videoElement.get());
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
  // TODO
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
  mEmit.send("onsourceopen");
}

void MSEMediaSource::onSourceEnded()
{
  mEmit.send("onsourceended");
}

void MSEMediaSource::onSourceClose()
{
  mEmit.send("onsourceclose");
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