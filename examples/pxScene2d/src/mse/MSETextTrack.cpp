#include "MSETextTrack.h"
#include "MSESourceBuffer.h"


MSETextTrack::MSETextTrack(MSESourceBuffer *buffer) : MSEBaseTrack(buffer), mMode(""), mInBandMetadataTrackDispatchType("")
{

}

rtError MSETextTrack::getMode(rtString &v) const
{
  v = mMode;
  return RT_OK;
}

rtError MSETextTrack::setMode(rtString const &v)
{
  mMode = v;
  return RT_OK;
}


rtError MSETextTrack::getInBandMetadataTrackDispatchType(rtString &v) const
{

  v = mInBandMetadataTrackDispatchType;
  return RT_OK;
}


rtError MSETextTrack::getActiveCues(rtObjectRef &v) const
{
  // TODO
  return RT_OK;
}

rtError MSETextTrack::getCues(rtObjectRef &v) const
{
  // TODO
  return RT_OK;
}


rtError MSETextTrack::addCue(rtObjectRef cue)
{
  // TODO
  return RT_OK;
}

rtError MSETextTrack::removeCue(rtObjectRef cue)
{
  // TODO
  return RT_OK;
}


rtDefineObject(MSETextTrack, MSEBaseTrack)
rtDefineMethod(MSETextTrack, addCue)
rtDefineMethod(MSETextTrack, removeCue)