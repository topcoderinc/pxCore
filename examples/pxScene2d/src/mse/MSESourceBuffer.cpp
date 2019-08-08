#include "MSESourceBuffer.h"
#include "MSEUtils.h"

rtDefineObject(MSESourceBuffer, MSEBaseObject)
rtDefineProperty(MSESourceBuffer, appendWindowEnd)
rtDefineProperty(MSESourceBuffer, appendWindowStart)
rtDefineProperty(MSESourceBuffer, audioTracks)
rtDefineProperty(MSESourceBuffer, buffered)
rtDefineProperty(MSESourceBuffer, mode)
rtDefineProperty(MSESourceBuffer, textTracks)
rtDefineProperty(MSESourceBuffer, timestampOffset)
rtDefineProperty(MSESourceBuffer, updating)
rtDefineProperty(MSESourceBuffer, videoTracks)
rtDefineMethod(MSESourceBuffer, abort)
rtDefineMethod(MSESourceBuffer, appendBuffer)
rtDefineMethod(MSESourceBuffer, changeType)
rtDefineMethod(MSESourceBuffer, remove)

MSESourceBuffer::MSESourceBuffer()
{
}

MSESourceBuffer::~MSESourceBuffer()
{
}

rtError MSESourceBuffer::abort()
{
  // TODO
  return RT_OK;
}

rtError MSESourceBuffer::appendBuffer(rtObjectRef buffer)
{
  // TODO
  return RT_OK;
}

void MSESourceBuffer::onUpdateStart()
{
  mEmit.send("onupdatestart");
}

void MSESourceBuffer::onUpdate()
{
  mEmit.send("onupdate");
}

void MSESourceBuffer::onUpdateEnd()
{
  mEmit.send("onupdateend");
}

void MSESourceBuffer::onError()
{
  mEmit.send("onerror");
}

void MSESourceBuffer::onAbort()
{
  mEmit.send("onabort");
}

rtError MSESourceBuffer::changeType(const rtString &type)
{
  // TODO
  return RT_OK;
}

rtError MSESourceBuffer::remove(double start, double end)
{
  // TODO
  return RT_OK;
}

rtError MSESourceBuffer::getAppendWindowEnd(float &v) const
{
  v = mAppendWindowEnd;
  return RT_OK;
}

rtError MSESourceBuffer::setAppendWindowEnd(float const &v)
{
  mAppendWindowEnd = v;
  return RT_OK;
}

rtError MSESourceBuffer::getAppendWindowStart(float &v) const
{
  v = mAppendWindowStart;
  return RT_OK;
}

rtError MSESourceBuffer::setAppendWindowStart(float const &v)
{
  mAppendWindowStart = v;
  return RT_OK;
}

rtError MSESourceBuffer::getBuffered(rtObjectRef &v) const
{
  v = &mBuffered;
  return RT_OK;
}

rtError MSESourceBuffer::getMode(rtString &v) const
{
  v = mMode;
  return RT_OK;
}

rtError MSESourceBuffer::setMode(rtString const &v)
{
  mMode = v;
  return RT_OK;
}

rtError MSESourceBuffer::getAudioTracks(rtObjectRef &v) const
{
  v = &mAudioTrackList;
  return RT_OK;
}

rtError MSESourceBuffer::getVideoTracks(rtObjectRef &v) const
{
  v = &mVideoTrackList;
  return RT_OK;
}

rtError MSESourceBuffer::getTextTracks(rtObjectRef &v) const
{
  v = &mTextTrackList;
  return RT_OK;
}

rtError MSESourceBuffer::getTimestampOffset(float &v) const
{
  v = mTimestampOffset;
  return RT_OK;
}

rtError MSESourceBuffer::getUpdating(bool &v) const
{
  v = mUpdating;
  return RT_OK;
}
