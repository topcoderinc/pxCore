#include "MSETimeRanges.h"

#include "WebCore/config.h"
#include "WebCore/html/TimeRanges.h"

rtDefineObject(MSETimeRanges, rtArrayObject)
rtDefineProperty(MSETimeRanges, length)
rtDefineMethod(MSETimeRanges, start)
rtDefineMethod(MSETimeRanges, end)

MSETimeRanges::MSETimeRanges(const WebCore::TimeRanges &ranges)
{
  for (int i = 0; i < ranges.length(); ++i) {
    mRanges.push_back(std::make_pair(ranges.ranges().start(i).toFloat(), ranges.ranges().end(i).toFloat()));
  }
}

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