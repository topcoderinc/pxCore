

#ifndef PXSCENE_MSEBASETRACK_H
#define PXSCENE_MSEBASETRACK_H

#include "MSEBaseObject.h"

class SourceBuffer;

/**
 * mse base track class, contains common properties
 */
class MSEBaseTrack : public MSEBaseObject {
public:

  MSEBaseTrack(SourceBuffer *buffer);

  rtDeclareObject(MSEBaseTrack, MSEBaseObject);

  rtProperty(id, getId, setId, rtString);

  rtProperty(kind, getKind, setKind, rtString);

  rtProperty(label, getLabel, setLabel, rtString);

  rtProperty(language, getLanguage, setLanguage, rtString);

  rtReadOnlyProperty(sourceBuffer, getSourceBuffer, rtObjectRef);

  rtError getId(rtString &v) const;

  rtError setId(rtString const &v);

  rtError getKind(rtString &v) const;

  rtError setKind(rtString const &v);


  rtError getLabel(rtString &v) const;

  rtError setLabel(rtString const &v);

  rtError getLanguage(rtString &v) const;

  rtError setLanguage(rtString const &v);

  rtError getSourceBuffer(rtObjectRef &v) const;

protected:
  SourceBuffer *mBuffer;
  rtString mId;
  rtString mKind;
  rtString mLabel;
  rtString mLanguage;
};


#endif
