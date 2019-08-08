#include "AudioTrack.h"


namespace WebCore {
  AudioTrack::AudioTrack(SourceBuffer *buffer) : BaseTrack(buffer)
  {

  }


  rtError AudioTrack::getEnabled(bool &v) const
  {
    v = mEnabled;
    return RT_OK;
  }

  rtError AudioTrack::setEnabled(bool const &v)
  {
    mEnabled = v;
    return RT_OK;
  }


  rtDefineObject(AudioTrack, BaseTrack)
  rtDefineProperty(AudioTrack, enabled)
}