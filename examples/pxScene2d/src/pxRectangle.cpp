// pxCore CopyRight 2007-2015 John Robinson
// pxText.h

#include "pxRectangle.h"
#include "pxContext.h"

extern pxContext context;

void pxRectangle::draw()
{
  context.drawRect(mw, mh, mLineWidth, mFillColor, mLineColor, mRadius);
}

rtDefineObject(pxRectangle, pxObject);
rtDefineProperty(pxRectangle, fillColor);
rtDefineProperty(pxRectangle, lineColor);
rtDefineProperty(pxRectangle, lineWidth);
rtDefineProperty(pxRectangle, radius);
