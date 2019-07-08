#ifndef PXSCENE_TEXTTRACK_H
#define PXSCENE_TEXTTRACK_H

#include "rtObject.h"
#include "AAMPBaseTrack.h"

/**
 * TextTrack class for source buffer
 * https://developer.mozilla.org/en-US/docs/Web/API/TextTrack
 */
class TextTrack : public AAMPBaseTrack {
public:
  TextTrack(SourceBuffer *buffer);

  rtDeclareObject(TextTrack, AAMPBaseTrack);


  rtProperty(mode, getMode, setMode, rtString);

  rtReadOnlyProperty(inBandMetadataTrackDispatchType, getInBandMetadataTrackDispatchType, rtString);

  rtReadOnlyProperty(activeCues, getActiveCues, rtObjectRef);

  rtReadOnlyProperty(cues, getCues, rtObjectRef);

  rtMethod1ArgAndNoReturn("addCue", addCue, rtObjectRef);

  rtMethod1ArgAndNoReturn("removeCue", removeCue, rtObjectRef);

  rtError getMode(rtString &v) const;

  rtError setMode(rtString const &v);

  rtError getInBandMetadataTrackDispatchType(rtString &v) const;

  rtError getActiveCues(rtObjectRef &v) const;

  rtError getCues(rtObjectRef &v) const;

  rtError addCue(rtObjectRef cue);

  rtError removeCue(rtObjectRef cue);

protected:
  rtString mMode;
  rtString mInBandMetadataTrackDispatchType;
};


#endif //PXSCENE_TEXTTRACK_H
