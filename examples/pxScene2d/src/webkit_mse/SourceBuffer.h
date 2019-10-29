#pragma once


#include "track/AudioTrack.h"
#include "track/TextTrack.h"
#include "track/VideoTrack.h"
#include "MediaSource.h"

#include <map>

namespace WebCore {


  class MediaSource;

  class PlatformTimeRanges;

  class BaseTrackList;

  class TimeRanges;

  class MediaTime;


  class BufferSource {
  public:
    int length() const
    { return 0; };

    void *data() const
    { return nullptr; }
  };

  // TODO
  class InitializationSegment {

  };

  // TODO
  class MediaSample {

  };

  // TODO
  class SampleMap {

  };

  // TODO
  class AppendResult {

  };

  //TODO
  class MediaDescription {

  };

  class SourceBuffer : public MSEBaseObject {
  public:
    static SourceBuffer *create(MediaSource *);


    virtual ~SourceBuffer();

    bool updating() const
    { return m_updating; }

    TimeRanges *buffered() const;

    double timestampOffset() const;

    void setTimestampOffset(double);


    BaseTrackList &videoTracks();

    BaseTrackList &audioTracks();

    BaseTrackList &textTracks();


    double appendWindowStart() const;

    void setAppendWindowStart(double);

    double appendWindowEnd() const;

    void setAppendWindowEnd(double);

    void appendBuffer(const BufferSource &);

    rtError abort();

    rtError remove(double start, double end);

    const TimeRanges &bufferedInternal() const
    {
      return *m_buffered;
    }

    MediaTime &currentTimeFudgeFactor() const;

    void abortIfUpdating();

    void removedFromMediaSource();

    void seekToTime(const MediaTime &);

    bool canPlayThroughRange(PlatformTimeRanges &);

    bool hasVideo() const;

    bool active() const
    { return m_active; }


    struct TrackBuffer;


    enum class AppendMode {
      Segments, Sequence
    };

    AppendMode mode() const
    { return m_mode; }

    void setMode(AppendMode);

    void setShouldGenerateTimestamps(bool flag)
    { m_shouldGenerateTimestamps = flag; }

    void rangeRemoval(const MediaTime &, const MediaTime &);

    bool isBufferedDirty() const
    { return m_bufferedDirty; }

    void setBufferedDirty(bool flag)
    { m_bufferedDirty = flag; }

    MediaTime highestPresentationTimestamp() const;

    void readyStateChanged();

    bool hasPendingActivity() const;

    rtDeclareObject(SourceBuffer, MSEBaseObject);

    rtProperty(appendWindowEnd, getAppendWindowEnd, updateAppendWindowEnd, double);

    rtProperty(appendWindowStart, getAppendWindowStart, updateAppendWindowStart, double);

    rtReadOnlyProperty(audioTracks, getAudioTracks, rtObjectRef);

    rtReadOnlyProperty(buffered, getBuffered, rtObjectRef);

    rtProperty(mode, getMode, setMode, int32_t);

    rtReadOnlyProperty(textTracks, getTextTracks, rtObjectRef);

    rtReadOnlyProperty(timestampOffset, getTimestampOffset, double);

    rtReadOnlyProperty(updating, getUpdating, bool);

    rtReadOnlyProperty(videoTracks, getVideoTracks, rtObjectRef);

    rtMethodNoArgAndNoReturn("abort", abort);

    rtMethod1ArgAndNoReturn("appendBuffer", appendBuffer, rtObjectRef);

    rtMethod2ArgAndNoReturn("remove", remove, double, double);


    rtError appendBuffer(rtObjectRef buffer);

    rtError getAppendWindowEnd(double &v) const;

    rtError updateAppendWindowEnd(double const &v);

    rtError getAppendWindowStart(double &v) const;

    rtError updateAppendWindowStart(double const &v);

    rtError getAudioTracks(rtObjectRef &v) const;

    rtError getBuffered(rtObjectRef &v) const;

    rtError getMode(int32_t &v) const;

    rtError setMode(int32_t const &v);

    rtError getTextTracks(rtObjectRef &v) const;

    rtError getVideoTracks(rtObjectRef &v) const;


    rtError getTimestampOffset(double &v) const;

    rtError getUpdating(bool &v) const;

  private:
    SourceBuffer(MediaSource *);


    void stop();

    const char *activeDOMObjectName() const;

    bool canSuspendForDocumentSuspension() const;

    void sourceBufferPrivateDidReceiveInitializationSegment(const InitializationSegment &);

    void sourceBufferPrivateDidReceiveSample(MediaSample &);

    bool sourceBufferPrivateHasAudio() const;

    bool sourceBufferPrivateHasVideo() const;

    void sourceBufferPrivateReenqueSamples(const rtString &trackID);

    void sourceBufferPrivateDidBecomeReadyForMoreSamples(const rtString &trackID);

    MediaTime sourceBufferPrivateFastSeekTimeForMediaTime(const MediaTime &, const MediaTime &negativeThreshold,
                                                          const MediaTime &positiveThreshold);

    void sourceBufferPrivateAppendComplete(AppendResult);

    void sourceBufferPrivateDidReceiveRenderingError(int errorCode);

    void audioTrackEnabledChanged(AudioTrack &);

    void videoTrackSelectedChanged(VideoTrack &);

    void textTrackKindChanged(TextTrack &);

    void textTrackModeChanged(TextTrack &);


    bool isRemoved() const;


    void appendBufferInternal(const unsigned char *, unsigned);

    void appendBufferTimerFired();

    void resetParserState();

    void setActive(bool);

    bool validateInitializationSegment(const InitializationSegment &);

    void reenqueueMediaForTime(TrackBuffer &, const rtString &trackID, const MediaTime &);

    void provideMediaData(TrackBuffer &, const rtString &trackID);

    void didDropSample();

    void evictCodedFrames(size_t newDataSize);

    size_t maximumBufferSize() const;

    void monitorBufferingRate();

    void removeTimerFired();

    void removeCodedFrames(const MediaTime &start, const MediaTime &end, bool keepDecodeQueue);

    size_t extraMemoryCost() const;

    void reportExtraMemoryAllocated();

    void updateBufferedFromTrackBuffers();

    void appendError(bool);

    bool hasAudio() const;


    MediaSource *m_source;
    AppendMode m_mode{AppendMode::Segments};

    std::vector<unsigned char> m_pendingAppendData;


    BaseTrackList *m_videoTracks;
    BaseTrackList *m_audioTracks;
    BaseTrackList *m_textTracks;

    std::vector<rtString> m_videoCodecs;
    std::vector<rtString> m_audioCodecs;
    std::vector<rtString> m_textCodecs;

    MediaTime m_timestampOffset;
    MediaTime m_appendWindowStart;
    MediaTime m_appendWindowEnd;

    MediaTime m_groupStartTimestamp;
    MediaTime m_groupEndTimestamp;

    std::map<rtString, TrackBuffer> m_trackBufferMap;

    TimeRanges *m_buffered;
    bool m_bufferedDirty{true};

    enum AppendStateType {
      WaitingForSegment, ParsingInitSegment, ParsingMediaSegment
    };
    AppendStateType m_appendState;

    double m_timeOfBufferingMonitor;
    double m_bufferedSinceLastMonitor{0};
    double m_averageBufferRate{0};

    size_t m_reportedExtraMemoryCost{0};

    MediaTime m_pendingRemoveStart;
    MediaTime m_pendingRemoveEnd;


    bool m_updating{false};
    bool m_receivedFirstInitializationSegment{false};
    bool m_active{false};
    bool m_bufferFull{false};
    bool m_shouldGenerateTimestamps{false};

    static size_t maxBufferSizeVideo;
    static size_t maxBufferSizeAudio;
    static size_t maxBufferSizeText;
  };

} // namespace WebCore


