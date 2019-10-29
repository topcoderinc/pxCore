/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SourceBufferList.h"


#include "SourceBuffer.h"

namespace WebCore {

  SourceBufferList::SourceBufferList()
  {
  }

  SourceBufferList::~SourceBufferList()
  {
  }

  void SourceBufferList::add(SourceBuffer *buffer)
  {
    m_list.push_back(buffer);
    mEmit.send("onaddsourcebuffer");
  }

  void SourceBufferList::remove(SourceBuffer *buffer)
  {
    auto index = std::remove(m_list.begin(), m_list.end(), buffer);
    if (index == m_list.end())
      return;
    mEmit.send("onremovesourcebuffer");
  }

  void SourceBufferList::clear()
  {
    m_list.clear();
    mEmit.send("onremovesourcebuffer");
  }

  void SourceBufferList::swap(std::vector<SourceBuffer *> &other)
  {
    int changeInSize = other.size() - m_list.size();
    int addedEntries = 0;
    for (auto &sourceBuffer : other) {
      if (!this->contains(sourceBuffer))
        ++addedEntries;
    }
    int removedEntries = addedEntries - changeInSize;

    m_list.swap(other);

    if (addedEntries)
      mEmit.send("onaddsourcebuffer");
    if (removedEntries)
      mEmit.send("onremovesourcebuffer");
  }

  rtError SourceBufferList::getItem(uint32_t index, rtObjectRef &v)
  {
    v = get(index);
    return RT_OK;
  }

  rtDefineObject(SourceBufferList, MSEBaseObject)
  rtDefineMethod(SourceBufferList, getItem)
  rtDefineProperty(SourceBufferList, length)

} // namespace WebCore


