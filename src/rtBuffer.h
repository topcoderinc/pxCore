/*

 pxCore Copyright 2005-2018 John Robinson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*/

// rtBuffer.h

#ifndef _RT_BUFFER_H
#define _RT_BUFFER_H

//#include "rtCore.h"
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>

class rtBuffer
{
public:
  rtBuffer();
  rtBuffer(int dataSize);
  rtBuffer(char *data, int dataSize);
  rtBuffer(const rtBuffer& other);
  ~rtBuffer();

  bool operator==(const rtBuffer &other) const;
  rtBuffer &operator=(const rtBuffer &other);

  void swap(rtBuffer &other);

  char *getData() const { return mData; }
  int   getSize() const { return mDataSize; }
  
private:
  char* mData;
  int   mDataSize;
};

#endif
