#ifndef PXSCENE_VIDEOTRACK_H
#define PXSCENE_VIDEOTRACK_H

#include "BaseTrack.h"

namespace WebCore {
/**
 * Video track class
 * https://developer.mozilla.org/en-US/docs/Web/API/VideoTrack
 */
  class VideoTrack : public BaseTrack {
  public:
    rtDeclareObject(VideoTrack, MSEBaseTrack);


    rtProperty(selected, getSelected, setSelected, bool);


    rtError getSelected(bool &v) const;

    rtError setSelected(bool const &v);


  private:
    bool mSelected;
  };
}

#endif //PXSCENE_VIDEOTRACK_H
