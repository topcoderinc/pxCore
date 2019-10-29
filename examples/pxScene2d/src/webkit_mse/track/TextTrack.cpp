#include "TextTrack.h"

namespace WebCore {


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


  rtDefineObject(TextTrack, BaseTrack)
  rtDefineMethod(TextTrack, addCue)
  rtDefineMethod(TextTrack, removeCue)
  rtDefineProperty(TextTrack, mode)
  rtDefineProperty(TextTrack, inBandMetadataTrackDispatchType)
  rtDefineProperty(TextTrack, activeCues)
  rtDefineProperty(TextTrack, cues)
}