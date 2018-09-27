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

//  pxClipboard.h

#ifndef PX_CLIPBOARD_H
#define PX_CLIPBOARD_H


#ifdef WIN32
#include "win\pxClipboardNative.h"
#endif

class pxClipboard: public pxClipboardNative
{
public:
             pxClipboard()   {};
    virtual ~pxClipboard()   {};
};

#endif // PX_CLIPBOARD_H
