#include "MSEBaseTrack.h"
#include "MSESourceBuffer.h"


MSEBaseTrack::MSEBaseTrack(MSESourceBuffer *buffer) :
    mBuffer(buffer), mId(""), mKind(""), mLabel(""), mLanguage("")
{

}

rtError MSEBaseTrack::getId(rtString &v) const
{
  v = mId;
  return RT_OK;
}

rtError MSEBaseTrack::setId(rtString const &v)
{
  mId = v;
  return RT_OK;
}

rtError MSEBaseTrack::getKind(rtString &v) const
{
  v = mKind;
  return RT_OK;
}

rtError MSEBaseTrack::setKind(rtString const &v)
{
  mKind = v;
  return RT_OK;
}

rtError MSEBaseTrack::getLabel(rtString &v) const
{
  v = mLabel;
  return RT_OK;
}

rtError MSEBaseTrack::setLabel(rtString const &v)
{
  mLabel = v;
  return RT_OK;
}

rtError MSEBaseTrack::getLanguage(rtString &v) const
{
  v = mLanguage;
  return RT_OK;
}

rtError MSEBaseTrack::setLanguage(rtString const &v)
{
  mLanguage = v;
  return RT_OK;
}

rtError MSEBaseTrack::getSourceBuffer(rtObjectRef &v) const
{
  v = mBuffer;
  return RT_OK;
}

rtDefineObject(MSEBaseTrack, MSEBaseObject)
