#include "MSEBaseTrack.h"
#include "SourceBuffer.h"


MSEBaseTrackList::MSEBaseTrackList()
{
}

rtError MSEBaseTrackList::length(int &c) const
{
  c = (int)mTracks.size();
  return RT_OK;
}

rtError MSEBaseTrackList::Get(uint32_t i, rtValue* value) const
{
  if (i >= mTracks.size()) 
  {
    return RT_PROP_NOT_FOUND;
  }
  *value = mTracks[i];
  return RT_OK;
}

rtError MSEBaseTrackList::Set(uint32_t i, const rtValue* value)
{
  if (i >= mTracks.size()) 
  {
    return RT_PROP_NOT_FOUND;
  }
  mTracks[i] = (MSEBaseTrack*)value->getVoidPtr();
  return RT_OK;
}

rtError MSEBaseTrackList::getTrackById(rtString id, rtObjectRef &out)
{
  // TODO
  return RT_PROP_NOT_FOUND;
}

void MSEBaseTrackList::onChange()
{
  mEmit.send("onchange");
}

void MSEBaseTrackList::onAddTrack()
{
  mEmit.send("onaddtrack");
}

void MSEBaseTrackList::onRemoveTrack()
{
  mEmit.send("onremovetrack");
}

VideoTrackList::VideoTrackList(): mSelectedIndex(-1)
{
}

rtError VideoTrackList::getSelectedIndex(int &v) const
{
  v = mSelectedIndex;
  return RT_OK;
}

rtDefineObject(MSEBaseTrackList, MSEBaseObject)
rtDefineProperty(MSEBaseTrackList, length)
rtDefineMethod(MSEBaseTrackList, getTrackById)

rtDefineObject(AudioTrackList, MSEBaseTrackList)

rtDefineObject(VideoTrackList, MSEBaseTrackList)
rtDefineProperty(VideoTrackList, selectedIndex)

rtDefineObject(TextTrackList, MSEBaseTrackList)