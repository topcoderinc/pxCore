#include "SourceBufferList.h"
#include "MSEUtils.h"

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
  std::remove(mBuffers.begin(), mBuffers.end(), buffer);
  mEmit.send("onremovesourcebuffer", buffer);
}

rtDefineObject(SourceBufferList, MSEBaseObject)
rtDefineMethod(SourceBufferList, getSourceBuffer)

