#include "AudioTrack.h"

AudioTrack::AudioTrack(SourceBuffer *buffer) : MSEBaseTrack(buffer)
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


rtDefineObject(AudioTrack, MSEBaseTrack)