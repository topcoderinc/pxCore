#include "SourceBuffer.h"


#include "MediaSource.h"
#include "SourceBufferList.h"
#include "track/BaseTrackList.h"
#include <limits>
#include <map>

namespace WebCore {
  static const double ExponentialMovingAverageCoefficient = 0.1;


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
  rtDefineMethod(SourceBuffer, remove)

  struct SourceBuffer::TrackBuffer {
    MediaTime lastDecodeTimestamp;
    MediaTime lastFrameDuration;
    MediaTime highestPresentationTimestamp;
    MediaTime lastEnqueuedPresentationTime;
    MediaTime lastEnqueuedDecodeEndTime;
    bool needRandomAccessFlag{true};
    bool enabled{false};
    bool needsReenqueueing{false};
    SampleMap samples;
    MediaDescription *description;
    PlatformTimeRanges buffered;

    TrackBuffer()
        : lastDecodeTimestamp(MediaTime::invalidTime()), lastFrameDuration(MediaTime::invalidTime()),
          highestPresentationTimestamp(MediaTime::invalidTime()),
          lastEnqueuedPresentationTime(MediaTime::invalidTime()), lastEnqueuedDecodeEndTime(MediaTime::invalidTime())
    {
    }
  };

  SourceBuffer *SourceBuffer::create(MediaSource *source)
  {
    return new SourceBuffer(source);
  }

  SourceBuffer::SourceBuffer(MediaSource *source)
      : m_source(source),

        m_appendWindowStart(MediaTime::zeroTime()), m_appendWindowEnd(MediaTime::positiveInfiniteTime()),
        m_groupStartTimestamp(MediaTime::invalidTime()), m_groupEndTimestamp(MediaTime::zeroTime()),
        m_buffered(TimeRanges::create()), m_appendState(WaitingForSegment),
        m_timeOfBufferingMonitor(0), m_pendingRemoveStart(MediaTime::invalidTime()),
        m_pendingRemoveEnd(MediaTime::invalidTime())
  {

  }

  SourceBuffer::~SourceBuffer()
  {
  }

// Allow hasCurrentTime() to be off by as much as the length of two 24fps video frames
  MediaTime &SourceBuffer::currentTimeFudgeFactor() const
  {
    MediaTime mediaTime = MediaTime::invalidTime();
    return mediaTime;
  }

  TimeRanges *SourceBuffer::buffered() const
  {
    return nullptr;
  }

  double SourceBuffer::timestampOffset() const
  {
    return 0.0;
  }

  void SourceBuffer::setTimestampOffset(double offset)
  {
  }

  double SourceBuffer::appendWindowStart() const
  {
    return 0.0;
  }

  void SourceBuffer::setAppendWindowStart(double newValue)
  {

  }

  double SourceBuffer::appendWindowEnd() const
  {
    return 0.0;
  }

  void SourceBuffer::setAppendWindowEnd(double newValue)
  {
  }

  void SourceBuffer::appendBuffer(const BufferSource &data)
  {
    return appendBufferInternal(static_cast<const unsigned char *>(data.data()), data.length());
  }

  void SourceBuffer::resetParserState()
  {
  }

  rtError SourceBuffer::abort()
  {
    return RT_OK;
  }

  rtError SourceBuffer::remove(double start, double end)
  {
    return RT_OK;
  }


  void SourceBuffer::rangeRemoval(const MediaTime &start, const MediaTime &end)
  {
    mEmit.send("onupdatestart");
  }

  void SourceBuffer::abortIfUpdating()
  {
    // Section 3.2 abort() method step 4 substeps.
    // https://rawgit.com/w3c/media-source/45627646344eea0170dd1cbc5a3d508ca751abb8/media-source-respec.html#dom-sourcebuffer-abort
    if (!m_updating)
      return;
    m_pendingAppendData.clear();


    mEmit.send("onabort");
    mEmit.send("onupdateend");
  }

  MediaTime SourceBuffer::highestPresentationTimestamp() const
  {
    return MediaTime::invalidTime();
  }

  void SourceBuffer::readyStateChanged()
  {
    updateBufferedFromTrackBuffers();
  }

  void SourceBuffer::removedFromMediaSource()
  {
    if (isRemoved())
      return;

    abortIfUpdating();
    m_source = nullptr;
  }

  void SourceBuffer::seekToTime(const MediaTime &time)
  {

  }

  MediaTime SourceBuffer::sourceBufferPrivateFastSeekTimeForMediaTime(const MediaTime &targetTime,
                                                                      const MediaTime &negativeThreshold,
                                                                      const MediaTime &positiveThreshold)
  {
    MediaTime seekTime = targetTime;
    return seekTime;
  }

  bool SourceBuffer::hasPendingActivity() const
  {
    return false;
  }

  void SourceBuffer::stop()
  {
  }

  bool SourceBuffer::canSuspendForDocumentSuspension() const
  {
    return !hasPendingActivity();
  }

  const char *SourceBuffer::activeDOMObjectName() const
  {
    return "SourceBuffer";
  }

  bool SourceBuffer::isRemoved() const
  {
    return !m_source;
  }


  void SourceBuffer::appendBufferInternal(const unsigned char *data, unsigned size)
  {
    mEmit.send("onupdatestart");
  }

  void SourceBuffer::appendBufferTimerFired()
  {
  }

  void SourceBuffer::sourceBufferPrivateAppendComplete(AppendResult result)
  {
    // 4. Queue a task to fire a simple event named update at this SourceBuffer object.
    mEmit.send("onupdate");

    // 5. Queue a task to fire a simple event named updateend at this SourceBuffer object.
    mEmit.send("onupdateend");
  }

  void SourceBuffer::sourceBufferPrivateDidReceiveRenderingError(int error)
  {
  }

  void SourceBuffer::removeCodedFrames(const MediaTime &start, const MediaTime &end, bool keepDecodeQueue)
  {
  }

  void SourceBuffer::removeTimerFired()
  {
    if (isRemoved())
      return;

    mEmit.send("onupdate");
    mEmit.send("onupdateend");
  }

  size_t SourceBuffer::maxBufferSizeVideo = 0;
  size_t SourceBuffer::maxBufferSizeAudio = 0;
  size_t SourceBuffer::maxBufferSizeText = 0;

  static void
  maximumBufferSizeDefaults(size_t &maxBufferSizeVideo, size_t &maxBufferSizeAudio, size_t &maxBufferSizeText)
  {

  }

  size_t SourceBuffer::maximumBufferSize() const
  {

  }

  BaseTrackList &SourceBuffer::videoTracks()
  {
    return *m_videoTracks;
  }

  BaseTrackList &SourceBuffer::audioTracks()
  {
    return *m_audioTracks;
  }

  BaseTrackList &SourceBuffer::textTracks()
  {
    return *m_textTracks;
  }

  void SourceBuffer::setActive(bool active)
  {
    if (m_active == active)
      return;
  }

  void SourceBuffer::sourceBufferPrivateDidReceiveInitializationSegment(const InitializationSegment &segment)
  {
    if (isRemoved())
      return;
  }

  bool SourceBuffer::validateInitializationSegment(const InitializationSegment &segment)
  {
    return true;
  }


  void SourceBuffer::appendError(bool decodeErrorParam)
  {

    // 3. Queue a task to fire a simple event named error at this SourceBuffer object.
    mEmit.send("onerror");

    // 4. Queue a task to fire a simple event named updateend at this SourceBuffer object.
    mEmit.send("onupdateend");


    // 5. If decode error is true, then run the end of stream algorithm with the error parameter set to "decode".
    if (decodeErrorParam)
      m_source->streamEndedWithError(MediaSource::EndOfStreamError::Decode);

  }

  void SourceBuffer::sourceBufferPrivateDidReceiveSample(MediaSample &sample)
  {
  }

  bool SourceBuffer::hasAudio() const
  {
    return m_audioTracks && m_audioTracks->size();
  }

  bool SourceBuffer::hasVideo() const
  {
    return m_videoTracks && m_videoTracks->size();
  }

  bool SourceBuffer::sourceBufferPrivateHasAudio() const
  {
    return hasAudio();
  }

  bool SourceBuffer::sourceBufferPrivateHasVideo() const
  {
    return hasVideo();
  }

  void SourceBuffer::videoTrackSelectedChanged(VideoTrack &track)
  {

  }

  void SourceBuffer::audioTrackEnabledChanged(AudioTrack &track)
  {

  }

  void SourceBuffer::textTrackModeChanged(TextTrack &track)
  {

  }

  void SourceBuffer::textTrackKindChanged(TextTrack &track)
  {

  }

  void SourceBuffer::sourceBufferPrivateReenqueSamples(const rtString &trackID)
  {
    if (isRemoved())
      return;
  }

  void SourceBuffer::sourceBufferPrivateDidBecomeReadyForMoreSamples(const rtString &trackID)
  {
    if (isRemoved())
      return;
  }

  void SourceBuffer::provideMediaData(TrackBuffer &trackBuffer, const rtString &trackID)
  {
    if (m_source->isSeeking())
      return;
  }

  void SourceBuffer::reenqueueMediaForTime(TrackBuffer &trackBuffer, const rtString &trackID, const MediaTime &time)
  {
  }


  void SourceBuffer::didDropSample()
  {
  }

  void SourceBuffer::monitorBufferingRate()
  {
  }

  void SourceBuffer::updateBufferedFromTrackBuffers()
  {
  }

  bool SourceBuffer::canPlayThroughRange(PlatformTimeRanges &ranges)
  {

  }

  size_t SourceBuffer::extraMemoryCost() const
  {
  }

  void SourceBuffer::reportExtraMemoryAllocated()
  {
  }


  void SourceBuffer::setMode(AppendMode newMode)
  {

  }


  rtError SourceBuffer::appendBuffer(rtObjectRef buffer)
  {
    this->appendBuffer(*(BufferSource *) buffer.getPtr());
    return RT_OK;
  }


  rtError SourceBuffer::getAppendWindowEnd(double &v) const
  {
    v = this->m_appendWindowEnd.toDouble();
    return RT_OK;
  }

  rtError SourceBuffer::updateAppendWindowEnd(double const &v)
  {
    return RT_OK;
  }

  rtError SourceBuffer::getAppendWindowStart(double &v) const
  {
    v = this->m_appendWindowStart.toDouble();
    return RT_OK;
  }

  rtError SourceBuffer::updateAppendWindowStart(double const &v)
  {
    return RT_OK;
  }

  rtError SourceBuffer::getAudioTracks(rtObjectRef &v) const
  {
    v = m_audioTracks;
    return RT_OK;
  }

  rtError SourceBuffer::getBuffered(rtObjectRef &v) const
  {
    v = m_buffered;
    return RT_OK;
  }

  rtError SourceBuffer::getMode(int32_t &v) const
  {
    v = static_cast<int32_t>(this->m_mode);
    return RT_OK;
  }

  rtError SourceBuffer::setMode(int32_t const &v)
  {
    this->m_mode = (AppendMode) v;
    return RT_OK;
  }

  rtError SourceBuffer::getTextTracks(rtObjectRef &v) const
  {
    v = m_textTracks;
    return RT_OK;
  }

  rtError SourceBuffer::getVideoTracks(rtObjectRef &v) const
  {
    v = m_videoTracks;
    return RT_OK;
  }

  rtError SourceBuffer::getTimestampOffset(double &v) const
  {
    v = m_timestampOffset.toDouble();
    return RT_OK;
  }

  rtError SourceBuffer::getUpdating(bool &v) const
  {
    v = m_updating;
    return RT_OK;
  }
} // namespace WebCore
