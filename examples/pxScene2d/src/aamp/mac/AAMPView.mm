#include "aamp/AAMPView.h"
#include "cocoa_window.h"
#include "pxObject.h"
#include <Cocoa/Cocoa.h>

extern void *gWindow;

static NSView *aampNativeView;

AAMPView::AAMPView()
{
  auto window = (NSWindow *) gWindow;

  /**
   * create subview
   */
  NSView *view = [[NSView alloc] init];
  [view setWantsLayer:YES];
  [window.contentView addSubview:view];
  aampNativeView = view;
}

AAMPView::~AAMPView()
{
  rtLogInfo("delete mAAMPView ...");
  [aampNativeView removeFromSuperview];
}

guintptr getWindowContentView()
{
  return (guintptr) aampNativeView;
}

int createAndRunCocoaWindow()
{
  rtLogError("createAndRunCocoaWindow didn't implement !");
  return 0;
}

void AAMPView::setSize(float x, float y, float w, float h)
{
  [aampNativeView setFrame:NSMakeRect(x, y, w, h)];
}
