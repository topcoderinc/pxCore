#include "AAMPBaseTrack.h"
#include "SourceBuffer.h"


AAMPBaseTrack::AAMPBaseTrack(SourceBuffer *buffer) :
    mBuffer(buffer), mId(""), mKind(""), mLabel(""), mLanguage("")
{

}

rtError AAMPBaseTrack::getId(rtString &v) const
{
  v = mId;
  return RT_OK;
}

rtError AAMPBaseTrack::setId(rtString const &v)
{
  mId = v;
  return RT_OK;
}

rtError AAMPBaseTrack::getKind(rtString &v) const
{
  v = mKind;
  return RT_OK;
}

rtError AAMPBaseTrack::setKind(rtString const &v)
{
  mKind = v;
  return RT_OK;
}

rtError AAMPBaseTrack::getLabel(rtString &v) const
{
  v = mLabel;
  return RT_OK;
}

rtError AAMPBaseTrack::setLabel(rtString const &v)
{
  mLabel = v;
  return RT_OK;
}

rtError AAMPBaseTrack::getLanguage(rtString &v) const
{
  v = mLanguage;
  return RT_OK;
}

rtError AAMPBaseTrack::setLanguage(rtString const &v)
{
  mLanguage = v;
  return RT_OK;
}

rtError AAMPBaseTrack::getSourceBuffer(rtObjectRef &v) const
{
  v = mBuffer;
  return RT_OK;
}

rtDefineObject(AAMPBaseTrack, AAMPBaseObject)
