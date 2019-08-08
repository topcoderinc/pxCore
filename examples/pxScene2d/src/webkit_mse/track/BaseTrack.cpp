#include "BaseTrack.h"
//#include "SourceBuffer.h"


namespace WebCore {

  class SourceBuffer : public MSEBaseObject {

  };

  BaseTrack::BaseTrack(SourceBuffer *buffer) :
      mBuffer(buffer), mId(""), mKind(""), mLabel(""), mLanguage("")
  {

  }

  rtError BaseTrack::getId(rtString &v) const
  {
    v = mId;
    return RT_OK;
  }

  rtError BaseTrack::setId(rtString const &v)
  {
    mId = v;
    return RT_OK;
  }

  rtError BaseTrack::getKind(rtString &v) const
  {
    v = mKind;
    return RT_OK;
  }

  rtError BaseTrack::setKind(rtString const &v)
  {
    mKind = v;
    return RT_OK;
  }

  rtError BaseTrack::getLabel(rtString &v) const
  {
    v = mLabel;
    return RT_OK;
  }

  rtError BaseTrack::setLabel(rtString const &v)
  {
    mLabel = v;
    return RT_OK;
  }

  rtError BaseTrack::getLanguage(rtString &v) const
  {
    v = mLanguage;
    return RT_OK;
  }

  rtError BaseTrack::setLanguage(rtString const &v)
  {
    mLanguage = v;
    return RT_OK;
  }

  rtError BaseTrack::getSourceBuffer(rtObjectRef &v) const
  {
    v = mBuffer;
    return RT_OK;
  }

  rtDefineObject(BaseTrack, MSEBaseObject)
  rtDefineProperty(BaseTrack, id)
  rtDefineProperty(BaseTrack, kind)
  rtDefineProperty(BaseTrack, label)
  rtDefineProperty(BaseTrack, language)
}