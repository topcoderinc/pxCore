
#ifndef PXSCENE_MSEWEBKITDOCUMENT_H
#define PXSCENE_MSEWEBKITDOCUMENT_H

#include "rtObject.h"
#include <vector>

struct MSEWebkitDocumentImpl;

namespace WebCore
{
  class Document;
}

class MSEWebkitDocument {
public:
  MSEWebkitDocument();
  ~MSEWebkitDocument();

  static MSEWebkitDocument &get();
  static void dispose();

  WebCore::Document &getDocument();

private:
  MSEWebkitDocumentImpl *mImpl;

  static MSEWebkitDocument *m_instance;
};

#endif
