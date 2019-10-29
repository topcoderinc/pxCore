

#pragma once

#include "MSEBaseObject.h"


namespace WebCore {

  class SourceBuffer;

  class SourceBufferList : public MSEBaseObject {
  public:
    static SourceBufferList *create()
    {
      new SourceBufferList();
    }

    SourceBufferList();


    virtual ~SourceBufferList();

    unsigned long length() const
    { return m_list.size(); }

    SourceBuffer *get(unsigned long index) const
    { return (index < m_list.size()) ? m_list[index] : nullptr; }

    void add(SourceBuffer *);

    void remove(SourceBuffer *);

    bool contains(SourceBuffer *buffer)
    { return std::find(m_list.begin(), m_list.end(), buffer) != m_list.end(); }

    void clear();

    void swap(std::vector<SourceBuffer *> &);

    std::vector<SourceBuffer *>::iterator begin()
    {
      return m_list.begin();
    }

    std::vector<SourceBuffer *>::iterator end()
    { return m_list.end(); }


    rtDeclareObject(SourceBufferList, MSEBaseObject);

    rtMethod1ArgAndReturn("getItem", getItem, uint32_t, rtObjectRef);

    rtReadOnlyProperty(length, getLength, uint32_t);

    rtError getLength(uint32_t &v) const
    {
      v = m_list.size();
      return RT_OK;
    }

    rtError getItem(uint32_t index, rtObjectRef &v);


  private:

    std::vector<SourceBuffer *> m_list;
  };

} // namespace WebCore


