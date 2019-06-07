#include "aamp/pxAAMPPlayer.h"
#include "cocoa_window.h"
#include <Cocoa/Cocoa.h>

static NSWindow *aampWindow = nullptr;

/**
 * prepare aamp native window view
 */
void prepareAAMP()
{
  if (!aampWindow) {
    if ([NSThread isMainThread]) {
      createAndRunCocoaWindow();
    } else {
      dispatch_async(dispatch_get_main_queue(), ^{
        createAndRunCocoaWindow();
      });
    }
  }
}

/**
 * get native view
 * @return the nsview
 */
guintptr getWindowContentView()
{
  return (guintptr) [aampWindow contentView];
}


/**
 * create cocco window
 * @return
 */
int createAndRunCocoaWindow()
{
  // HACK, create 1x1 window, and hide it
  // WHY need this
  // gstreamer don't support offscreen render, the only way is create window and set overlay window
  // then hide the window
  NSRect frame = NSMakeRect(0, 0, 1, 1);
  NSWindow *window = [[[NSWindow alloc] initWithContentRect:frame
                                                  styleMask:NSBorderlessWindowMask
                                                    backing:NSBackingStoreBuffered
                                                      defer:NO] autorelease];
  [window setBackgroundColor:[NSColor blueColor]];
  [window makeKeyAndOrderFront:[NSApplication sharedApplication]];
  [[window contentView] setWantsLayer:YES];
  [window contentView].alphaValue = 0;
  aampWindow = window;
  return 0;
}
