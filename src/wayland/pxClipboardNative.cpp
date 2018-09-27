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

//  pxClipboardNative.cpp

#include <stdio.h>

#include "pxClipboardNative.h"

pxClipboardNative *pxClipboardNative::s_instance;


std::string pxClipboardNative::getString(std::string type)
{
    printf("pxClipboardNative::getString() - ENTER\n");
    
    // NSPasteboard*  pasteBoard = [NSPasteboard generalPasteboard];
    // NSString*        myString = [pasteBoard  stringForType: NSPasteboardTypeString];
    
    return std::string("(empty)");
}

void pxClipboardNative::setString(std::string type, std::string clip)
{
    printf("pxClipboardNative::setString() - ENTER\n");

    // NSString *stringToWrite = [NSString stringWithUTF8String: clip.c_str()];
    
    // NSPasteboard*  pasteBoard = [NSPasteboard generalPasteboard];

    // [pasteBoard declareTypes:[NSArray arrayWithObject:NSStringPboardType] owner:nil];
    // [pasteBoard setString:stringToWrite forType:NSStringPboardType];
}
