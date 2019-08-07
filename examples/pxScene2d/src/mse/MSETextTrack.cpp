#include "TextTrack.h"
#include "SourceBuffer.h"


TextTrack::TextTrack(SourceBuffer *buffer) : MSEBaseTrack(buffer), mMode(""), mInBandMetadataTrackDispatchType("")
{

}

rtError TextTrack::getMode(rtString &v) const
{
  v = mMode;
  return RT_OK;
}

rtError TextTrack::setMode(rtString const &v)
{
  mMode = v;
  return RT_OK;
}


rtError TextTrack::getInBandMetadataTrackDispatchType(rtString &v) const
{

  v = mInBandMetadataTrackDispatchType;
  return RT_OK;
}


rtError TextTrack::getActiveCues(rtObjectRef &v) const
{
  // TODO
  return RT_OK;
}

rtError TextTrack::getCues(rtObjectRef &v) const
{
  // TODO
  return RT_OK;
}


rtError TextTrack::addCue(rtObjectRef cue)
{
  // TODO
  return RT_OK;
}

rtError TextTrack::removeCue(rtObjectRef cue)
{
  // TODO
  return RT_OK;
}


rtDefineObject(TextTrack, MSEBaseTrack)
rtDefineMethod(TextTrack, addCue)
rtDefineMethod(TextTrack, removeCue)