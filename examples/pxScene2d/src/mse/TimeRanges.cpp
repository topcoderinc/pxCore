#include "TimeRanges.h"

rtDefineObject(TimeRanges, rtArrayObject)
rtDefineProperty(TimeRanges, length)
rtDefineMethod(TimeRanges, start)
rtDefineMethod(TimeRanges, end)

rtError TimeRanges::length(int &c) const
{
  c = (int)mRanges.size();
  return RT_OK;
}

rtError TimeRanges::start(int index, double &v) const
{
  if (index >= mRanges.size())
  {
    return RT_PROP_NOT_FOUND;
  }
  v = mRanges[index].first;
  return RT_OK;
}

rtError TimeRanges::end(int index, double &v) const
{
  if (index >= mRanges.size())
  {
    return RT_PROP_NOT_FOUND;
  }
  v = mRanges[index].second;
  return RT_OK;
}