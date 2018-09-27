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

// rtPromise.h

#ifndef _RT_PROMISE_H
#define _RT_PROMISE_H

// TODO Eliminate std::string
#include <string>

#include "rtObject.h"

enum rtPromiseState {PENDING,FULFILLED,REJECTED};

struct thenData
{
  rtFunctionRef mResolve;
  rtFunctionRef mReject;
  rtObjectRef   mNextPromise;

  thenData(rtFunctionRef resolve,
           rtFunctionRef reject,
           rtObjectRef   next) :
      mResolve(resolve), mReject(reject), mNextPromise(next) {};

};

class rtPromise: public rtObject
{
  //static uint32_t promiseID;

public:
  rtDeclareObject(rtPromise,rtObject);
  rtMethod2ArgAndReturn("then",then,rtFunctionRef,rtFunctionRef,rtObjectRef);
  rtMethod2ArgAndNoReturn("then2",then2,rtFunctionRef,rtFunctionRef);
  rtMethod1ArgAndNoReturn("resolve",resolve,rtValue);
  rtMethod1ArgAndNoReturn("reject",reject,rtValue);
  rtProperty(promiseId, getPromiseId, setPromiseId, rtString);

  rtPromise() :  /*promise_id(promiseID++),*/ promise_name(""), mState(PENDING), mObject(NULL)
  {
//    rtLogDebug("############# PROMISE >> CREATED   [ id: %d ]  \n", promise_id);
  }

#if 0
  rtPromise(uint32_t id) : /*promise_id(id),*/ promise_name("") , mState(PENDING), mObject(NULL)
  {
//    rtLogDebug("############# PROMISE >> CREATED   [ id: %d ]  \n", promise_id);
  }

  rtPromise(std::string name) : /*promise_id(promiseID++),*/ promise_name(name), mState(PENDING), mObject(NULL)
  {
//    rtLogDebug("############# PROMISE >> CREATED   [ id: %d  >> %s ]", promise_id, promise_name.c_str());
  }
  #endif

  rtError then(rtFunctionRef resolve, rtFunctionRef reject, 
               rtObjectRef& newPromise)
  {
    if (mState == PENDING)
    {
      thenData data(resolve, reject, new rtPromise);

      mThenData.push_back(data);

      newPromise = data.mNextPromise;

      //rtLogDebug("############# PROMISE >> THEN >> PENDING       [ id: %d  >> %s ]",
      //  promise_id, promise_name.c_str());
    }
    else if (mState == FULFILLED)
    {
      //rtLogDebug("############# PROMISE >> THEN >> FULFILLED     [ id: %d  >> %s ]",
      //  promise_id, promise_name.c_str());

      if (resolve)
      {
        resolve.send(mObject);
        newPromise = new rtPromise;
        newPromise.send("resolve",mObject);
      }
    }
    else if (mState == REJECTED)
    {
      if (reject)
      {
        reject.send(mObject);
        newPromise = new rtPromise;
        newPromise.send("reject",mObject);
      }
    }
    else
    {
      rtLogError("Invalid rtPromise state");
    }

    return RT_OK;
  }

  rtError then2(rtFunctionRef resolve, rtFunctionRef reject)
  {
    rtObjectRef discard;
    return then(resolve, reject, discard);            
  }

  rtError resolve(const rtValue& v)
  {
    if (mState != PENDING)
    {
      //rtLogDebug("############# ERROR:  resolve() >> [%s] != PENDING   [ id: %d  >> %s ]",
      //  rtPromiseState2String(mState), promise_id,promise_name.c_str());

      return RT_FAIL;
    }

    mState = FULFILLED;
    mObject = v.toObject().getPtr();

    for (std::vector<thenData>::iterator it = mThenData.begin();
         it != mThenData.end(); ++it)
    {
      if (it->mResolve)
      {
        it->mResolve.send(mObject);
        it->mNextPromise.send("resolve",mObject);
      }
    }
    mThenData.clear();
    return RT_OK;
  }

  rtError reject(const rtValue& v)
  {
    if (mState != PENDING)
    {
      //rtLogDebug("############# ERROR:  reject() >>[%s] != PENDING   [ id: %d  >> %s ]",
      //  rtPromiseState2String(mState), promise_id,promise_name.c_str());

      return RT_FAIL;
    }

    mState = REJECTED;
    rtObjectRef objRef = v.toObject();

    if (objRef.getPtr() != NULL)
    {
      mObject = objRef.getPtr();
    }
    if (mObject != NULL)
    {
      for (std::vector<thenData>::iterator it = mThenData.begin();
         it != mThenData.end(); ++it)
      {
        if (it->mReject)
        {
          it->mReject.send(mObject);
          it->mNextPromise.send("reject",mObject);
        }
      }
    }
    mThenData.clear();
    mObject = objRef.getPtr();
    return RT_OK;
  }

  bool status()
  {
    if( mState == PENDING)
      return false;
    else 
      return true;
  }
  
  const char* rtPromiseState2String(rtPromiseState s)
  {
    switch(s)
    {
      case PENDING:   return "PENDING";
      case FULFILLED: return "FULFILLED";
      case REJECTED:  return "REJECTED";
      default:        return "(unknown rtPromiseState)";
    }
  }

  rtError getPromiseId(rtString& v) const { v = mPromiseId; return RT_OK; }
  rtError setPromiseId(rtString v) { mPromiseId = v; return RT_OK; }

  rtString mPromiseId;
  //int promise_id;
  std::string promise_name;

private:
  rtPromiseState mState;
  std::vector<thenData> mThenData;
  rtIObject* mObject;
};

// uint32_t rtPromise::promiseID = 0;


#endif
