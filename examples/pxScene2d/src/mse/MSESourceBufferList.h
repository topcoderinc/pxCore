#ifndef PXSCENE_MSESourceBufferList_H
#define PXSCENE_MSESourceBufferList_H

#include "MSEBaseObject.h"
#include "MSESourceBuffer.h"
#include <vector>

/**
 * source buffer list class
 * https://developer.mozilla.org/en-US/docs/Web/API/MSESourceBufferList
 */
class MSESourceBufferList : public MSEBaseObject {
public:
  rtDeclareObject(MSESourceBufferList, MSEBaseObject);

  rtMethodNoArgAndReturn("SourceBuffer", getSourceBuffer, rtObjectRef);

  MSESourceBufferList();

  rtError getSourceBuffer(rtObjectRef &v) const;

  void add(MSESourceBuffer *buffer);

  void remove(MSESourceBuffer *buffer);

  // MSESourceBufferList.idl
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


#endif //PXSCENE_MSESourceBufferList_H
