/*

pxCore Copyright 2005-2018 John Robinson

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#ifndef ENABLE_RT_NODE
#define ENABLE_RT_NODE
#endif

#include <sstream>

#define private public
#define protected public

#include "pxScene2d.h"
#include <string.h>
#include "pxIView.h"
#include "pxTimer.h"
#include "rtObject.h"
#include <rtRef.h>

#include "test_includes.h" // Needs to be included last

using namespace std;

extern rtScript script;

class pxSceneContainerLeakTest : public testing::Test
{
  public:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }

    void withParentRemovedGCNotHappenedTest()
    {
      startJsFile("onescene_with_parent.js");
      process();
      populateObjects();

      pxObject* sceneContainer = mSceneContainer[0];
      sceneContainer->remove();
      EXPECT_TRUE (sceneContainer->mRefCount > 1);
      EXPECT_TRUE (sceneContainer->parent() == NULL);
      script.collectGarbage();
    }

    void withParentRemovedGCHappenedTest()
    {
      startJsFile("onescene_with_parent.js");
      process();
      populateObjects();

      pxObject* sceneContainer = mSceneContainer[0];
      sceneContainer->AddRef();
      sceneContainer->remove();
      script.collectGarbage();
      pxSleepMS(3000);
      EXPECT_TRUE (sceneContainer->mRefCount == 1);
      script.collectGarbage();
    }

    void withoutParentRemovedGCNotHappenedTest()
    {
      startJsFile("onescene_with_parent.js");
      process();
      populateObjects();

      pxObject* sceneContainer = mSceneContainer[0];
      EXPECT_TRUE (sceneContainer->mRefCount > 1);
      EXPECT_TRUE (sceneContainer->parent() != NULL);
      script.collectGarbage();
    }

    void withoutParentRemovedGCHappenedTest()
    {
      startJsFile("onescene_with_parent.js");
      process();
      populateObjects();

      pxObject* sceneContainer = mSceneContainer[0];
      script.collectGarbage();
      EXPECT_TRUE (sceneContainer->mRefCount > 1);
      EXPECT_TRUE (sceneContainer->parent() != NULL);
    }

private:

    void startJsFile(const char *jsfile)
    {
      mUrl = jsfile;
      mView = new pxScriptView(mUrl,"");
    }

    void process()
    {
      double  secs = pxSeconds();
      while ((pxSeconds() - secs) < 1.0)
      {
        if (NULL != mView)
        {
          mView->onUpdate(pxSeconds());
          script.pump();
        }
      }
    }

    void populateObjects()
    {
      rtObjectRef scene = mView->mScene;
      pxScene2d* sceneptr = (pxScene2d*)scene.getPtr();
      mRoot = sceneptr->getRoot();
      int objcount = 0;
      for(vector<rtRefT<pxObject> >::iterator it = mRoot->mChildren.begin(); it != mRoot->mChildren.end(); ++it)
      {
        if (strcmp((*it)->getMap()->className,"pxSceneContainer") == 0)
        {
          mSceneContainer[objcount] = (*it).getPtr();
        }
      }
    }

    pxObject* mRoot;
    pxObject* mSceneContainer[100];
    pxScriptView* mView;
    rtString mUrl;
};

/*TEST_F(pxSceneContainerLeakTest, sceneContainerTest)
{
  withParentRemovedGCNotHappenedTest();
  withParentRemovedGCHappenedTest();
  withoutParentRemovedGCNotHappenedTest();
  withoutParentRemovedGCHappenedTest();
}*/
