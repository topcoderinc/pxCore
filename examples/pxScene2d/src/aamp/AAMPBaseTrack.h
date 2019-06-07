

#ifndef PXSCENE_AAMPBASETRACK_H
#define PXSCENE_AAMPBASETRACK_H

#include "AAMPBaseObject.h"

class SourceBuffer;

/**
 * aamp base track class, contains common properties
 */
class AAMPBaseTrack : public AAMPBaseObject {
public:

  AAMPBaseTrack(SourceBuffer *buffer);

  rtDeclareObject(AAMPBaseTrack, AAMPBaseObject);

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
