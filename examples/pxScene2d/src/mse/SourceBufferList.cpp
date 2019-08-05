#include "SourceBufferList.h"
#include "MSEUtils.h"
#include <algorithm>

SourceBufferList::SourceBufferList()
{
  mBuffers.clear();
}

rtError SourceBufferList::getSourceBuffer(rtObjectRef &v) const
{
  rtArrayObject *arr = vectorToRTArr(mBuffers);
  v = arr;
  return RT_OK;
}

void SourceBufferList::add(SourceBuffer *buffer)
{
  mBuffers.push_back(buffer);
  mEmit.send("onaddsourcebuffer", buffer);
}

void SourceBufferList::remove(SourceBuffer *buffer)
{
  /*auto it = std::find(mBuffers.begin(), mBuffers.end(), buffer);
  if (it != mBuffers.end()) {
    mBuffers.erase(it);
  }*/
  std::remove(mBuffers.begin(), mBuffers.end(), buffer);
  mEmit.send("onremovesourcebuffer", buffer);
}

rtError SourceBufferList::length(int &c) const
{
  c = (int)mBuffers.size();

  return RT_OK;
}

rtError SourceBufferList::Get(uint32_t idx, rtValue* value) const
{
  if (idx >= mBuffers.size()) 
  {
    return RT_PROP_NOT_FOUND;
  }
  *value = mBuffers[idx];
  return RT_OK;
}

rtDefineObject(SourceBufferList, MSEBaseObject)
rtDefineMethod(SourceBufferList, getSourceBuffer)
rtDefineProperty(SourceBufferList, length)

