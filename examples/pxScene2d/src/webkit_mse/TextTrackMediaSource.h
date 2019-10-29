#pragma once


#include "track/TextTrack.h"

namespace WebCore {

  class SourceBuffer;

  class TextTrackMediaSource {
  public:
    static SourceBuffer *sourceBuffer(TextTrack &track)
    { return track.sourceBuffer(); }
  };

} // namespace WebCore

