#ifndef PXSCENE_SOURCEBUFFERLIST_H
#define PXSCENE_SOURCEBUFFERLIST_H

#include "MSEBaseObject.h"
#include "SourceBuffer.h"
#include <vector>

/**
 * source buffer list class
 * https://developer.mozilla.org/en-US/docs/Web/API/SourceBufferList
 */
class SourceBufferList : public MSEBaseObject {
public:
  rtDeclareObject(SourceBufferList, MSEBaseObject);

  rtMethodNoArgAndReturn("SourceBuffer", getSourceBuffer, rtObjectRef);

  SourceBufferList();

  rtError getSourceBuffer(rtObjectRef &v) const;

  void add(SourceBuffer *buffer);

  void remove(SourceBuffer *buffer);

  // SourceBufferList.idl
  // readonly attribute unsigned long length;
  rtReadOnlyProperty(length, length, int);
  rtError length(int &c) const;

  //getter SourceBuffer item(unsigned long index);
  rtError Get(uint32_t /*i*/, rtValue* /*value*/) const override;

  //attribute EventHandler onaddsourcebuffer;
  //attribute EventHandler onremovesourcebuffer;

private:
  std::vector<MSEBaseObject *> mBuffers;
};


#endif //PXSCENE_SOURCEBUFFERLIST_H
