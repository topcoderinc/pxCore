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

// rtNode.cpp

#if defined WIN32
#include <Windows.h>
#include <direct.h>
#define __PRETTY_FUNCTION__ __FUNCTION__
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <errno.h>

#include <string>
#include <fstream>

#include <iostream>
#include <sstream>

#ifndef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "node.h"
#include "node_javascript.h"
#include "node_contextify_mods.h"

#include "env.h"
#include "env-inl.h"

#include "jsbindings/rtWrapperUtils.h"

#ifndef WIN32
#pragma GCC diagnostic pop
#endif

#ifndef RUNINMAIN
extern uv_loop_t *nodeLoop;
#endif
//#include "rtThreadQueue.h"

//extern rtThreadQueue gUIThreadQueue;

#ifdef RUNINMAIN

//#include "pxEventLoop.h"
//extern pxEventLoop* gLoop;

#define ENTERSCENELOCK()
#define EXITSCENELOCK()
#else
#define ENTERSCENELOCK() rtWrapperSceneUpdateEnter();
#define EXITSCENELOCK()  rtWrapperSceneUpdateExit();
#endif

using namespace v8;
using namespace node;

#ifdef ENABLE_DEBUG_MODE
int g_argc = 0;
char** g_argv;
#endif
#ifndef ENABLE_DEBUG_MODE
extern args_t *s_gArgs;
#endif
namespace node
{
extern bool use_debug_agent;
#ifdef HAVE_INSPECTOR
extern bool use_inspector;
#endif
extern bool debug_wait_connect;
}

static int exec_argc;
static const char** exec_argv;
static rtAtomic sNextId = 100;


args_t *s_gArgs;

#ifdef RUNINMAIN
//extern rtNode script;
#endif
rtNodeContexts  mNodeContexts;

#ifdef ENABLE_NODE_V_6_9
ArrayBufferAllocator* array_buffer_allocator = NULL;
bool bufferAllocatorIsSet = false;
#endif
bool nodeTerminated = false;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef RUNINMAIN
#ifdef WIN32
static DWORD __rt_main_thread__;
#else
static pthread_t __rt_main_thread__;
#endif

// rtIsMainThread() - Previously:  identify the MAIN thread of 'node' which running JS code.
//
// rtIsMainThread() - Currently:   identify BACKGROUND thread which running JS code.
//
bool rtIsMainThread()
{
  // Since this is single threaded version we're always on the js thread
  return true;
}
#endif
#if 0
static inline bool file_exists(const char *file)
{
  struct stat buffer;
  return (stat (file, &buffer) == 0);
}
#endif

rtNodeContext::rtNodeContext(Isolate *isolate,Platform* platform) :
     js_file(NULL), mIsolate(isolate), mEnv(NULL), mRefCount(0),mPlatform(platform), mContextifyContext(NULL)
{
  assert(isolate); // MUST HAVE !
  mId = rtAtomicInc(&sNextId);

  createEnvironment();
}

#ifdef USE_CONTEXTIFY_CLONES
rtNodeContext::rtNodeContext(Isolate *isolate, rtNodeContextRef clone_me) :
      js_file(NULL), mIsolate(isolate), mEnv(NULL), mRefCount(0), mPlatform(NULL), mContextifyContext(NULL)
{
  assert(mIsolate); // MUST HAVE !
  mId = rtAtomicInc(&sNextId);

  clonedEnvironment(clone_me);
}
#endif

void rtNodeContext::createEnvironment()
{
  rtLogInfo(__FUNCTION__);
  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);

  // Create a new context.
  Local<Context> local_context = Context::New(mIsolate);

#ifdef ENABLE_NODE_V_6_9

  local_context->SetEmbedderData(HandleMap::kContextIdIndex, Integer::New(mIsolate, mId));

  mContextId = GetContextId(local_context);

  mContext.Reset(mIsolate, local_context); // local to persistent

  Context::Scope context_scope(local_context);

  Handle<Object> global = local_context->Global();



  mRtWrappers.Reset(mIsolate, global);

  // Create Environment.

  mEnv = CreateEnvironment(mIsolate,
                           uv_default_loop(),
                           local_context,
#ifdef ENABLE_DEBUG_MODE
                           g_argc,
                           g_argv,
#else
                           s_gArgs->argc,
                           s_gArgs->argv,
#endif
                           exec_argc,
                           exec_argv);

   array_buffer_allocator->set_env(mEnv);

  mIsolate->SetAbortOnUncaughtExceptionCallback(
        ShouldAbortOnUncaughtException);
#ifdef ENABLE_DEBUG_MODE
  // Start debug agent when argv has --debug
  if (use_debug_agent)
  {
    rtLogWarn("use_debug_agent\n");
#ifdef HAVE_INSPECTOR
    if (use_inspector)
    {
      char currentPath[100];
      memset(currentPath,0,sizeof(currentPath));
      getcwd(currentPath,sizeof(currentPath));
      StartDebug(mEnv, currentPath, debug_wait_connect, mPlatform);
    }
    else
#endif
    {
      StartDebug(mEnv, NULL, debug_wait_connect);
    }
  }
#endif
  // Load Environment.
  {
    Environment::AsyncCallbackScope callback_scope(mEnv);
    LoadEnvironment(mEnv);
  }
#ifdef ENABLE_DEBUG_MODE
  if (use_debug_agent)
  {
    rtLogWarn("use_debug_agent\n");
    EnableDebug(mEnv);
  }
#endif
    rtObjectWrapper::exportPrototype(mIsolate, global);
    rtFunctionWrapper::exportPrototype(mIsolate, global);

    {
      SealHandleScope seal(mIsolate);
#ifndef RUNINMAIN
      EmitBeforeExit(mEnv);
#else
      bool more;
#ifdef ENABLE_NODE_V_6_9
      v8::platform::PumpMessageLoop(mPlatform, mIsolate);
#endif //ENABLE_NODE_V_6_9
      more = uv_run(mEnv->event_loop(), UV_RUN_ONCE);
      if (more == false)
      {
        EmitBeforeExit(mEnv);
      }
#endif

    }
#else
  local_context->SetEmbedderData(HandleMap::kContextIdIndex, Integer::New(mIsolate, mId));

  mContextId = GetContextId(local_context);

  mContext.Reset(mIsolate, local_context); // local to persistent

  Context::Scope context_scope(local_context);

  Handle<Object> global = local_context->Global();

  // Register wrappers.
  rtObjectWrapper::exportPrototype(mIsolate, global);
  rtFunctionWrapper::exportPrototype(mIsolate, global);

  mRtWrappers.Reset(mIsolate, global);

  // Create Environment.
  mEnv = CreateEnvironment(mIsolate,
                           uv_default_loop(),
                           local_context,
#ifdef ENABLE_DEBUG_MODE
                           g_argc,
                           g_argv,
#else
                           s_gArgs->argc,
                           s_gArgs->argv,
#endif
                           exec_argc,
                           exec_argv);

  // Start debug agent when argv has --debug
  if (use_debug_agent)
  {
    rtLogWarn("use_debug_agent\n");
    StartDebug(mEnv, debug_wait_connect);
  }

  // Load Environment.
  LoadEnvironment(mEnv);

  // Enable debugger
  if (use_debug_agent)
  {
    EnableDebug(mEnv);
  }
#endif //ENABLE_NODE_V_6_9
}

#ifdef USE_CONTEXTIFY_CLONES

void rtNodeContext::clonedEnvironment(rtNodeContextRef clone_me)
{
  rtLogInfo(__FUNCTION__);
  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);

  // Get parent Local context...
  Local<Context> local_context = clone_me->getLocalContext();
  Context::Scope context_scope(local_context);

  // Create dummy sandbox for ContextifyContext::makeContext() ...
  Local<Object> sandbox = Object::New(mIsolate);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if( clone_me->has(SANDBOX_IDENTIFIER) )
  {
    rtValue       val_array = clone_me->get(SANDBOX_IDENTIFIER);
    rtObjectRef       array = val_array.toObject();

    int len = array.get<int>("length");

    rtString s;
    for(int i = 0; i < len; i++)
    {
      array.get<rtString>( (uint32_t) i, s);  // get 'name' for object
      rtValue obj = clone_me->get(s);         // get object for 'name'

      if( obj.isEmpty() == false)
      {
          // Copy to var/module 'sandbox' under construction...
          Local<Value> module = local_context->Global()->Get( String::NewFromUtf8(mIsolate, s.cString() ) );
          sandbox->Set( String::NewFromUtf8(mIsolate, s.cString()), module);
      }
      else
      {
        rtLogError("## FATAL:   '%s' is empty !! - UNEXPECTED", s.cString());
      }
    }
  }
  else
  {
    rtLogWarn("## WARNING:   '%s' is undefined !! - UNEXPECTED", SANDBOX_IDENTIFIER);
  }
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // Clone a new context.
  {
    Local<Context>  clone_local = node::makeContext(mIsolate, sandbox); // contextify context with 'sandbox'

    clone_local->SetEmbedderData(HandleMap::kContextIdIndex, Integer::New(mIsolate, mId));
#ifdef ENABLE_NODE_V_6_9
    Local<Context> envCtx = Environment::GetCurrent(mIsolate)->context();
    Local<String> symbol_name = FIXED_ONE_BYTE_STRING(mIsolate, "_contextifyPrivate");
    Local<Private> private_symbol_name = Private::ForApi(mIsolate, symbol_name);
    MaybeLocal<Value> maybe_value = sandbox->GetPrivate(envCtx,private_symbol_name);
    Local<Value> decorated;
    if (true == maybe_value.ToLocal(&decorated))
    {
      mContextifyContext = decorated.As<External>()->Value();
    }
#else
    Local<String> hidden_name = FIXED_ONE_BYTE_STRING(mIsolate, "_contextifyHidden");
    mContextifyContext = sandbox->GetHiddenValue(hidden_name).As<External>()->Value();
#endif

    mContextId = GetContextId(clone_local);

    mContext.Reset(mIsolate, clone_local); // local to persistent

    Context::Scope context_scope(clone_local);

    Handle<Object> clone_global = clone_local->Global();

    // Register wrappers in this cloned context...
      rtObjectWrapper::exportPrototype(mIsolate, clone_global);
    rtFunctionWrapper::exportPrototype(mIsolate, clone_global);

    mRtWrappers.Reset(mIsolate, clone_global);
}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}

#endif // USE_CONTEXTIFY_CLONES

rtNodeContext::~rtNodeContext()
{
  rtLogInfo(__FUNCTION__);
  //Make sure node is not destroyed abnormally
  if (true == node_is_initialized)
  {
    runScript("var process = require('process');process._tickCallback();");
    if(mEnv)
    {
      Locker                locker(mIsolate);
      Isolate::Scope isolate_scope(mIsolate);
      HandleScope     handle_scope(mIsolate);

      RunAtExit(mEnv);
    #ifdef ENABLE_NODE_V_6_9
      if (nodeTerminated)
      {
        array_buffer_allocator->set_env(NULL);
      }
      else
      {
        mEnv->Dispose();
      }
    #else
      mEnv->Dispose();
    #endif // ENABLE_NODE_V_6_9
      mEnv = NULL;
      #ifndef USE_CONTEXTIFY_CLONES
      HandleMap::clearAllForContext(mId);
      #endif
    }
    else
    {
    // clear out persistent javascript handles
      HandleMap::clearAllForContext(mId);
#ifdef ENABLE_NODE_V_6_9
      node::deleteContextifyContext(mContextifyContext);
#endif
      mContextifyContext = NULL;
    }
    if(exec_argv)
    {
      delete[] exec_argv;
      exec_argv = NULL;
      exec_argc = 0;
    }

    // TODO:  Might not be needed in ST case...
    //
    // Un-Register wrappers.
    // rtObjectWrapper::destroyPrototype();
    // rtFunctionWrapper::destroyPrototype();
    mContext.Reset();
    mRtWrappers.Reset();

    Release();
  }
  // NOTE: 'mIsolate' is owned by rtNode.  Don't destroy here !
}

rtError rtNodeContext::add(const char *name, rtValue const& val)
{
  if(name == NULL)
  {
    rtLogDebug(" rtNodeContext::add() - no symbolic name for rtValue");
    return RT_FAIL;
  }
  else if(this->has(name))
  {
    rtLogDebug(" rtNodeContext::add() - ALREADY HAS '%s' ... over-writing.", name);
   // return; // Allow for "Null"-ing erasure.
  }
  
  if(val.isEmpty())
  {
    rtLogDebug(" rtNodeContext::add() - rtValue is empty");
    return RT_FAIL;
  }

  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

  // Get a Local context...
  Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
  Context::Scope context_scope(local_context);

  local_context->Global()->Set( String::NewFromUtf8(mIsolate, name), rt2js(local_context, val));
  
  return RT_OK;
}

rtValue rtNodeContext::get(std::string name)
{
  return get( name.c_str() );
}

rtValue rtNodeContext::get(const char *name)
{
  if(name == NULL)
  {
    rtLogError(" rtNodeContext::get() - no symbolic name for rtValue");
    return rtValue();
  }

  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

  // Get a Local context...
  Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
  Context::Scope context_scope(local_context);

  Handle<Object> global = local_context->Global();

  // Get the object
  Local<Value> object = global->Get( String::NewFromUtf8(mIsolate, name) );

  if(object->IsUndefined() || object->IsNull() )
  {
    rtLogError("FATAL: '%s' is Undefined ", name);
    return rtValue();
  }
  else
  {
    rtWrapperError error; // TODO - handle error
    return js2rt(local_context, object, &error);
  }
}

bool rtNodeContext::has(std::string name)
{
  return has( name.c_str() );
}

bool rtNodeContext::has(const char *name)
{
  if(name == NULL)
  {
    rtLogError(" rtNodeContext::has() - no symbolic name for rtValue");
    return false;
  }

  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

  // Get a Local context...
  Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
  Context::Scope context_scope(local_context);

  Handle<Object> global = local_context->Global();

#ifdef ENABLE_NODE_V_6_9
  TryCatch try_catch(mIsolate);
#else
  TryCatch try_catch;
#endif // ENABLE_NODE_V_6_9

  Handle<Value> value = global->Get(String::NewFromUtf8(mIsolate, name) );

  if (try_catch.HasCaught())
  {
     rtLogError("\n ## has() - HasCaught()  ... ERROR");
     return false;
  }

  // No need to check if |value| is empty because it's taken care of
  // by TryCatch above.

  return ( !value->IsUndefined() && !value->IsNull() );
}

// DEPRECATED - 'has()' is replacement for 'find()'
//
// bool rtNodeContext::find(const char *name)
// {
//   rtNodeContexts_iterator it = mNodeContexts.begin();
//
//   while(it != mNodeContexts.end())
//   {
//     rtNodeContextRef ctx = it->second;
//
//     rtLogWarn("\n ######## CONTEXT !!! ID: %d  %s  '%s'",
//       ctx->getContextId(),
//       (ctx->has(name) ? "*HAS*" : "does NOT have"),
//       name);
//
//     it++;
//   }
//
//   rtLogWarn("\n ");
//
//   return false;
// }

rtError rtNodeContext::runScript(const char* script, rtValue* retVal /*= NULL*/, const char *args /*= NULL*/)
{
  if(script == NULL)
  {
    rtLogError(" %s  ... no script given.",__PRETTY_FUNCTION__);

    return RT_FAIL;
  }

  // rtLogDebug(" %s  ... Running...",__PRETTY_FUNCTION__);

  return runScript(std::string(script), retVal, args);
}

rtError rtNodeContext::runScript(const std::string &script, rtValue* retVal /*= NULL*/, const char* /* args = NULL*/)
{
  rtLogInfo(__FUNCTION__);
  if(script.empty())
  {
    rtLogError(" %s  ... no script given.",__PRETTY_FUNCTION__);

    return RT_FAIL;
  }

  {//scope
    Locker                locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

    // Get a Local context...
    Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
    Context::Scope context_scope(local_context);
// !CLF TODO: TEST FOR MT
#ifdef RUNINMAIN
#ifdef ENABLE_NODE_V_6_9
  TryCatch tryCatch(mIsolate);
#else
  TryCatch tryCatch;
#endif // ENABLE_NODE_V_6_9
#endif
    Local<String> source = String::NewFromUtf8(mIsolate, script.c_str());

    // Compile the source code.
    Local<Script> run_script = Script::Compile(source);

    // Run the script to get the result.
    Local<Value> result = run_script->Run();
// !CLF TODO: TEST FOR MT
#ifdef RUNINMAIN
   if (tryCatch.HasCaught())
    {
      String::Utf8Value trace(tryCatch.StackTrace());
      rtLogWarn("%s", *trace);

      return RT_FAIL;
    }
#endif

    if(retVal)
    {
      // Return val
      rtWrapperError error;
      *retVal = js2rt(local_context, result, &error);
      
      if(error.hasError())
      {
        rtLogError("js2rt() - return from script error");
        return RT_FAIL;
      }
    }

   return RT_OK;

  }//scope

  return RT_FAIL;
}

std::string readFile(const char *file)
{
  std::ifstream       src_file(file);
  std::stringstream   src_script;

  src_script << src_file.rdbuf(); // slurp up file

  std::string s = src_script.str();

  return s;
}

rtError rtNodeContext::runFile(const char *file, rtValue* retVal /*= NULL*/, const char* args /*= NULL*/)
{
  if(file == NULL)
  {
    rtLogError(" %s  ... no script given.",__PRETTY_FUNCTION__);

    return RT_FAIL;
  }

  // Read the script file
  js_file   = file;
  js_script = readFile(file);
  
  if( js_script.empty() ) // load error
  {
    rtLogError(" %s  ... load error / not found.",__PRETTY_FUNCTION__);
     
    return RT_FAIL;
  }

  return runScript(js_script, retVal, args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

rtNode::rtNode()
#ifndef RUNINMAIN
#ifdef USE_CONTEXTIFY_CLONES
: mRefContext(), mNeedsToEnd(false)
#else
: mNeedsToEnd(false)
#endif
#endif
{
  rtLogInfo(__FUNCTION__);
  mTestGc = false;
  mIsolate = NULL;
  mPlatform = NULL;
  initializeNode();
}

rtNode::rtNode(bool initialize)
#ifndef RUNINMAIN
#ifdef USE_CONTEXTIFY_CLONES
: mRefContext(), mNeedsToEnd(false)
#else
: mNeedsToEnd(false)
#endif
#endif
{
  rtLogInfo(__FUNCTION__);
  mTestGc = false;
  mIsolate = NULL;
  mPlatform = NULL;
  if (true == initialize)
  {
    initializeNode();
  }
}

void rtNode::initializeNode()
{
  rtLogInfo(__FUNCTION__);
  char const* s = getenv("RT_TEST_GC");
  if (s && strlen(s) > 0)
    mTestGc = true;
  else
    mTestGc = false;

  if (mTestGc)
    rtLogWarn("*** PERFORMANCE WARNING *** : gc being invoked in render thread");

// TODO Please make this better... less hard coded...

                              //0123456 789ABCDEF012 345 67890ABCDEF
#if ENABLE_V8_HEAP_PARAMS
#ifdef ENABLE_NODE_V_6_9
  static const char *args2   = "rtNode\0-e\0console.log(\"rtNode Initalized\");\0\0";
  static const char *argv2[] = {&args2[0], &args2[7], &args2[10], NULL};
#else
  rtLogWarn("v8 old heap space configured to 64mb\n");
  static const char *args2   = "rtNode\0--expose-gc\0--max_old_space_size=64\0-e\0console.log(\"rtNode Initalized\");\0\0";
  static const char *argv2[] = {&args2[0], &args2[7], &args2[19], &args2[43], &args2[46], NULL};
#endif // ENABLE_NODE_V_6_9
#else
#ifdef ENABLE_NODE_V_6_9
#ifndef ENABLE_DEBUG_MODE
   static const char *args2   = "rtNode\0-e\0console.log(\"rtNode Initalized\");\0\0";
   static const char *argv2[] = {&args2[0], &args2[7], &args2[10], NULL};
#endif //!ENABLE_DEBUG_MODE
#else
  static const char *args2   = "rtNode\0--expose-gc\0-e\0console.log(\"rtNode Initalized\");\0\0";
  static const char *argv2[] = {&args2[0], &args2[7], &args2[19], &args2[22], NULL};
#endif // ENABLE_NODE_V_6_9
#endif //ENABLE_V8_HEAP_PARAMS
#ifndef ENABLE_DEBUG_MODE
  int          argc   = sizeof(argv2)/sizeof(char*) - 1;

  static args_t aa(argc, (char**)argv2);

  s_gArgs = &aa;


  char **argv = aa.argv;
#endif

#ifdef RUNINMAIN
#ifdef WIN32
  __rt_main_thread__ = GetCurrentThreadId();
#else
  __rt_main_thread__ = pthread_self(); //  NB
#endif
#endif
  nodePath();


#ifdef ENABLE_NODE_V_6_9
  rtLogWarn("rtNode::rtNode() calling init \n");
#ifdef ENABLE_DEBUG_MODE
  init();
#else
  init(argc, argv);
#endif
#else
  mIsolate     = Isolate::New();
  node_isolate = mIsolate; // Must come first !!

#ifdef ENABLE_DEBUG_MODE
  init();
#else
  init(argc, argv);
#endif
#endif // ENABLE_NODE_V_6_9
}

rtNode::~rtNode()
{
  // rtLogInfo(__FUNCTION__);
  term();
}

void rtNode::pump()
{
//#ifndef RUNINMAIN
//  return;
//#else
  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.
#ifdef ENABLE_NODE_V_6_9
  v8::platform::PumpMessageLoop(mPlatform, mIsolate);
#endif //ENABLE_NODE_V_6_9
  uv_run(uv_default_loop(), UV_RUN_NOWAIT);//UV_RUN_ONCE);

  // Enable this to expedite garbage collection for testing... warning perf hit
  if (mTestGc)
  {
    static int sGcTickCount = 0;

    if (sGcTickCount++ > 60)
    {
      Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
      Context::Scope contextScope(local_context);
      mIsolate->RequestGarbageCollectionForTesting(Isolate::kFullGarbageCollection);
      sGcTickCount = 0;
    }
  }
//#endif // RUNINMAIN
}

std::string rtNode::name() const
{
  return "node";
}

void rtNode::garbageCollect()
{
//#ifndef RUNINMAIN
//  return;
//#else
  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

  Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
  Context::Scope contextScope(local_context);
  mIsolate->LowMemoryNotification();
//#endif // RUNINMAIN
}

#if 0
rtNode::forceGC()
{
  mIsolate->RequestGarbageCollectionForTesting(Isolate::kFullGarbageCollection);
}
#endif

void rtNode::nodePath()
{
  const char* NODE_PATH = ::getenv("NODE_PATH");

  if(NODE_PATH == NULL)
  {
    char cwd[1024] = {};

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
#ifdef WIN32
	  _putenv_s("NODE_PATH", cwd);
#else
	  ::setenv("NODE_PATH", cwd, 1); // last arg is 'overwrite' ... 0 means DON'T !
#endif
	  rtLogInfo("NODE_PATH=%s", cwd);
    }
    else
    {
      rtLogError(" - failed to set NODE_PATH");
    }
  }
}
#ifndef RUNINMAIN
bool rtNode::isInitialized()
{
  //rtLogDebug("rtNode::isInitialized returning %d\n",node_is_initialized);
  return node_is_initialized;
}
#endif
#ifdef ENABLE_DEBUG_MODE
void rtNode::init()
#else
void rtNode::init(int argc, char** argv)
#endif
{
  // Hack around with the argv pointer. Used for process.title = "blah".
#ifdef ENABLE_DEBUG_MODE
  g_argv = uv_setup_args(g_argc, g_argv);
#else
  argv = uv_setup_args(argc, argv);
#endif

  rtLogInfo(__FUNCTION__);

#if 0
#warning Using DEBUG AGENT...
  use_debug_agent = true; // JUNK
#endif

  if(node_is_initialized == false)
  {
    rtLogWarn("About to Init\n");
#ifdef ENABLE_DEBUG_MODE
    Init(&g_argc, const_cast<const char**>(g_argv), &exec_argc, &exec_argv);
#else
    Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);
#endif

//    mPlatform = platform::CreateDefaultPlatform();
//    V8::InitializePlatform(mPlatform);

#ifdef ENABLE_NODE_V_6_9
   rtLogWarn("using node version %s\n", NODE_VERSION);
   V8::InitializeICU();
#ifdef ENABLE_DEBUG_MODE
   V8::InitializeExternalStartupData(g_argv[0]);
#else
   V8::InitializeExternalStartupData(argv[0]);
#endif
   Platform* platform = platform::CreateDefaultPlatform();
   mPlatform = platform;
   V8::InitializePlatform(platform);
   V8::Initialize();
   Isolate::CreateParams params;
   array_buffer_allocator = new ArrayBufferAllocator();
   const char* source1 = "function pxSceneFooFunction(){ return 0;}";
   static v8::StartupData data = v8::V8::CreateSnapshotDataBlob(source1);
   params.snapshot_blob = &data;
   params.array_buffer_allocator = array_buffer_allocator;
   mIsolate     = Isolate::New(params);
   node_isolate = mIsolate; // Must come first !!
#else
    V8::Initialize();
#endif // ENABLE_NODE_V_6_9
    rtLogWarn("rtNode::init() node_is_initialized=%d\n",node_is_initialized);
    node_is_initialized = true;

    Locker                locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

    Local<Context> ctx = Context::New(mIsolate);
    ctx->SetEmbedderData(HandleMap::kContextIdIndex, Integer::New(mIsolate, 99));
    mContext.Reset(mIsolate, ctx);
    rtLogWarn("DONE in rtNode::init()\n");
  }
}

void rtNode::term()
{
  rtLogInfo(__FUNCTION__);
  nodeTerminated = true;
#ifdef USE_CONTEXTIFY_CLONES
  if( mRefContext.getPtr() )
  {
    mRefContext->Release();
  }
#endif

  if(node_isolate)
  {
// JRJRJR  Causing crash???  ask Hugh

    rtLogWarn("\n++++++++++++++++++ DISPOSE\n\n");
    node_isolate->Dispose();
    node_isolate = NULL;
    mIsolate     = NULL;
  }

  if(node_is_initialized)
  {
    //V8::Dispose();

    node_is_initialized = false;

    V8::ShutdownPlatform();
    if(mPlatform)
    {
      delete mPlatform;
      mPlatform = NULL;
    }

  //  if(mPxNodeExtension)
  //  {
  //    delete mPxNodeExtension;
  //    mPxNodeExtension = NULL;
  //  }
  }
}


inline bool fileExists(const std::string& name)
{
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

rtNodeContextRef rtNode::getGlobalContext() const
{
  return rtNodeContextRef();
}

rtNodeContextRef rtNode::createContext(bool ownThread)
{
  UNUSED_PARAM(ownThread);    // not implemented yet.

  rtNodeContextRef ctxref;

#ifdef USE_CONTEXTIFY_CLONES
  if(mRefContext.getPtr() == NULL)
  {
    mRefContext = new rtNodeContext(mIsolate,mPlatform);
    ctxref = mRefContext;

    static std::string sandbox_path;

    if(sandbox_path.empty()) // only once.
    {
      char *nodePath = ::getenv("NODE_PATH");
      if (NULL != nodePath)
      {
        const std::string NODE_PATH = nodePath;
        sandbox_path = NODE_PATH + "/" + SANDBOX_JS;
      }
    }

    // Populate 'sandbox' vars in JS...
    if(fileExists(sandbox_path.c_str()))
    {
      mRefContext->runFile(sandbox_path.c_str());
    }
    else
    {
      rtLogError("## ERROR:   Could not find \"%s\" ...", sandbox_path.c_str());
    }
    // !CLF: TODO Why is ctxref being reassigned from the mRefContext already assigned?
    //ctxref = new rtNodeContext(mIsolate, mRefContext);
  }
  else
  {
    // rtLogInfo("\n createContext()  >>  CLONE CREATED !!!!!!");
    ctxref = new rtNodeContext(mIsolate, mRefContext); // CLONE !!!
  }
#else
    ctxref = new rtNodeContext(mIsolate,mPlatform);

#endif

  // TODO: Handle refs in map ... don't leak !
  // mNodeContexts[ ctxref->getContextId() ] = ctxref;  // ADD to map

  return ctxref;
}

unsigned long rtNodeContext::Release()
{
    long l = rtAtomicDec(&mRefCount);
    if (l == 0)
    {
     delete this;
    }
    return l;
}


