#include "VideoTrack.h"


VideoTrack::VideoTrack(SourceBuffer *buffer) : AAMPBaseTrack(buffer), mSelected(false)
{

}

rtError VideoTrack::getSelected(bool &v) const
{
  v = mSelected;
  return RT_OK;
}

rtError VideoTrack::setSelected(bool const &v)
{
  mSelected = v;
  return RT_OK;
}

rtDefineObject(VideoTrack, AAMPBaseTrack)

