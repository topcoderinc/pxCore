#include "MediaSource.h"
#include "AAMPUtils.h"


MediaSource::MediaSource() : curSourceBuffer(nullptr),
                             bufferList(new SourceBufferList)
{
}

rtError MediaSource::getSourceBuffers(rtObjectRef &v) const
{
  return bufferList->getSourceBuffer(v);
}

rtError MediaSource::getActiveSourceBuffers(rtObjectRef &v) const
{

  auto *rtArr = new rtArrayObject();
  rtValue c(curSourceBuffer);
  rtArr->Set((int32_t) 0, &c);
  v = rtArr;
  return RT_OK;
}

rtError MediaSource::getReadyState(int32_t &v) const
{
  v = gstPlayer->getReadyState();
  return RT_OK;
}

rtError MediaSource::getDuration(double &v) const
{
  v = gstPlayer->getDuration();
  return RT_OK;
}

rtError MediaSource::setDuration(double const &v)
{
  rtLogError("cannot set duration for const video");
  return RT_OK;
}

rtError MediaSource::addSourceBuffer(rtObjectRef buffer)
{
  auto *sourceBuffer = dynamic_cast<SourceBuffer *>(buffer.getPtr());
  sourceBuffer->setGstPlayer(gstPlayer);
  bufferList->add(sourceBuffer);
  if (curSourceBuffer) {
    gstPlayer->release();
  }
  curSourceBuffer = sourceBuffer;
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
  gstPlayer->release();
  return RT_OK;
}

rtError MediaSource::removeSourceBuffer(rtObjectRef buffer)
{
  // TODO
  return RT_OK;
}

rtError MediaSource::setLiveSeekableRange(int32_t start, int32_t end)
{
  // TODO
  return RT_OK;
}


SourceBuffer *MediaSource::getCurSourceBuffer() const
{
  return curSourceBuffer;
}

void MediaSource::load(const char *src)
{
  rtLogError("start load src = %s", src);
  gstPlayer->loadFile(src);
}


GStreamPlayer *MediaSource::getGstPlayer() const
{
  return gstPlayer;
}

void MediaSource::setGstPlayer(GStreamPlayer *gstPlayer)
{
  MediaSource::gstPlayer = gstPlayer;
}

void MediaSource::onEvent(const char *event)
{
  if (!strcmp(event, "loadeddata")) {
    mEmit.send("sourceopen", rtValue("sourceopen"));
  } else if (!strcmp(event, "ended")) {
    mEmit.send("sourceended", rtValue("sourceended"));
  } else if (!strcmp(event, "closed")) {
    mEmit.send("sourceclose", rtValue("sourceclose"));
  }

}

rtDefineObject(MediaSource, AAMPBaseObject)
rtDefineProperty(MediaSource, sourceBuffers)
rtDefineProperty(MediaSource, activeSourceBuffers)
rtDefineProperty(MediaSource, readyState)
rtDefineProperty(MediaSource, duration)
rtDefineMethod(MediaSource, addSourceBuffer)
rtDefineMethod(MediaSource, clearLiveSeekableRange)
rtDefineMethod(MediaSource, endOfStream)
rtDefineMethod(MediaSource, removeSourceBuffer)
rtDefineMethod(MediaSource, setLiveSeekableRange)