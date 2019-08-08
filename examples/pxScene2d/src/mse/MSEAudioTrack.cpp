#include "MSEAudioTrack.h"

MSEAudioTrack::MSEAudioTrack(MSESourceBuffer *buffer) : MSEBaseTrack(buffer)
{

}


rtError MSEAudioTrack::getEnabled(bool &v) const
{
  v = mEnabled;
  return RT_OK;
}

rtError MSEAudioTrack::setEnabled(bool const &v)
{
  mEnabled = v;
  return RT_OK;
}


rtDefineObject(MSEAudioTrack, MSEBaseTrack)