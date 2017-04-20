#include "pxGraphic.h"
#include "pxContext.h"
#include "stdio.h"

extern pxContext context;

pxGraphic::pxGraphic(pxScene2d *scene) : pxObject(scene), mLineWidth(0) 
{
  float f[4] = {0, 0, 0, 1};
  float l[4] = {1, 1, 1, 1};

  setFillColor(f);
  setLineColor(l);
}

void pxGraphic::draw() 
{

  if (!strcmp(mType, "lines")) 
  {
    if (mLineWidth <= 0) 
    {
      return;
    }
    drawLines();
  } 
  else if (!strcmp(mType, "polygon"))
  {
    drawPolygon();
  }
}

void pxGraphic::drawLines() 
{
  if (mVertNumber < 2) 
  {
    return;
  }
  context.drawLines(mVerts,mVertNumber,mLineColor,mLineWidth);
}

void pxGraphic::drawPolygon() 
{
  if (mVertNumber < 3) 
  {
    return;
  }
  context.drawPolygon(mVerts, mVertNumber, mLineWidth, mFillColor, mLineColor);
}


rtDefineObject(pxGraphic, pxObject);
rtDefineProperty(pxGraphic, fillColor);
rtDefineProperty(pxGraphic, lineColor);
rtDefineProperty(pxGraphic, lineWidth);
rtDefineProperty(pxGraphic, type);
rtDefineProperty(pxGraphic, points);

