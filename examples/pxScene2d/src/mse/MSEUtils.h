
#ifndef PXSCENE_MSEUTILS_H
#define PXSCENE_MSEUTILS_H

#include "rtObject.h"
#include "MSEBaseObject.h"
#include <vector>

/**
 * vector array to rt arr
 * @param items the vector items
 * @return the rt array
 */
rtArrayObject *vectorToRTArr(const std::vector<MSEBaseObject *> &items);


/**
 * fill vector from rtArr
 * @param rtArr  the rtArr
 * @param items the vector
 */
void fillVectorFromRTArr(rtObjectRef const &rtArr, std::vector<MSEBaseObject *> &items);

#endif
