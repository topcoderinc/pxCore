#pragma once

#include "MSEBaseObject.h"

namespace WebCore {

  class VideoPlaybackQuality : public MSEBaseObject {
  public:
    static VideoPlaybackQuality *
    create(double creationTime, unsigned long totalVideoFrames, unsigned long droppedVideoFrames,
           unsigned long corruptedVideoFrames, double totalFrameDelay);

    double creationTime() const
    { return m_creationTime; }

    unsigned long totalVideoFrames() const
    { return m_totalVideoFrames; }

    unsigned long droppedVideoFrames() const
    { return m_droppedVideoFrames; }

    unsigned long corruptedVideoFrames() const
    { return m_corruptedVideoFrames; }

    double totalFrameDelay() const
    { return m_totalFrameDelay; }


    rtDeclareObject(VideoPlaybackQuality, MSEBaseObject);
    rtReadOnlyProperty(creationTime, creationTime, double);
    rtReadOnlyProperty(totalVideoFrames, totalVideoFrames, uint64_t);
    rtReadOnlyProperty(droppedVideoFrames, droppedVideoFrames, uint64_t);
    rtReadOnlyProperty(corruptedVideoFrames, corruptedVideoFrames, uint64_t);
    rtReadOnlyProperty(totalFrameDelay, totalFrameDelay, double);

    rtError creationTime(double &v) const
    {
      v = m_creationTime;
      return RT_OK;
    }

    rtError totalVideoFrames(uint64_t &v) const
    {
      v = m_totalVideoFrames;
      return RT_OK;
    }

    rtError droppedVideoFrames(uint64_t &v) const
    {
      v = m_droppedVideoFrames;
      return RT_OK;
    }

    rtError corruptedVideoFrames(uint64_t &v) const
    {
      v = m_corruptedVideoFrames;
      return RT_OK;
    }

    rtError totalFrameDelay(double &v) const
    {
      v = m_totalFrameDelay;
      return RT_OK;
    }

  protected:
    VideoPlaybackQuality(double creationTime, unsigned long totalVideoFrames, unsigned long droppedVideoFrames,
                         unsigned long corruptedVideoFrames, double totalFrameDelay);

    double m_creationTime;
    unsigned long m_totalVideoFrames;
    unsigned long m_droppedVideoFrames;
    unsigned long m_corruptedVideoFrames;
    double m_totalFrameDelay;
  };

} // namespace WebCore
