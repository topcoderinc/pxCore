
#ifndef PXSCENE_AAMPVIEW_H
#define PXSCENE_AAMPVIEW_H


/**
 * Mac native aamp view
 */
class AAMPView {

public:
  /**
   * the aamp constructor
   */
  AAMPView();

  /**
   * the de constructor
   */
  ~AAMPView();

  /**
   * update view size
   * @param x pos x
   * @param y pos y
   * @param w width
   * @param h height
   */
  void setSize(float x, float y, float w, float h);
};


#endif
