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

// pxText.h

#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
// #include <glm/glm.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// #include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include "pxVideo.h"

using namespace std::placeholders;

pxVideo *pxVideo::pxVideoObj = NULL;
GMainLoop *pxVideo::AAMPGstPlayerMainLoop;

extern pxContext context;
extern rtThreadQueue* gUIThreadQueue;

static void CheckGlError( int line )
{
    int err = glGetError();
    if( err )
    {
        printf( "%d: glGetError: 0x%04x\n", line, err ); // GL_INVALID_OPERATION == 0x0502
//        abort();
    }
}

/**
 * @brief Thread to run mainloop (for standalone mode)
 * @param[in] arg user_data
 * @retval void pointer
 */
void* pxVideo::AAMPGstPlayer_StreamThread(void */*arg */)
{
  if (AAMPGstPlayerMainLoop)
  {
    g_main_loop_run(AAMPGstPlayerMainLoop); // blocks
    printf("AAMPGstPlayer_StreamThread: exited main event loop\n");
  }
  g_main_loop_unref(AAMPGstPlayerMainLoop);
  AAMPGstPlayerMainLoop = NULL;
  return NULL;
}

/**
 * @brief To initialize Gstreamer and start mainloop (for standalone mode)
 * @param[in] argc number of arguments
 * @param[in] argv array of arguments
 */
void pxVideo::InitPlayerLoop()
{
  if (!initialized)
  {
    initialized = true;
    gst_init(NULL, NULL);
    AAMPGstPlayerMainLoop = g_main_loop_new(NULL, FALSE);
    aampMainLoopThread = g_thread_new("AAMPGstPlayerLoop", &pxVideo::AAMPGstPlayer_StreamThread, NULL );
  }
}

void pxVideo::TermPlayerLoop()
{
	if(AAMPGstPlayerMainLoop)
	{
		g_main_loop_quit(AAMPGstPlayerMainLoop);
		g_thread_join(aampMainLoopThread);
		delete gPlayer;
		gst_deinit ();
		printf("%s(): Exited GStreamer MainLoop.\n", __FUNCTION__);
	}
}

GLuint pxVideo::LoadShader( GLenum type )
{
	GLuint shaderHandle = 0;
	const char *sources[1];

	if(GL_VERTEX_SHADER == type)
	{
		sources[0] = VSHADER;
	}
	else
	{
		sources[0] = FSHADER;
	}

	if( sources[0] )
	{
		shaderHandle = glCreateShader(type);
		glShaderSource(shaderHandle, 1, sources, 0);
		glCompileShader(shaderHandle);
		GLint compileSuccess;
		glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
		if (compileSuccess == GL_FALSE)
		{
			GLchar msg[1024];
			glGetShaderInfoLog(shaderHandle, sizeof(msg), 0, &msg[0]);
			printf( "%s\n", msg );
		}
	}

	return shaderHandle;
}

void pxVideo::InitYUVShaders()
{
	sharedContext->makeCurrent(true);

	GLint linked;

	if (gAampFbo.getPtr() == NULL)
	{
		gAampFbo = context.createFramebuffer(1280, 780);
	}
	else
	{
		context.updateFramebuffer(gAampFbo, 1280, 780);
	}
	pxContextFramebufferRef prevFbo = context.getCurrentFramebuffer();
	pxError replacedFbo = PX_NOTINITIALIZED;
	bool existingFbo = false;
	if (prevFbo.getPtr() != gAampFbo.getPtr())
	{
		replacedFbo = context.setFramebuffer(gAampFbo);
	}
	else
	{
		existingFbo = true;
	}
	if (existingFbo || replacedFbo == PX_OK)
	{
		GLenum gl_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		assert( gl_status == GL_FRAMEBUFFER_COMPLETE );
		CheckGlError(__LINE__);
		GLint vShader = LoadShader(GL_VERTEX_SHADER);
		CheckGlError(__LINE__);
		GLint fShader = LoadShader(GL_FRAGMENT_SHADER);
		CheckGlError(__LINE__);
		mProgramID = glCreateProgram();
		glAttachShader(mProgramID,vShader);
		glAttachShader(mProgramID,fShader);
		CheckGlError(__LINE__);

		glBindAttribLocation(mProgramID, ATTRIB_VERTEX, "vertexIn");
		glBindAttribLocation(mProgramID, ATTRIB_TEXTURE, "textureIn");
		CheckGlError(__LINE__);
		glLinkProgram(mProgramID);
		glValidateProgram(mProgramID);

		glGetProgramiv(mProgramID, GL_LINK_STATUS, &linked);
		if( linked == GL_FALSE )
		{
			GLint logLen;
			glGetProgramiv(mProgramID, GL_INFO_LOG_LENGTH, &logLen);
			GLchar *msg = (GLchar *)malloc(sizeof(GLchar)*logLen);
			glGetProgramInfoLog(mProgramID, logLen, &logLen, msg );
			printf( "%s\n", msg );
			free( msg );
		}
		glUseProgram(mProgramID);
		CheckGlError(__LINE__);
		glDeleteShader(vShader);
		glDeleteShader(fShader);
		CheckGlError(__LINE__);
		//Get Uniform Variables Location
		textureUniformY = glGetUniformLocation(mProgramID, "tex_y");
		textureUniformU = glGetUniformLocation(mProgramID, "tex_u");
		textureUniformV = glGetUniformLocation(mProgramID, "tex_v");
		CheckGlError(__LINE__);
		typedef struct _vertex
		{
			float p[2];
			float uv[2];
		} Vertex;

		static const Vertex vertexPtr[4] =
		{
			{{-1,-1}, {0.0,1 } },
			{{ 1,-1}, {1,1 } },
			{{ 1, 1}, {1,0.0 } },
			{{-1, 1}, {0.0,0.0} }
		};
		static const unsigned short index[6] =
		{
			0,1,2, 2,3,0
		};

		glGenVertexArrays(1, &_vertexArray);
		CheckGlError(__LINE__);
		glBindVertexArray(_vertexArray);
		CheckGlError(__LINE__);
		glGenBuffers(2, _vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertexPtr), vertexPtr, GL_STATIC_DRAW );
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vertexBuffer[1]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index), index, GL_STATIC_DRAW );
		glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE,
								sizeof(Vertex), (const GLvoid *)offsetof(Vertex,p) );
		glEnableVertexAttribArray(ATTRIB_VERTEX);

		glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, GL_FALSE,
							  sizeof(Vertex), (const GLvoid *)offsetof(Vertex, uv ) );
		glEnableVertexAttribArray(ATTRIB_TEXTURE);
		glBindVertexArray(0);
		CheckGlError(__LINE__);
		glGenTextures(1, &id_y);
		CheckGlError(__LINE__);
		glBindTexture(GL_TEXTURE_2D, id_y);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		CheckGlError(__LINE__);
		glGenTextures(1, &id_u);
		glBindTexture(GL_TEXTURE_2D, id_u);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		CheckGlError(__LINE__);
		glGenTextures(1, &id_v);
		glBindTexture(GL_TEXTURE_2D, id_v);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		CheckGlError(__LINE__);

		if(replacedFbo == PX_OK)
		{
			context.setFramebuffer(prevFbo);
		}
	}
}

pxVideo::pxVideo(pxScene2d* scene):pxObject(scene), mVideoTexture()
#ifdef ENABLE_SPARK_VIDEO_PUNCHTHROUGH
 ,mEnablePunchThrough(true)
#else
, mEnablePunchThrough(false)
#endif //ENABLE_SPARK_VIDEO_PUNCHTHROUGH
,mAutoPlay(false)
,mUrl("")
,gPlayer(nullptr)
,mTimeTik(0)
,mPreviousFrameTime(-1.0)
,previousFrame(nullptr)
{
	  FBO_W = 1280;		//TODO: How to get scene size?
	  FBO_H = 720;
	  gAampFbo = NULL;
	  aampMainLoopThread = NULL;
	  AAMPGstPlayerMainLoop = NULL;
	  InitPlayerLoop();
#ifdef AAMP_USE_SHADER
//	  mAamp = new PlayerInstanceAAMP(NULL, std::bind(&pxVideo::updateYUVFrame_shader, this, _1, _2, _3, _4));
#else
//	  mAamp = new PlayerInstanceAAMP(NULL, std::bind(&pxVideo::updateYUVFrame, this, _1, _2, _3, _4));
#endif
//	  assert (nullptr != mAamp);
  // clean
  while (!frames.empty()) {
    frames.pop();
  }
  gPlayer = new GStreamPlayer(nullptr, [&](guint8 *buffer, int w, int h, guint stride, guint pixel_stride) {
    gAampFboMutex.lock();
    guint8 *bits = buffer;
    pxOffscreen *offscreen = new pxOffscreen;
    offscreen->init(w, h);  // video frame size may changed when netword changed, so need reset size

    for (int y = 0; y < h; y++) {
      pxPixel *d = offscreen->scanline(y);
      for (int x = 0; x < w; x++) {
        int index = y * w + x;
        *d = pxPixel(bits[index * 4], bits[index * 4 + 1], bits[index * 4 + 2]);
        d++;
      }
    }
    frames.push(offscreen);
    rtLogError("count = %d",frames.size());
    gAampFboMutex.unlock();
    gUIThreadQueue->addTask(newAampFrame, pxVideo::pxVideoObj, NULL);
  });

  gPlayer->setEventCallback([&](const char *event, void *data) {
    mEmit.send(event);
  });
  rtLogWarn("OpenGL Version[%s], GLSL Version[%s]\n", glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
  sharedContext = context.createSharedContext();
  pxVideo::pxVideoObj = this;
#ifdef AAMP_USE_SHADER
  InitYUVShaders();
#endif
}

pxVideo::~pxVideo()
{
//	mAamp->Stop();
//	delete mAamp;
	TermPlayerLoop();
}

void pxVideo::onInit()
{
	rtLogError("%s:%d.",__FUNCTION__,__LINE__);
	if(mAutoPlay)
	{
		play();
	}
  mReady.send("resolve",this);
  pxObject::onInit();
}

void pxVideo::newAampFrame(void* context, void* data)
{
	pxVideo* videoObj = reinterpret_cast<pxVideo*>(context);
	if (videoObj)
	{
		videoObj->onTextureReady();
	}
}

void pxVideo::updateYUVFrame_shader(uint8_t *yuvBuffer, int size, int pixel_w, int pixel_h) //YUV=>RGB conversion in shader
{
	/** Input in I420 (YUV420) format.
	  * Buffer structure:
	  * ----------
	  * |        |
	  * |   Y    | size = w*h
	  * |        |
	  * |________|
	  * |   U    |size = w*h/4
	  * |--------|
	  * |   V    |size = w*h/4
	  * ----------*
	  */
	if(yuvBuffer)
	{
		sharedContext->makeCurrent(true);

		gAampFboMutex.lock();
		if (gAampFbo.getPtr() == NULL)
		{
			gAampFbo = context.createFramebuffer(pixel_w, pixel_h);
		}
		else
		{
			context.updateFramebuffer(gAampFbo, pixel_w, pixel_h);
		}
		FBO_W = pixel_w;
		FBO_H = pixel_h;
		pxContextFramebufferRef prevFbo = context.getCurrentFramebuffer();
		pxError replacedFbo = PX_NOTINITIALIZED;
		bool existingFbo = false;
		if (prevFbo.getPtr() != gAampFbo.getPtr())
		{
			replacedFbo = context.setFramebuffer(gAampFbo);
		}
		else
		{
			existingFbo = true;
		}
		if (existingFbo || replacedFbo == PX_OK)
		{
//			printf("AAMP: Rendering frame.\n");
			unsigned char *yPlane, *uPlane, *vPlane;
			yPlane = yuvBuffer;
			uPlane = yPlane + (pixel_w*pixel_h);
			vPlane = uPlane + (pixel_w*pixel_h)/4;

			glClearColor(0.0,0.0,0.0,0.0);
			glClear(GL_COLOR_BUFFER_BIT);

			//Y
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, id_y);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pixel_w, pixel_h, 0, GL_RED, GL_UNSIGNED_BYTE, yPlane);
			glUniform1i(textureUniformY, 0);

			//U
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, id_u);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pixel_w/2, pixel_h/2, 0, GL_RED, GL_UNSIGNED_BYTE, uPlane);
			glUniform1i(textureUniformU, 1);

			//V
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, id_v);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pixel_w/2, pixel_h/2, 0, GL_RED, GL_UNSIGNED_BYTE, vPlane);
			glUniform1i(textureUniformV, 2);

			glBindVertexArray(_vertexArray);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0 );
			glBindVertexArray(0);

			if(replacedFbo == PX_OK)
			{
				context.setFramebuffer(prevFbo);
			}
		}
		gAampFboMutex.unlock();
		gUIThreadQueue->addTask(newAampFrame, pxVideo::pxVideoObj, NULL);
	}
}

inline unsigned char RGB_ADJUST(double tmp)
{
	return (unsigned char)((tmp >= 0 && tmp <= 255)?tmp:(tmp < 0 ? 0 : 255));
}

void CONVERT_YUV420PtoRGB24(unsigned char* yuv420buf,unsigned char* rgbOutBuf,int nWidth,int nHeight)
{
	unsigned char Y,U,V,R,G,B;
	unsigned char* yPlane,*uPlane,*vPlane;
	int rgb_width , u_width;
	rgb_width = nWidth * 3;
	u_width = (nWidth >> 1);
	int offSet = 0;

	yPlane = yuv420buf;
	uPlane = yuv420buf + nWidth*nHeight;
	vPlane = uPlane + nWidth*nHeight/4;

	for(int i = 0; i < nHeight; i++)
	{
		for(int j = 0; j < nWidth; j ++)
		{
			Y = *(yPlane + nWidth * i + j);
			offSet = (i>>1) * (u_width) + (j>>1);
			V = *(uPlane + offSet);
			U = *(vPlane + offSet);

			//  R,G,B values
			R = RGB_ADJUST((Y + (1.4075 * (V - 128))));
			G = RGB_ADJUST((Y - (0.3455 * (U - 128) - 0.7169 * (V - 128))));
			B = RGB_ADJUST((Y + (1.7790 * (U - 128))));
			offSet = rgb_width * i + j * 3;

			rgbOutBuf[offSet] = B;
			rgbOutBuf[offSet + 1] = G;
			rgbOutBuf[offSet + 2] = R;
		}
	}
}

void pxVideo::updateYUVFrame(uint8_t *yuvBuffer, int size, int pixel_w, int pixel_h) //YUV=>RGB conversion in C++
{
	/** Input in I420 (YUV420) format.
	  * Buffer structure:
	  * ----------
	  * |        |
	  * |   Y    | size = w*h
	  * |        |
	  * |________|
	  * |   U    |size = w*h/4
	  * |--------|
	  * |   V    |size = w*h/4
	  * ----------*
	  */
	if(yuvBuffer)
	{
		sharedContext->makeCurrent(true);

		gAampFboMutex.lock();
		if (gAampFbo.getPtr() == NULL)
		{
			gAampFbo = context.createFramebuffer(1280, 720);
		}
		else
		{
			context.updateFramebuffer(gAampFbo, 1280, 720);
		}
		FBO_W = 1280;
		FBO_H = 720;
		pxContextFramebufferRef prevFbo = context.getCurrentFramebuffer();
		pxError replacedFbo = PX_NOTINITIALIZED;
		bool existingFbo = false;
		if (prevFbo.getPtr() != gAampFbo.getPtr())
		{
			replacedFbo = context.setFramebuffer(gAampFbo);
		}
		else
		{
			existingFbo = true;
		}
		if (existingFbo || replacedFbo == PX_OK)
		{
			static std::vector<uint8_t> rgbVect;
			int rgbLen = pixel_w*pixel_h*3;
			if(rgbVect.size() < rgbLen)
			{
				rgbVect.resize(rgbLen);
			}
			uint8_t *buffer_convert = rgbVect.data();
//			printf("AAMP: Buffer size=%d pixel_w=%d pixel_h=%d.\n",size, pixel_w, pixel_h);
			CONVERT_YUV420PtoRGB24(yuvBuffer,buffer_convert,pixel_w,pixel_h);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, id_y);

			glClearColor(0.0,1.0,0.0,1.0);
			glClear(GL_COLOR_BUFFER_BIT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			glTexImage2D(GL_TEXTURE_2D, 0, 3, pixel_w, pixel_h, 0, GL_RGB, GL_UNSIGNED_BYTE,buffer_convert);

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glEnable(GL_TEXTURE_2D);
			glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

			glBegin(GL_QUADS);

				glTexCoord2f(0.0, 1.0);
				glVertex3f(-1.0, -1.0, 0.0);

				glTexCoord2f(0.0, 0.0);
				glVertex3f(-1.0, 1.0, 0.0);

				glTexCoord2f(1.0, 0.0);
				glVertex3f(1.0, 1.0, 0.0);

				glTexCoord2f(1.0,1.0);
				glVertex3f(1.0, -1.0, 0.0);

			glEnd();

			glDisable(GL_TEXTURE_2D);
			glFlush();

//			printf("AAMP: Rendered frame.\n");
		}
		if(replacedFbo == PX_OK)
		{
			context.setFramebuffer(prevFbo);
		}
		gAampFboMutex.unlock();
		gUIThreadQueue->addTask(newAampFrame, pxVideo::pxVideoObj, NULL);
	}
}

void pxVideo::dispose(bool /*pumpJavascript*/)
{
  delete gPlayer; // delete player, but don't stop g loop
  gAampFboMutex.lock(); // clear all frames
  while (!frames.empty()) {
    pxOffscreen *frame = frames.front();
    delete frame;
    frames.pop();
  }
}

void pxVideo::draw()
{
  if (mEnablePunchThrough && !isRotated())
  {
    int screenX = 0;
    int screenY = 0;
    context.mapToScreenCoordinates(0,0,screenX, screenY);
    context.punchThrough(screenX,screenY,mw, mh);
  }
  else
  {
    static pxTextureRef nullMaskRef;
    gAampFboMutex.lock();
    if (NULL != gAampFbo)
    {
      context.drawImage(x(), y(), FBO_W, FBO_H, gAampFbo->getTexture(), nullMaskRef);
    }

    if (frames.size() > 0) {
      if (previousFrame) {
        delete previousFrame;
      }
      pxOffscreen *offscreen = frames.back(); // only fetch the last frame
      while (!frames.empty()) {
        pxOffscreen *t = frames.front();
        if (t != offscreen) {
          delete t;
        }
        frames.pop();
      }
      previousFrame = offscreen;
    }
    gAampFboMutex.unlock();
    if (previousFrame)
      context.drawImage(0, 0, mw, mh, context.createTexture(*previousFrame), nullMaskRef, false);
  }
}

bool pxVideo::isRotated()
{
  pxMatrix4f matrix = context.getMatrix();
  float *f = matrix.data();
  const float e= 1.0e-2;

  if ( (fabsf(f[1]) > e) ||
       (fabsf(f[2]) > e) ||
       (fabsf(f[4]) > e) ||
       (fabsf(f[6]) > e) ||
       (fabsf(f[8]) > e) ||
       (fabsf(f[9]) > e) )
  {
    return true;
  }

  return false;
}

void pxVideo::update(double t, bool updateChildren)
{
  if (mPreviousFrameTime <= 0) {
    mPreviousFrameTime = t;
  }

  mTimeTik += (t - mPreviousFrameTime);
  if (mTimeTik > 0.2 && gPlayer->isLoaded()) {
    mEmit.send("timeupdate");
    mTimeTik = 0;
  }

  mPreviousFrameTime = t;
  pxObject::update(t, updateChildren);
}
//properties
rtError pxVideo::availableAudioLanguages(rtObjectRef& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::availableClosedCaptionsLanguages(rtObjectRef& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::availableSpeeds(rtObjectRef& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::duration(float& v) const
{
  v = gPlayer->getDuration();
  return RT_OK;
}

rtError pxVideo::zoom(rtString& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setZoom(const char* /*s*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::playing(int &v) const
{
  v = gPlayer->isPlaying() ? 1 : 0;
  return RT_OK;
}

rtError pxVideo::volume(uint32_t& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setVolume(uint32_t /*v*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::closedCaptionsOptions(rtObjectRef& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setClosedCaptionsOptions(rtObjectRef /*v*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::closedCaptionsLanguage(rtString& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setClosedCaptionsLanguage(const char* /*s*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::contentOptions(rtObjectRef& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setContentOptions(rtObjectRef /*v*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::speed(float& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setSpeedProperty(float /*v*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::position(float& v) const
{
  v = gPlayer->getPosition();
  return RT_OK;
}

rtError pxVideo::setPosition(float v)
{
  gPlayer->setPosition(v);
  return RT_OK;
}

rtError pxVideo::audioLanguage(rtString& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setAudioLanguage(const char* /*s*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::secondaryAudioLanguage(rtString& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setSecondaryAudioLanguage(const char* /*s*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::url(rtString& url) const
{
	url = mUrl;
	return RT_OK;
}

rtError pxVideo::setUrl(const char* url)
{
	mUrl = rtString(url);
	rtLogError("%s:%d: URL[%s].",__FUNCTION__,__LINE__,url);

    if(!mUrl.isEmpty()){
        gPlayer->loadFile(mUrl.cString());
    }
	return RT_OK;
}

rtError pxVideo::tsbEnabled(bool& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setTsbEnabled(bool /*v*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::closedCaptionsEnabled(bool& /*v*/) const
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setClosedCaptionsEnabled(bool /*v*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::autoPlay(bool& autoPlay) const
{
	autoPlay = mAutoPlay;
	return RT_OK;
}

rtError pxVideo::setAutoPlay(bool value)
{
	mAutoPlay = value;
	rtLogError("%s:%d: autoPlay[%s].",__FUNCTION__,__LINE__,value?"TRUE":"FALSE");
	gPlayer->setAutoPlay(mAutoPlay);
	return RT_OK;
}

rtError pxVideo::play()
{
	rtLogError("%s:%d.",__FUNCTION__,__LINE__);
	if(!mUrl.isEmpty())
	{
//		mAamp->Tune(mUrl.cString());
	}

	gPlayer->play();
	return RT_OK;
}

rtError pxVideo::pause()
{
//	if(mAamp)
//	{
//		mAamp->SetRate(0);
//	}
  gPlayer->pause();
	return RT_OK;
}

rtError pxVideo::stop()
{
//	if(mAamp)
//	{
//		mAamp->Stop();
//	}
  gPlayer->release();
	return RT_OK;
}

rtError pxVideo::setSpeed(float /*speed*/, float /*overshootCorrection*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setPositionRelative(float /*seconds*/)
{
  //TODO
  return RT_OK;
}

rtError pxVideo::requestStatus()
{
  //TODO
  return RT_OK;
}

rtError pxVideo::setAdditionalAuth(rtObjectRef /*params*/)
{
  //TODO
  return RT_OK;
}

rtDefineObject(pxVideo, pxObject);
rtDefineProperty(pxVideo, availableAudioLanguages);
rtDefineProperty(pxVideo, availableClosedCaptionsLanguages);
rtDefineProperty(pxVideo, availableSpeeds);
rtDefineProperty(pxVideo, duration);
rtDefineProperty(pxVideo, playing);
rtDefineProperty(pxVideo, zoom);
rtDefineProperty(pxVideo, volume);
rtDefineProperty(pxVideo, closedCaptionsOptions);
rtDefineProperty(pxVideo, closedCaptionsLanguage);
rtDefineProperty(pxVideo, contentOptions);
rtDefineProperty(pxVideo, speed);
rtDefineProperty(pxVideo, position);
rtDefineProperty(pxVideo, audioLanguage);
rtDefineProperty(pxVideo, secondaryAudioLanguage);
rtDefineProperty(pxVideo, url);
rtDefineProperty(pxVideo, tsbEnabled);
rtDefineProperty(pxVideo, closedCaptionsEnabled);
rtDefineProperty(pxVideo, autoPlay);
rtDefineMethod(pxVideo, play);
rtDefineMethod(pxVideo, pause);
rtDefineMethod(pxVideo, stop);
rtDefineMethod(pxVideo, setSpeed);
rtDefineMethod(pxVideo, setPositionRelative);
rtDefineMethod(pxVideo, requestStatus);
rtDefineMethod(pxVideo, setAdditionalAuth);

