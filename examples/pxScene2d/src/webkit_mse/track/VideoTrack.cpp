#include "VideoTrack.h"

namespace WebCore {

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

  rtDefineObject(VideoTrack, BaseTrack)
  rtDefineProperty(VideoTrack, selected)

}