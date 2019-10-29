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

// Buffer.h

#include "rtBuffer.h"
#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include "rtLog.h"
#include <algorithm>

rtBuffer::rtBuffer(): mData(NULL), mDataSize(0)
{
}

rtBuffer::rtBuffer(int dataSize): mDataSize(dataSize)
{
  mData = (char*)malloc(dataSize);
}

rtBuffer::rtBuffer(char *data, int dataSize): rtBuffer(dataSize)
{
  memcpy(mData, data, dataSize);
}

rtBuffer::rtBuffer(const rtBuffer& other): rtBuffer(other.mData, other.mDataSize)
{
}

rtBuffer::~rtBuffer()
{
  if (mData) {
    free(mData);
    mData = NULL;
  }
}

bool rtBuffer::operator==(const rtBuffer &other) const
{
  return mData == other.mData && mDataSize == other.mDataSize;
}

rtBuffer &rtBuffer::operator=(const rtBuffer &other)
{
  if (this != &other) {
    rtBuffer tmp(other);
    swap(tmp);
  }
  return *this;
}

void rtBuffer::swap(rtBuffer &other)
{
  std::swap(mData, other.mData);
  std::swap(mDataSize, other.mDataSize);
}