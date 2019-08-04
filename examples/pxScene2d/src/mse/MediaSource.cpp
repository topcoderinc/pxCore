#include "MediaSource.h"
#include "MSEUtils.h"


MediaSource::MediaSource()
{
}

rtError MediaSource::getSourceBuffers(rtObjectRef &v) const
{
  v = mBufferList;
  return RT_OK;
}

rtError MediaSource::getActiveSourceBuffers(rtObjectRef &v) const
{
  v = mActiveBufferList;
  return RT_OK;
}

rtError MediaSource::getReadyState(rtString &v) const
{
  v = mReadyState;
  return RT_OK;
}

rtError MediaSource::getDuration(double &v) const
{
  v = mDuration;
  return RT_OK;
}

rtError MediaSource::setDuration(double const &v)
{
  mDuration = v;
  return RT_OK;
}

rtError MediaSource::addSourceBuffer(rtString type, rtObjectRef &buffer)
{
  // TODO
  return RT_OK;
}

rtError MediaSource::clearLiveSeekableRange()
{
  // TODO
  return RT_OK;
}

rtError MediaSource::endOfStream(rtString reason)
{
  rtLogWarn("endOfStream with reason = %s", reason.cString());
  return RT_OK;
}

rtError MediaSource::removeSourceBuffer(rtObjectRef buffer)
{
  // TODO
  return RT_OK;
}

rtError MediaSource::setLiveSeekableRange(double start, double end)
{
  // TODO
  return RT_OK;
}

rtError MediaSource::isTypeSupported(rtString reason, bool &ret)
{
  ret = false;
  return RT_OK;
}

void MediaSource::onSourceOpen()
{
  mEmit.send("onsourceopen");
}

void MediaSource::onSourceEnded()
{
  mEmit.send("onsourceended");
}

void MediaSource::onSourceClose()
{
  mEmit.send("onsourceclose");
}

//SourceBuffer *MediaSource::getCurSourceBuffer() const
//{
//  return curSourceBuffer;
//}
//
//void MediaSource::load(const char *src)
//{
//  rtLogError("start load src = %s", src);
//  gstPlayer->loadFile(src);
//}
//
//
//GStreamPlayer *MediaSource::getGstPlayer() const
//{
//  return gstPlayer;
//}
//
//void MediaSource::setGstPlayer(GStreamPlayer *gstPlayer)
//{
//  MediaSource::gstPlayer = gstPlayer;
//}
//
//void MediaSource::onEvent(const char *event)
//{
//  if (!strcmp(event, "loadeddata")) {
//    mEmit.send("sourceopen", rtValue("sourceopen"));
//  } else if (!strcmp(event, "ended")) {
//    mEmit.send("sourceended", rtValue("sourceended"));
//  } else if (!strcmp(event, "closed")) {
//    mEmit.send("sourceclose", rtValue("sourceclose"));
//  }
//}

rtDefineObject(MediaSource, MSEBaseObject)
rtDefineProperty(MediaSource, sourceBuffers)
rtDefineProperty(MediaSource, activeSourceBuffers)
rtDefineProperty(MediaSource, duration)
rtDefineProperty(MediaSource, readyState)
rtDefineMethod(MediaSource, addSourceBuffer)
rtDefineMethod(MediaSource, clearLiveSeekableRange)
rtDefineMethod(MediaSource, endOfStream)
rtDefineMethod(MediaSource, removeSourceBuffer)
rtDefineMethod(MediaSource, setLiveSeekableRange)
rtDefineMethod(MediaSource, isTypeSupported)