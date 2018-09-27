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

#ifndef RT_FUNCTION_WRAPPER_H
#define RT_FUNCTION_WRAPPER_H

#include <rtScriptHeaders.h>

#include "rtWrapperUtilsDuk.h"

extern "C" {
#include "duv.h"
}

namespace rtScriptDukUtils
{
class rtFunctionWrapper : public rtWrapper<rtFunctionRef, rtFunctionWrapper>
{
public:
  rtFunctionWrapper(const rtFunctionRef& ref);
  virtual ~rtFunctionWrapper();

public:
  static void createFromFunctionReference(duk_context *ctx, const rtFunctionRef& func);
};

class jsFunctionWrapper : public rtIFunction
{
public:
  jsFunctionWrapper(duk_context *ctx, const std::string &funcName) : mRefCount(0), mDukCtx(ctx), mDukFuncName(funcName), mComplete(false), mTeardownThreadingPrimitives(false), mHash(-1) { }
  virtual ~jsFunctionWrapper();

  virtual unsigned long AddRef();
  virtual unsigned long Release();
  virtual unsigned long getRefCount() const {
    return mRefCount;
  }
  virtual size_t hash() {
    return mHash;
  }

  virtual void setHash(size_t hash)
  {
    mHash = hash;
  }
#if 0
  void signal(rtValue const& returnValue);
#endif

private:
  virtual rtError Send(int numArgs, const rtValue* args, rtValue* result);

#if 0
  rtValue wait();
  void setupSynchronousWait();
#endif

private:
  unsigned long mRefCount;
  std::vector<rtValue> mArgs;

  duk_context *mDukCtx;
  std::string  mDukFuncName;

  bool mComplete;
  bool mTeardownThreadingPrimitives;

#if 0
#ifdef USE_STD_THREADS
  std::mutex mMutex;
  std::condition_variable mCond;
#else
  pthread_mutex_t mMutex;
  pthread_cond_t mCond;
#endif
#endif
  size_t mHash;
  rtValue mReturnValue;
};

} // namespace rtScriptDukUtils

#endif
