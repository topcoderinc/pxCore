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

#include <list>
#include <sstream>

#define private public
#define protected public

#include "pxAnimate.h"
#include "rtString.h"
#include "pxScene2d.h"
#include "pxImage.h"
#include <string.h>
#include <sstream>

#include "test_includes.h" // Needs to be included last

class pxAnimateTest : public testing::Test
{
  public:
    virtual void SetUp()
    {
      mScene = new pxScene2d(false);
      mImage = new pxImage(mScene);
    }

    virtual void TearDown()
    {
    }

    void pxAnimateMembersSetTest ()
    {
         mAnimate = new pxAnimate(rtObjectRef(), 0, pxConstantsAnimation::OPTION_OSCILLATE, 10, -1,  rtObjectRef(), (pxImage*)mImage.getPtr());
         validateReadOnlyMembers(rtObjectRef(), 0, pxConstantsAnimation::OPTION_OSCILLATE, 10, -1);
         EXPECT_TRUE (mAnimate->mCancelled == false);
         EXPECT_TRUE (mAnimate->mStatus == pxConstantsAnimation::STATUS_IDLE);
    }

    void pxAnimateCancelTest ()
    {
         mAnimate = new pxAnimate(rtObjectRef(), 0, pxConstantsAnimation::OPTION_OSCILLATE, 10, -1,  rtObjectRef(), (pxImage*)mImage.getPtr());
         mAnimate->cancel();
         validateReadOnlyMembers(rtObjectRef(), 0, pxConstantsAnimation::OPTION_OSCILLATE, 10, -1);
         EXPECT_TRUE (mAnimate->mCancelled == true);
         EXPECT_TRUE (mAnimate->mStatus == pxConstantsAnimation::STATUS_CANCELLED);
    }
   
    void pxAnimatePropsUpdateTest ()
    {
         rtObjectRef props = new rtMapObject;
         props.set("x",10.0);
         props.set("y",5.0);
         mAnimate = new pxAnimate(props, 0, pxConstantsAnimation::OPTION_OSCILLATE, 10, -1,  rtObjectRef(), (pxImage*)mImage.getPtr());
         struct animation a;
         a.cancelled = false;
         a.prop = "x";
         a.from = 10.0;
         a.to = 2.0;
         a.actualCount = 1;
         a.duration = 1;
         mAnimate->update(a.prop, &a, pxConstantsAnimation::STATUS_INPROGRESS);
         validateReadOnlyMembers(props, 0, pxConstantsAnimation::OPTION_OSCILLATE, 10, -1);
         EXPECT_TRUE (mAnimate->mCancelled == false);
         rtObjectRef propParams = mAnimate->mCurrDetails.get<rtObjectRef>("x");
         pxAnimate::pxAnimationParams* propParamsPtr = (pxAnimate::pxAnimationParams*) propParams.getPtr();
         EXPECT_TRUE(NULL != propParamsPtr);
         EXPECT_TRUE(propParamsPtr->mTo == 2.0);
         EXPECT_TRUE(propParamsPtr->mCount == 1);
         EXPECT_TRUE(propParamsPtr->mFrom == 10.0);
         EXPECT_TRUE(propParamsPtr->mDuration == 1);
         EXPECT_TRUE(propParamsPtr->mStatus == pxConstantsAnimation::STATUS_INPROGRESS);
    }

    void pxAnimateSetStatusTest ()
    {
         mAnimate = new pxAnimate(rtObjectRef(), 0, pxConstantsAnimation::OPTION_OSCILLATE, 10, -1,  rtObjectRef(), (pxImage*)mImage.getPtr());
         mAnimate->setStatus(pxConstantsAnimation::STATUS_INPROGRESS);
         validateReadOnlyMembers(rtObjectRef(), 0, pxConstantsAnimation::OPTION_OSCILLATE, 10, -1);
         EXPECT_TRUE (mAnimate->mStatus == pxConstantsAnimation::STATUS_INPROGRESS);
    }

    private:

      void validateReadOnlyMembers(rtObjectRef props, uint32_t interp, pxConstantsAnimation::animationOptions type, double duration, int32_t count)
      {
         EXPECT_TRUE (mAnimate->mInterp == interp);
         EXPECT_TRUE (mAnimate->mType == type);
         EXPECT_TRUE (mAnimate->mProvisionedDuration == duration);
         EXPECT_TRUE (mAnimate->mProvisionedCount == count);
         EXPECT_TRUE (mAnimate->mProps.getPtr() == props.getPtr());
      }

      pxScene2dRef mScene;
      rtObjectRef mImage;
      pxAnimateRef mAnimate;
}  ;

TEST_F(pxAnimateTest, pxAnimateCompleteTest)
{
    pxAnimateMembersSetTest();
    pxAnimateCancelTest();
    pxAnimatePropsUpdateTest();
    pxAnimateSetStatusTest();
}

