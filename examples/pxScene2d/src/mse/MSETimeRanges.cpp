#include "MSETimeRanges.h"

rtDefineObject(MSETimeRanges, rtArrayObject)
rtDefineProperty(MSETimeRanges, length)
rtDefineMethod(MSETimeRanges, start)
rtDefineMethod(MSETimeRanges, end)

rtError MSETimeRanges::length(int &c) const
{
  c = (int)mRanges.size();
  return RT_OK;
}

rtError MSETimeRanges::start(int index, double &v) const
{
  if (index >= mRanges.size())
  {
    return RT_PROP_NOT_FOUND;
  }
  v = mRanges[index].first;
  return RT_OK;
}

rtError MSETimeRanges::end(int index, double &v) const
{
  if (index >= mRanges.size())
  {
    return RT_PROP_NOT_FOUND;
  }
  v = mRanges[index].second;
  return RT_OK;
}