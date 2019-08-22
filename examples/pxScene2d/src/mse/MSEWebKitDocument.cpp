
#include "MSEWebKitDocument.h"

#include "WebCore/config.h"
#include "MediaSource.h"
#include "HTMLVideoElement.h"
#include "WebCore/fileapi/Blob.h"
#include "WebCore/dom/Event.h"
#include "WebCore/dom/EventNames.h"

MSEWebkitDocument *MSEWebkitDocument::m_instance = NULL;

struct MSEWebkitDocumentImpl
{
  MSEWebkitDocumentImpl(): mDocument(WebCore::Document::create(WTF::URL())) {}
  Ref<WebCore::Document> mDocument;
};

MSEWebkitDocument &MSEWebkitDocument::get()
{
  if (!m_instance) {
    m_instance = new MSEWebkitDocument();
  }
  return *m_instance;
}

void MSEWebkitDocument::dispose()
{
  if (m_instance) {
    delete m_instance;
  }
  m_instance = nullptr;
}

MSEWebkitDocument::MSEWebkitDocument(): mImpl(new MSEWebkitDocumentImpl())
{
}


MSEWebkitDocument::~MSEWebkitDocument()
{
  if (mImpl) {
    delete mImpl;
    mImpl = nullptr;
  }
}

WebCore::Document &MSEWebkitDocument::getDocument()
{
  return mImpl->mDocument.get();
}
