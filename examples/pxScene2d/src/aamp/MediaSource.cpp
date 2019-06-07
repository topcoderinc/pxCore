#include "MediaSource.h"
#include "AAMPUtils.h"

#include "priv_aamp.h"

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
  // TODO
  return RT_OK;
}

rtError MediaSource::getDuration(double &v) const
{
  v = instanceAamp->GetPlaybackPosition();
  return RT_OK;
}

rtError MediaSource::setDuration(double const &v)
{
  // TODO
  return RT_OK;
}

rtError MediaSource::addSourceBuffer(rtObjectRef buffer)
{
  auto *sourceBuffer = dynamic_cast<SourceBuffer *>(buffer.getPtr());
  sourceBuffer->setInstanceAamp(instanceAamp);
  bufferList->add(sourceBuffer);
  if (curSourceBuffer) {
    instanceAamp->Stop();
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
  // TODO
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
  instanceAamp->Tune(src);
}

rtDefineObject(MediaSource, AAMPBaseObject)

PlayerInstanceAAMP *MediaSource::getInstanceAamp() const
{
  return instanceAamp;
}

void MediaSource::setInstanceAamp(PlayerInstanceAAMP *instanceAamp)
{
  MediaSource::instanceAamp = instanceAamp;
  instanceAamp->aamp->mStateChangeFunc = [&](const char *type) {
    mEmit.send(type);
  };
}

rtDefineProperty(MediaSource, sourceBuffers)
rtDefineProperty(MediaSource, activeSourceBuffers)
rtDefineProperty(MediaSource, readyState)
rtDefineProperty(MediaSource, duration)
rtDefineMethod(MediaSource, addSourceBuffer)
rtDefineMethod(MediaSource, clearLiveSeekableRange)
rtDefineMethod(MediaSource, endOfStream)
rtDefineMethod(MediaSource, removeSourceBuffer)
rtDefineMethod(MediaSource, setLiveSeekableRange)