/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/
/**
 * @file cocoa_window.mm
 * Advanced Adaptive Media Player (AAMP)
 */

#import <Cocoa/Cocoa.h>
#import "cocoa_window.h"

@interface VideoWindow: NSWindow <NSApplicationDelegate>
{
    gboolean m_windowActive;
}
- (gboolean) isActive;
- (id) initializeWindow:(NSRect) cordinates;
@end

@implementation VideoWindow

- (id) initializeWindow:(NSRect)cordinates
{
    self = [super initWithContentRect: cordinates
                            styleMask: (NSWindowStyleMaskClosable | NSWindowStyleMaskTitled | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable)
                              backing: NSBackingStoreBuffered defer: NO screen: nil];

    [[NSApplication sharedApplication] setDelegate:self];
    [self setReleasedWhenClosed:NO];
    m_windowActive = TRUE;
    [self setTitle:@"AAMP Test Player"];
    return self;
}

- (BOOL) windowShouldClose:(id)sender
{
    m_windowActive = FALSE;
    return YES;
}

- (void) applicationDidFinishLaunching: (NSNotification *)note
{
    [self makeMainWindow];
    [self center];
    [self orderFront:self];
}

- (gboolean) isActive
{
    return m_windowActive;
}

@end

static VideoWindow *gCocoaWindow=nil;

guintptr getWindowContentView()
{
    return (guintptr)[gCocoaWindow contentView];
}

int createAndRunCocoaWindow()
{
    NSRect windowCordinates;
    windowCordinates.size.width = 640;
    windowCordinates.size.height = 480;
    windowCordinates.origin.x = 0;
    windowCordinates.origin.y = 0;

    [NSApplication sharedApplication];
    gCocoaWindow = [[VideoWindow alloc] initializeWindow:windowCordinates];

    [gCocoaWindow orderFront:gCocoaWindow];
    while ([gCocoaWindow isActive])
    {
        NSEvent *winEvent = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate dateWithTimeIntervalSinceNow:1] inMode:NSDefaultRunLoopMode dequeue:YES];
        if (winEvent)
            [NSApp sendEvent:winEvent];
    }
    return 0;
}
