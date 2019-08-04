#ifndef PXSCENE_TIME_RANGES_H
#define PXSCENE_TIME_RANGES_H

#include "rtObject.h"

class TimeRanges : public rtObject {
public:
  rtDeclareObject(TimeRanges, rtObject);

  //readonly attribute unsigned long length;
  rtReadOnlyProperty(length, length, int);
  rtError length(int &c) const;

  //[MayThrowException] unrestricted double start(unsigned long index);
  rtMethod1ArgAndReturn(start, start, int, double);
  rtError start(int index, double &v) const;

  //[MayThrowException] unrestricted double end(unsigned long index);
  rtMethod1ArgAndReturn(end, end, int, double);
  rtError end(int index, double &v) const;

private:
  vector<std::pair<double, double>> mRanges;
};


#endif //PXSCENE_TIME_RANGES_H
