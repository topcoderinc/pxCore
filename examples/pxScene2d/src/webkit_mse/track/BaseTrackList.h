
#pragma once

#include "../MSEBaseObject.h"
#include "BaseTrack.h"

namespace WebCore {


  class BaseTrackList : public MSEBaseObject {
  public:


    BaseTrack *getTrackById(const rtString &) const;

    BaseTrack *get(unsigned index) const;

    BaseTrack *lastItem() const
    {
      if (mTracks.size() <= 0) {
        return nullptr;
      }
      return mTracks[mTracks.size() - 1];
    }

    void append(BaseTrack *track);

    void remove(SourceBuffer *buffer);

    size_t size() const
    {
      return mTracks.size();
    }

    rtDeclareObject(BaseTrackList, MSEBaseObject);

    rtMethod1ArgAndReturn("getItem", getItem, uint32_t, rtObjectRef);

    rtMethod1ArgAndReturn("getTrackById", getTrackById, rtString, rtObjectRef);

    rtReadOnlyProperty(length, getLength, uint32_t);

    rtError getLength(uint32_t &v) const
    {
      v = mTracks.size();
      return RT_OK;
    }

    rtError getItem(uint32_t index, rtObjectRef &v)
    {
      v = get(index);
      return RT_OK;
    }

    rtError getTrackById(rtString id, rtObjectRef &v)
    {
      v = getTrackById(id);
      return RT_OK;
    }

  private:
    std::vector<BaseTrack *> mTracks;
  };

} // namespace WebCore


