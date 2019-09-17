#include "MSEVideoTrack.h"


MSEVideoTrack::MSEVideoTrack(MSESourceBuffer *buffer) : MSEBaseTrack(buffer), mSelected(false)
{

}

rtError MSEVideoTrack::getSelected(bool &v) const
{
  v = mSelected;
  return RT_OK;
}

rtError MSEVideoTrack::setSelected(bool const &v)
{
  mSelected = v;
  return RT_OK;
}

rtDefineObject(MSEVideoTrack, MSEBaseTrack)
