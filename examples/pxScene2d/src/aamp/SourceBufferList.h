#ifndef PXSCENE_SOURCEBUFFERLIST_H
#define PXSCENE_SOURCEBUFFERLIST_H

#include "AAMPBaseObject.h"
#include "SourceBuffer.h"
#include <vector>

/**
 * source buffer list class
 * https://developer.mozilla.org/en-US/docs/Web/API/SourceBufferList
 */
class SourceBufferList : public AAMPBaseObject {
public:
  rtDeclareObject(SourceBufferList, AAMPBaseObject);

  rtMethodNoArgAndReturn("SourceBuffer", getSourceBuffer, rtObjectRef);

  SourceBufferList();

  rtError getSourceBuffer(rtObjectRef &v) const;

  void add(SourceBuffer *buffer);

  void remove(SourceBuffer *buffer);

private:
  std::vector<AAMPBaseObject *> mBuffers;
};


#endif //PXSCENE_SOURCEBUFFERLIST_H
