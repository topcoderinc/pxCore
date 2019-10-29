
#pragma once

#include "BaseTrackList.h"

namespace WebCore {


  BaseTrack *BaseTrackList::getTrackById(const rtString &) const
  {
    return nullptr;
  }

  BaseTrack *BaseTrackList::get(unsigned index) const
  {
    return nullptr;
  }

  void BaseTrackList::append(BaseTrack *track)
  {
    mEmit.send("onaddtrack");
    mEmit.send("onchange");
  }

  void BaseTrackList::remove(SourceBuffer *buffer)
  {
    mEmit.send("onremovetrack");
    mEmit.send("onchange");
  }

  rtDefineObject(BaseTrackList, MSEBaseObject)
  rtDefineMethod(BaseTrackList, getItem)
  rtDefineMethod(BaseTrackList, getTrackById)
  rtDefineProperty(BaseTrackList, length)


} // namespace WebCore


