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

// pxMatrix4T.cpp

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif
#include <math.h>
#include "pxMatrix4T.h"

#ifndef _GLIBCXX_HAVE_SINCOS
void sincos(double x, double *s, double *c) {
  *s = sin(x);
  *c = cos(x);
}
#endif

#ifndef _GLIBCXX_HAVE_SINCOSF
void sincosf(float x, float *s, float *c) {
  *s = static_cast<float>(sin(x));
  *c = static_cast<float>(cos(x));
}
#endif
