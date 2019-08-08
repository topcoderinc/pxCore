#ifndef PXSCENE_AUDIOTRACK_H
#define PXSCENE_AUDIOTRACK_H

#include "BaseTrack.h"

namespace WebCore {
/**
 * Audio track class
 * https://developer.mozilla.org/en-US/docs/Web/API/AudioTrack
 */
  class AudioTrack : public BaseTrack {
    AudioTrack(SourceBuffer *buffer);

    rtDeclareObject(AudioTrack, MSEBaseTrack);


    rtProperty(enabled, getEnabled, setEnabled, bool);


    rtError getEnabled(bool &v) const;

    rtError setEnabled(bool const &v);

  protected:
    bool mEnabled;
  };

}
#endif //PXSCENE_AUDIOTRACK_H
