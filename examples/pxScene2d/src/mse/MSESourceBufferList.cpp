#include "MSESourceBufferList.h"
#include "MSEUtils.h"
#include <algorithm>

MSESourceBufferList::MSESourceBufferList()
{
  mBuffers.clear();
}

rtError MSESourceBufferList::getSourceBuffer(rtObjectRef &v) const
{
  rtArrayObject *arr = vectorToRTArr(mBuffers);
  v = arr;
  return RT_OK;
}

void MSESourceBufferList::add(MSESourceBuffer *buffer)
{
  mBuffers.push_back(buffer);
  mEmit.send("addsourcebuffer", buffer);
}

void MSESourceBufferList::remove(MSESourceBuffer *buffer)
{
  /*auto it = std::find(mBuffers.begin(), mBuffers.end(), buffer);
  if (it != mBuffers.end()) {
    mBuffers.erase(it);
  }*/
  std::remove(mBuffers.begin(), mBuffers.end(), buffer);
  mEmit.send("removesourcebuffer", buffer);
}

rtError MSESourceBufferList::length(int &c) const
{
  c = (int)mBuffers.size();

  return RT_OK;
}

rtError MSESourceBufferList::Get(uint32_t idx, rtValue* value) const
{
  if (idx >= mBuffers.size()) 
  {
    return RT_PROP_NOT_FOUND;
  }
  *value = mBuffers[idx];
  return RT_OK;
}

rtDefineObject(MSESourceBufferList, MSEBaseObject)
rtDefineMethod(MSESourceBufferList, getSourceBuffer)
rtDefineProperty(MSESourceBufferList, length)

