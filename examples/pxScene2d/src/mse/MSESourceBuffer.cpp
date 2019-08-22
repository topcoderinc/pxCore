#include "MSESourceBuffer.h"
#include "MSEUtils.h"

// webkit-includes
#include "WebCore/config.h"
#include "MediaSource.h"
#include "SourceBuffer.h"
#include "HTMLVideoElement.h"
#include "WebCore/fileapi/Blob.h"
#include "WebCore/dom/Event.h"
#include "WebCore/dom/EventNames.h"
#include "WebCore/bindings/js/BufferSource.h"


struct MSESourceBufferImpl {
  MSESourceBufferImpl(WebCore::MediaSource &mediaSource, rtString type): 
    mMediaSource(mediaSource),
    mSourceBuffer(mediaSource.addSourceBuffer(type.cString()))
  {
    assert(!mSourceBuffer.hasException());
  }

  WebCore::SourceBuffer &getSourceBuffer()
  {
    assert(!mSourceBuffer.hasException());
    return mSourceBuffer.returnValue().get();
  }

  Ref<WebCore::MediaSource>                        mMediaSource;
  WebCore::ExceptionOr<Ref<WebCore::SourceBuffer>> mSourceBuffer;
};

MSESourceBuffer::MSESourceBuffer(WebCore::MediaSource &mediaSource, rtString type)
{
  mSourceBufferImpl = new MSESourceBufferImpl(mediaSource, type);

  for (auto s: {"updateend", "error", "abort"}) {
    addWebkitEventListener(&mSourceBufferImpl->getSourceBuffer(), s);
  }
}

MSESourceBuffer::~MSESourceBuffer()
{
  if (mSourceBufferImpl) {
    delete mSourceBufferImpl;
    mSourceBufferImpl = NULL;
  }
}

void MSESourceBuffer::onWebkitEvent(const std::string &name)
{
  mEmit.send(rtString(name.c_str()));
}

rtError MSESourceBuffer::abort()
{
  // TODO
  return RT_OK;
}

rtError MSESourceBuffer::appendBuffer(const rtBuffer &buffer)
{
  Ref<JSC::ArrayBuffer> jscBuf = JSC::ArrayBuffer::create(buffer.getData(), buffer.getSize());
  WebCore::BufferSource bufferSource(WTF::RefPtr<JSC::ArrayBuffer>(&jscBuf.get()));
  WebCore::ExceptionOr<void> exc = mSourceBufferImpl->getSourceBuffer().appendBuffer(bufferSource);
  assert(!exc.hasException());
  return RT_OK;
}

void MSESourceBuffer::onUpdateStart()
{
  mEmit.send("updatestart");
}

void MSESourceBuffer::onUpdate()
{
  mEmit.send("update");
}

void MSESourceBuffer::onUpdateEnd()
{
  mEmit.send("updateend");
}

void MSESourceBuffer::onError()
{
  mEmit.send("error");
}

void MSESourceBuffer::onAbort()
{
  mEmit.send("abort");
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