#include "SourceBuffer.h"
#include "MSEUtils.h"

rtDefineObject(SourceBuffer, MSEBaseObject)
rtDefineProperty(SourceBuffer, appendWindowEnd)
rtDefineProperty(SourceBuffer, appendWindowStart)
rtDefineProperty(SourceBuffer, audioTracks)
rtDefineProperty(SourceBuffer, buffered)
rtDefineProperty(SourceBuffer, mode)
rtDefineProperty(SourceBuffer, textTracks)
rtDefineProperty(SourceBuffer, timestampOffset)
rtDefineProperty(SourceBuffer, updating)
rtDefineProperty(SourceBuffer, videoTracks)
rtDefineMethod(SourceBuffer, abort)
rtDefineMethod(SourceBuffer, appendBuffer)
rtDefineMethod(SourceBuffer, changeType)
rtDefineMethod(SourceBuffer, remove)

SourceBuffer::SourceBuffer() :
    gstPlayer(nullptr)
{
}

SourceBuffer::~SourceBuffer()
{
}

rtError SourceBuffer::abort()
{
  // TODO
  return RT_OK;
}

rtError SourceBuffer::appendBuffer(rtObjectRef buffer)
{
  // TODO
  return RT_OK;
}

void SourceBuffer::onUpdateStart()
{
  mEmit.send("onupdatestart");
}

void SourceBuffer::onUpdate()
{
  mEmit.send("onupdate");
}

void SourceBuffer::onUpdateEnd()
{
  mEmit.send("onupdateend");
}

void SourceBuffer::onError()
{
  mEmit.send("onerror");
}

void SourceBuffer::onAbort()
{
  mEmit.send("onabort");
}

rtError SourceBuffer::changeType(rtString type)
{
  // TODO
  return RT_OK;
}

rtError SourceBuffer::remove(int32_t start, int32_t end)
{
  // TODO
  return RT_OK;
}

rtError SourceBuffer::getAppendWindowEnd(float &v) const
{
  v = mAppendWindowEnd;
  return RT_OK;
}

rtError SourceBuffer::setAppendWindowEnd(float const &v)
{
  mAppendWindowEnd = v;
  return RT_OK;
}

rtError SourceBuffer::getAppendWindowStart(float &v) const
{
  v = mAppendWindowStart;
  return RT_OK;
}

rtError SourceBuffer::setAppendWindowStart(float const &v)
{
  mAppendWindowStart = v;
  return RT_OK;
}

rtError SourceBuffer::getBuffered(rtObjectRef &v) const
{
  v = mBuffered;
  return RT_OK;
}

rtError SourceBuffer::getMode(rtString &v) const
{
  v = mMode;
  return RT_OK;
}

rtError SourceBuffer::setMode(rtString const &v)
{
  mMode = v;
  return RT_OK;
}

rtError SourceBuffer::getAudioTracks(rtObjectRef &v) const
{
  v = mAudioTrackList;
  return RT_OK;
}

rtError SourceBuffer::getVideoTracks(rtObjectRef &v) const
{
  v = mVideoTrackList;
  return RT_OK;
}

rtError SourceBuffer::getTextTracks(rtObjectRef &v) const
{
  v = mTextTrackList;
  return RT_OK;
}

rtError SourceBuffer::getTimestampOffset(float &v) const
{
  v = mTimestampOffset;
  return RT_OK;
}

rtError SourceBuffer::getUpdating(bool &v) const
{
  v = mUpdating;
  return RT_OK;
}

GStreamPlayer *SourceBuffer::getGstPlayer() const
{
  return gstPlayer;
}

void SourceBuffer::setGstPlayer(GStreamPlayer *gstPlayer)
{
  SourceBuffer::gstPlayer = gstPlayer;
}
