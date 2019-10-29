#pragma once

#include "MSEBaseObject.h"


namespace WebCore {


  class pxHtmlVideo;

  class SourceBuffer;

  class SourceBufferList;

  class SourceBufferPrivate;

  class TimeRanges;


  //TODO use WTF PlatformTimeRanges
  class PlatformTimeRanges {

  };


  // TODO use WTF MediaTime
  class MediaTime {
  public:
    bool isValid() const
    {
      return false;
    }

    static MediaTime invalidTime()
    {
      return MediaTime::createWithDouble(0);
    }

    static MediaTime zeroTime()
    { return MediaTime::createWithDouble(0); }

    static MediaTime positiveInfiniteTime()
    { return MediaTime::createWithDouble(0); }

    static MediaTime createWithDouble(double v)
    {
      return MediaTime();
    }

    double toDouble() const
    {
      return 0;
    }
  };

  //TODO use WTF TimeRanges
  class TimeRanges : public MSEBaseObject {
  public:
    static TimeRanges *create()
    { return nullptr; }
  };

  class MediaSource final : public MSEBaseObject {
  public:


    MediaSource();

    static MediaSource *create();

    virtual ~MediaSource();

    void addedToRegistry();

    void removedFromRegistry();

    void openIfInEndedState();

    bool isOpen() const;

    bool isClosed() const;

    bool isEnded() const;

    void sourceBufferDidChangeActiveState(SourceBuffer &, bool);

    enum class EndOfStreamError {
      Network, Decode
    };

    void streamEndedWithError(EndOfStreamError error);

    MediaTime duration() const;

    void durationChanged(const MediaTime &);

    std::unique_ptr<PlatformTimeRanges> buffered() const;

    bool attachToElement(pxHtmlVideo &);

    void detachFromElement(pxHtmlVideo &);

    void monitorSourceBuffers();

    bool isSeeking() const
    { return m_pendingSeekTime.isValid(); }

    TimeRanges *seekable();

    rtError setLiveSeekableRange(double start, double end);

    rtError clearLiveSeekableRange();

    rtError setDuration(const double &);

    void setDurationInternal(const MediaTime &);

    MediaTime currentTime() const;

    enum class ReadyState {
      Closed, Open, Ended
    };

    ReadyState readyState() const
    { return m_readyState; }

    rtError endOfStream(uint32_t err);

    pxHtmlVideo *mediaElement() const
    { return m_mediaElement; }

    SourceBufferList *sourceBuffers()
    { return m_sourceBuffers; }

    SourceBufferList *activeSourceBuffers()
    { return m_activeSourceBuffers; }

    SourceBuffer &addSourceBuffer(const rtString &type);

    void removeSourceBuffer(SourceBuffer &);

    static bool isTypeSupported(const rtString &type);


    bool hasPendingActivity() const;

    const MediaTime &currentTimeFudgeFactor();

    virtual bool hasBufferedTime(const MediaTime &);


    rtDeclareObject(MediaSource, MSEBaseObject);

    rtReadOnlyProperty(sourceBuffers, getSourceBuffers, rtObjectRef);

    rtReadOnlyProperty(activeSourceBuffers, getActiveSourceBuffers, rtObjectRef);

    rtReadOnlyProperty(readyState, getReadyState, int32_t);

    rtProperty(duration, getDuration, setDuration, double);

    rtMethod1ArgAndReturn("addSourceBuffer", addSourceBuffer, rtString, rtObjectRef);

    rtMethodNoArgAndNoReturn("clearLiveSeekableRange", clearLiveSeekableRange);

    rtMethod1ArgAndNoReturn("endOfStream", endOfStream, uint32_t);

    rtMethod1ArgAndNoReturn("removeSourceBuffer", removeSourceBuffer, rtObjectRef);

    rtMethod2ArgAndNoReturn("setLiveSeekableRange", setLiveSeekableRange, double, double);

    rtMethod1ArgAndReturn("isTypeSupported", isTypeSupported, rtString, bool);

    rtError getSourceBuffers(rtObjectRef &v) const;

    rtError getActiveSourceBuffers(rtObjectRef &v) const;

    rtError getReadyState(int32_t &v) const;

    rtError getDuration(double &v) const;

    rtError isTypeSupported(rtString type, bool &v);

    rtError addSourceBuffer(rtString type, rtObjectRef &v);

    rtError removeSourceBuffer(rtObjectRef buffer);

  private:

    void stop();

    bool canSuspendForDocumentSuspension() const;

    const char *activeDOMObjectName() const;

    void seekToTime(const MediaTime &);


    void setReadyState(ReadyState);

    void onReadyStateChange(ReadyState oldState, ReadyState newState);

    std::vector<PlatformTimeRanges> activeRanges() const;


    bool hasCurrentTime();

    bool hasFutureTime();

    void regenerateActiveSourceBuffers();

    void completeSeek();


    SourceBufferList *m_sourceBuffers;
    SourceBufferList *m_activeSourceBuffers;
    mutable std::unique_ptr<PlatformTimeRanges> m_buffered;
    std::unique_ptr<PlatformTimeRanges> m_liveSeekable;
    pxHtmlVideo *m_mediaElement{nullptr};
    MediaTime m_duration;
    MediaTime m_pendingSeekTime;
    ReadyState m_readyState{ReadyState::Closed};
  };

}

