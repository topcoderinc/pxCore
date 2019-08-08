#include "VideoPlaybackQuality.h"

namespace WebCore {
  VideoPlaybackQuality *VideoPlaybackQuality::create(
      double creationTime,
      unsigned long totalVideoFrames, unsigned long droppedVideoFrames,
      unsigned long corruptedVideoFrames, double totalFrameDelay)
  {
    return new VideoPlaybackQuality(
        creationTime,
        totalVideoFrames,
        droppedVideoFrames,
        corruptedVideoFrames,
        totalFrameDelay);
  }

  VideoPlaybackQuality::VideoPlaybackQuality(double creationTime, unsigned long totalVideoFrames,
                                             unsigned long droppedVideoFrames, unsigned long corruptedVideoFrames,
                                             double totalFrameDelay)
      : m_creationTime(creationTime), m_totalVideoFrames(totalVideoFrames), m_droppedVideoFrames(droppedVideoFrames),
        m_corruptedVideoFrames(corruptedVideoFrames), m_totalFrameDelay(totalFrameDelay)
  {
  }

  rtDefineObject(VideoPlaybackQuality, MSEBaseObject)
  rtDefineProperty(VideoPlaybackQuality, creationTime)
  rtDefineProperty(VideoPlaybackQuality, totalVideoFrames)
  rtDefineProperty(VideoPlaybackQuality, droppedVideoFrames)
  rtDefineProperty(VideoPlaybackQuality, corruptedVideoFrames)
  rtDefineProperty(VideoPlaybackQuality, totalFrameDelay)
}
