#pragma once

#include "track/AudioTrack.h"

namespace WebCore {
  class AudioTrackMediaSource {
  public:
    static SourceBuffer *sourceBuffer(AudioTrack &track)
    {
      return track.sourceBuffer();
    }
  };
}
