#include "helper.h"
#include <map>
#include <Cocoa/Cocoa.h>

std::map<guintptr, NSView*> __viewMap;

extern void *gWindow;
guintptr contentView(guintptr holder)
{
    if(__viewMap[holder])
    {
        return (guintptr)(__viewMap[holder]);
    }
    
    auto window = (NSWindow *) gWindow;
    NSView *view = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 1, 1)];
    [window.contentView addSubview:view];
    view.wantsLayer = true;
    view.alphaValue = 0.0;
    
    __viewMap[holder] = view;
    return (guintptr)view;
}

void releaseView(guintptr holder)
{
    if(__viewMap[holder]){
        [__viewMap[holder] removeFromSuperview];
        __viewMap.erase(holder);
    }
}
