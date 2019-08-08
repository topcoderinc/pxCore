
#pragma once

#include "track/VideoTrack.h"

namespace WebCore {

  class VideoTrackMediaSource {
  public:
    static SourceBuffer *sourceBuffer(VideoTrack &track)
    {
      return track.sourceBuffer();
    }
  };

}