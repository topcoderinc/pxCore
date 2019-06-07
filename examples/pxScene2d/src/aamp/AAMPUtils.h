
#ifndef PXSCENE_AAMPUTILS_H
#define PXSCENE_AAMPUTILS_H

#include "rtObject.h"
#include "AAMPBaseObject.h"
#include <vector>

/**
 * vector array to rt arr
 * @param items the vector items
 * @return the rt array
 */
rtArrayObject *vectorToRTArr(const std::vector<AAMPBaseObject *> &items);


/**
 * fill vector from rtArr
 * @param rtArr  the rtArr
 * @param items the vector
 */
void fillVectorFromRTArr(rtObjectRef const &rtArr, std::vector<AAMPBaseObject *> &items);

#endif
