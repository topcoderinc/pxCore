#include "config.h"
#include "MediaSource.h"
#include "SourceBuffer.h"
#include "SourceBufferList.h"

#include "AudioTrackMediaSource.h"
#include "TextTrackMediaSource.h"
#include "VideoTrackMediaSource.h"

namespace WebCore {


  rtDefineObject(MediaSource, MSEBaseObject)
  rtDefineProperty(MediaSource, sourceBuffers)
  rtDefineProperty(MediaSource, activeSourceBuffers)
  rtDefineProperty(MediaSource, readyState)
  rtDefineProperty(MediaSource, duration)
  rtDefineMethod(MediaSource, addSourceBuffer)
  rtDefineMethod(MediaSource, clearLiveSeekableRange)
  rtDefineMethod(MediaSource, endOfStream)
  rtDefineMethod(MediaSource, removeSourceBuffer)
  rtDefineMethod(MediaSource, isTypeSupported)
  rtDefineMethod(MediaSource, setLiveSeekableRange)

  static const char *toString(MediaSource::ReadyState readyState)
  {
    switch (readyState) {
      case MediaSource::ReadyState::Closed:
        return "closed";
      case MediaSource::ReadyState::Ended:
        return "ended";
      case MediaSource::ReadyState::Open:
        return "open";
      default:
        return "(unknown)";
    }
  }

  MediaSource *MediaSource::create()
  {
    return new MediaSource();
  }

  MediaSource::MediaSource()
      : m_duration(MediaTime::invalidTime()), m_pendingSeekTime(MediaTime::invalidTime())
  {
    m_sourceBuffers = SourceBufferList::create();
    m_activeSourceBuffers = SourceBufferList::create();
  }

  MediaSource::~MediaSource()
  {
  }


  void MediaSource::addedToRegistry()
  {

  }

  void MediaSource::removedFromRegistry()
  {

  }

  MediaTime MediaSource::duration() const
  {
    return m_duration;
  }

  void MediaSource::durationChanged(const MediaTime &duration)
  {
    m_duration = duration;
  }

  MediaTime MediaSource::currentTime() const
  {
    return MediaTime::zeroTime();
  }

  std::unique_ptr<PlatformTimeRanges> MediaSource::buffered() const
  {
    return nullptr;
  }

  void MediaSource::seekToTime(const MediaTime &time)
  {
    if (isClosed())
      return;
    completeSeek();
  }

  void MediaSource::completeSeek()
  {
    if (isClosed())
      return;
    monitorSourceBuffers();
  }

  TimeRanges *MediaSource::seekable()
  {
    return new TimeRanges();
  }

  rtError MediaSource::setLiveSeekableRange(double start, double end)
  {
    return RT_OK;
  }

  rtError MediaSource::clearLiveSeekableRange()
  {
    return RT_OK;
  }

  const MediaTime &MediaSource::currentTimeFudgeFactor()
  {
    return MediaTime();
  }

  bool MediaSource::hasBufferedTime(const MediaTime &time)
  {
    return false;
  }

  bool MediaSource::hasCurrentTime()
  {
    return hasBufferedTime(currentTime());
  }

  bool MediaSource::hasFutureTime()
  {
    return false;
  }

  void MediaSource::monitorSourceBuffers()
  {
    if (isClosed())
      return;
    if (m_pendingSeekTime.isValid())
      completeSeek();
  }

  rtError MediaSource::setDuration(const double &v)
  {
    setDurationInternal(MediaTime::createWithDouble(v));
    return RT_OK;
  }

  void MediaSource::setDurationInternal(const MediaTime &duration)
  {

  }

  void MediaSource::setReadyState(ReadyState state)
  {
    auto oldState = readyState();
    if (oldState == state)
      return;

    m_readyState = state;

    onReadyStateChange(oldState, state);
  }

  rtError MediaSource::endOfStream(uint32_t error)
  {
    streamEndedWithError(static_cast<EndOfStreamError >(error));
    return RT_OK;
  }

  void MediaSource::streamEndedWithError(EndOfStreamError error)
  {
    if (isClosed())
      return;
  }

  SourceBuffer &MediaSource::addSourceBuffer(const rtString &type)
  {
    SourceBuffer *sourceBuffer = SourceBuffer::create(this);
    return *sourceBuffer;
  }

  void MediaSource::removeSourceBuffer(SourceBuffer &buffer)
  {

  }

  bool MediaSource::isTypeSupported(const rtString &type)
  {
    return false;
  }

  bool MediaSource::isOpen() const
  {
    return readyState() == ReadyState::Open;
  }

  bool MediaSource::isClosed() const
  {
    return readyState() == ReadyState::Closed;
  }

  bool MediaSource::isEnded() const
  {
    return readyState() == ReadyState::Ended;
  }

  void MediaSource::detachFromElement(pxHtmlVideo &element)
  {

  }

  void MediaSource::sourceBufferDidChangeActiveState(SourceBuffer &, bool)
  {
    regenerateActiveSourceBuffers();
  }

  bool MediaSource::attachToElement(pxHtmlVideo &element)
  {
    if (m_mediaElement)
      return false;
    return true;
  }

  void MediaSource::openIfInEndedState()
  {
    if (m_readyState != ReadyState::Ended)
      return;

    setReadyState(ReadyState::Open);
  }

  bool MediaSource::hasPendingActivity() const
  {
    return false;
  }

  void MediaSource::stop()
  {
    m_readyState = ReadyState::Closed;
  }

  bool MediaSource::canSuspendForDocumentSuspension() const
  {
    return isClosed();
  }

  const char *MediaSource::activeDOMObjectName() const
  {
    return "MediaSource";
  }

  void MediaSource::onReadyStateChange(ReadyState oldState, ReadyState newState)
  {
    if (isOpen()) {
      mEmit.send("onsourceopen");
      return;
    }

    if (oldState == ReadyState::Open && newState == ReadyState::Ended) {
      mEmit.send("onsourceended");
      return;
    }

    if (isClosed()) {
      mEmit.send("onsourceclose");
    }
  }

  std::vector<PlatformTimeRanges> MediaSource::activeRanges() const
  {
    return std::vector<PlatformTimeRanges>();
  }


  void MediaSource::regenerateActiveSourceBuffers()
  {

  }

  rtError MediaSource::getSourceBuffers(rtObjectRef &v) const
  {
    v = m_sourceBuffers;
    return RT_OK;
  }

  rtError MediaSource::getActiveSourceBuffers(rtObjectRef &v) const
  {
    v = m_activeSourceBuffers;
    return RT_OK;
  }

  rtError MediaSource::getReadyState(int32_t &v) const
  {
    v = static_cast<int>(m_readyState);
    return RT_OK;
  }

  rtError MediaSource::getDuration(double &v) const
  {
    v = m_duration.toDouble();
    return RT_OK;
  }

  rtError MediaSource::isTypeSupported(rtString type, bool &v)
  {
    v = isTypeSupported(type);
    return RT_OK;
  }

  rtError MediaSource::addSourceBuffer(rtString type, rtObjectRef &v)
  {
    v = &this->addSourceBuffer(type);
    return RT_OK;
  }

  rtError MediaSource::removeSourceBuffer(rtObjectRef buffer)
  {
    this->removeSourceBuffer(*static_cast<SourceBuffer *>(buffer.getPtr()));
    return RT_OK;
  }

}


