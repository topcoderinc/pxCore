#ifndef PXSCENE_MSEVideoTrack_H
#define PXSCENE_MSEVideoTrack_H

#include "MSEBaseTrack.h"
#include "MSESourceBuffer.h"

/**
 * Video track class
 * https://developer.mozilla.org/en-US/docs/Web/API/MSEVideoTrack
 */
class MSEVideoTrack : MSEBaseTrack {
public:
  rtDeclareObject(MSEVideoTrack, MSEBaseTrack);

  MSEVideoTrack(MSESourceBuffer *buffer);

  rtProperty(selected, getSelected, setSelected, bool);


  rtError getSelected(bool &v) const;

  rtError setSelected(bool const &v);


private:
  bool mSelected;
};


#endif //PXSCENE_MSEVideoTrack_H
