
#include "MSEUtils.h"

rtArrayObject *vectorToRTArr(const std::vector<MSEBaseObject *> &items)
{
  auto *rtArr = new rtArrayObject();
  int index = 0;
  for (auto &item : items) {
    rtValue c(item);
    rtArr->Set(index++, &c);
  }
  return rtArr;
}

void fillVectorFromRTArr(rtObjectRef const &rtArr, std::vector<MSEBaseObject *> &items)
{
  auto *arr = (rtArrayObject *) rtArr.getPtr();
  for (uint32_t i = 0, l = arr->length(); i < l; ++i) {
    rtValue item;
    if (arr->Get(i, &item) == RT_OK && !item.isEmpty()) {
      rtObjectRef track;
      item.getObject(track);
      items.push_back(dynamic_cast<MSEBaseObject *>(track.getPtr()));
    }
  }
}
