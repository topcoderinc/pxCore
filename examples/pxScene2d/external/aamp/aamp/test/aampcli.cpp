/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file aampcli.cpp
 * @brief Stand alone AAMP player with command line interface.
 */

#include <stdio.h>
#include <list>
#include <string.h>
#include <gst/gst.h>
#include <priv_aamp.h>
#include <main_aamp.h>
#include "../StreamAbstractionAAMP.h"

#ifdef IARM_MGR
#include "host.hpp"
#include "manager.hpp"
#include "libIBus.h"
#include "libIBusDaemon.h"
#endif

#ifdef __APPLE__
#import <cocoa_window.h>
#endif

static PlayerInstanceAAMP *mSingleton;
static GMainLoop *AAMPGstPlayerMainLoop = NULL;

/**
 * @struct VirtualChannelInfo
 * @brief Holds information of a virtual channel
 */
struct VirtualChannelInfo
{
	int channelNumber;
	std::string name;
	std::string uri;
};

static std::list<VirtualChannelInfo> mVirtualChannelMap;

/**
 * @brief Thread to run mainloop (for standalone mode)
 * @param[in] arg user_data
 * @retval void pointer
 */
static void* AAMPGstPlayer_StreamThread(void *arg);
static bool initialized = false;
GThread *aampMainLoopThread = NULL;


/**
 * @brief trim a string
 * @param[in][out] cmd Buffer containing string
 */
static void trim(char **cmd)
{
    std::string src = *cmd;
    size_t first = src.find_first_not_of(' ');
    if (first != std::string::npos)
    {
        size_t last = src.find_last_not_of(" \r\n");
        std::string dst = src.substr(first, (last - first + 1));
        strncpy(*cmd, (char*)dst.c_str(), dst.size());
        (*cmd)[dst.size()] = '\0';
    }
}

/**
 * @brief check if the char array is having numbers only
 * @param s
 * @retval true or false
 */
static bool isNumber(const char *s)
{
    if (*s)
    {
        if (*s == '-')
        { // skip leading minus
            s++;
        }
        for (;;)
        {
            if (*s >= '0' && *s <= '9')
            {
                s++;
                continue;
            }
            if (*s == 0x00)
            {
                return true;
            }
            break;
        }
    }
    return false;
}



/**
 * @brief Thread to run mainloop (for standalone mode)
 * @param[in] arg user_data
 * @retval void pointer
 */
static void* AAMPGstPlayer_StreamThread(void *arg)
{
	if (AAMPGstPlayerMainLoop)
	{
		g_main_loop_run(AAMPGstPlayerMainLoop); // blocks
		logprintf("AAMPGstPlayer_StreamThread: exited main event loop\n");
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
void InitPlayerLoop(int argc, char **argv)
{
	if (!initialized)
	{
		initialized = true;
		gst_init(&argc, &argv);
		AAMPGstPlayerMainLoop = g_main_loop_new(NULL, FALSE);
		aampMainLoopThread = g_thread_new("AAMPGstPlayerLoop", &AAMPGstPlayer_StreamThread, NULL );
	}
}

/**
 * @brief Stop mainloop execution (for standalone mode)
 */
void TermPlayerLoop()
{
	if(AAMPGstPlayerMainLoop)
	{
		g_main_loop_quit(AAMPGstPlayerMainLoop);
		g_thread_join(aampMainLoopThread);
		gst_deinit ();
		logprintf("%s(): Exit\n", __FUNCTION__);
	}
}

/**
 * @brief Show help menu with aamp command line interface
 */
static void ShowHelp(void)
{
	int i = 0;
	if (!mVirtualChannelMap.empty())
	{
		logprintf("\nChannel Map from aampcli.cfg\n*************************\n");

		for (std::list<VirtualChannelInfo>::iterator it = mVirtualChannelMap.begin(); it != mVirtualChannelMap.end(); ++it, ++i)
		{
			VirtualChannelInfo &pChannelInfo = *it;
			printf("%4d: %s", pChannelInfo.channelNumber, pChannelInfo.name.c_str());
			if ((i % 4) == 3)
			{
				printf("\n");
			}
			else
			{
				printf("\t");
			}
		}
		printf("\n");
	}

	logprintf("List of Commands\n****************\n");
	logprintf("<channelNumber> // Play selected channel from guide\n");
	logprintf("<url> // Play arbitrary stream\n");
	logprintf("info gst trace curl progress // Logging toggles\n");
	logprintf("pause play stop status flush // Playback options\n");
	logprintf("sf, ff<x> rw<y> // Trickmodes (x- 16, 32. y- 4, 8, 16, 32)\n");
	logprintf("+ - // Change profile\n");
	logprintf("sap // Use SAP track (if avail)\n");
	logprintf("seek <seconds> // Specify start time within manifest\n");
	logprintf("live // Seek to live point\n");
	logprintf("underflow // Simulate underflow\n");
	logprintf("help // Show this list again\n");
	logprintf("exit // Exit from application\n");
}


//#define LOG_CLI_EVENTS
#ifdef LOG_CLI_EVENTS
static class PlayerInstanceAAMP *mpPlayerInstanceAAMP;

/**
 * @class myAAMPEventListener
 * @brief
 */
class myAAMPEventListener :public AAMPEventListener
{
public:

	/**
	 * @brief Implementation of event callback
	 * @param e Event
	 */
	void Event(const AAMPEvent & e)
	{
		switch (e.type)
		{
		case AAMP_EVENT_TUNED:
			logprintf("AAMP_EVENT_TUNED\n");
			break;
		case AAMP_EVENT_TUNE_FAILED:
			logprintf("AAMP_EVENT_TUNE_FAILED\n");
			break;
		case AAMP_EVENT_SPEED_CHANGED:
			logprintf("AAMP_EVENT_SPEED_CHANGED\n");
			break;
		case AAMP_EVENT_DRM_METADATA:
                        logprintf("AAMP_DRM_FAILED\n");
                        break;
		case AAMP_EVENT_EOS:
			logprintf("AAMP_EVENT_EOS\n");
			break;
		case AAMP_EVENT_PLAYLIST_INDEXED:
			logprintf("AAMP_EVENT_PLAYLIST_INDEXED\n");
			break;
		case AAMP_EVENT_PROGRESS:
			//			logprintf("AAMP_EVENT_PROGRESS\n");
			break;
		case AAMP_EVENT_CC_HANDLE_RECEIVED:
			logprintf("AAMP_EVENT_CC_HANDLE_RECEIVED\n");
			break;
		case AAMP_EVENT_BITRATE_CHANGED:
			logprintf("AAMP_EVENT_BITRATE_CHANGED\n");
			break;
		}
	}
}; // myAAMPEventListener

static class myAAMPEventListener *myEventListener;
#endif

/**
 * @brief Parse config entries for aamp-cli, and update gpGlobalConfig params
 *        based on the config.
 * @param cfg config to process
 */
static void ProcessCLIConfEntry(char *cfg)
{
	trim(&cfg);
	if (cfg[0] == '*')
	{
			char *delim = strchr(cfg, ' ');
			if (delim)
			{
				//Populate channel map from aampcli.cfg
				VirtualChannelInfo channelInfo;
				channelInfo.channelNumber = INT_MIN;
				char *channelStr = &cfg[1];
				char *token = strtok(channelStr, " ");
				while (token != NULL)
				{
					if (isNumber(token))
						channelInfo.channelNumber = atoi(token);
					else if (memcmp(token, "http", 4) == 0)
							channelInfo.uri = token;
					else
						channelInfo.name = token;
					token = strtok(NULL, " ");
				}
				if (!channelInfo.uri.empty())
				{
					if (INT_MIN == channelInfo.channelNumber)
					{
						channelInfo.channelNumber = mVirtualChannelMap.size() + 1;
					}
					if (channelInfo.name.empty())
					{
						channelInfo.name = "CH" + std::to_string(channelInfo.channelNumber);
					}
					mVirtualChannelMap.push_back(channelInfo);
				}
				else
				{
					logprintf("%s(): Could not parse uri of %s\n", __FUNCTION__, cfg);
				}
			}
	}
}

/**
 * @brief Process command
 * @param cmd command
 */
static void ProcessCliCommand(char *cmd)
{
	double seconds = 0;
	int rate = 0;
	trim(&cmd);
	if (cmd[0] == 0)
	{
		if (mSingleton->aamp->mpStreamAbstractionAAMP)
		{
			mSingleton->aamp->mpStreamAbstractionAAMP->DumpProfiles();
		}
		logprintf("current bitrate ~= %ld\n", mSingleton->aamp->GetCurrentlyAvailableBandwidth());
	}
	else if (strcmp(cmd, "help") == 0)
	{
		ShowHelp();
	}
	else if (memcmp(cmd, "http", 4) == 0)
	{
		mSingleton->Tune(cmd);
	}
	else if (isNumber(cmd))
	{
		int channelNumber = atoi(cmd);
		logprintf("channel number: %d\n", channelNumber);
		for (std::list<VirtualChannelInfo>::iterator it = mVirtualChannelMap.begin(); it != mVirtualChannelMap.end(); ++it)
		{
			VirtualChannelInfo &channelInfo = *it;
			if(channelInfo.channelNumber == channelNumber)
			{
			//	logprintf("Found %d tuning to %s\n",channelInfo.channelNumber, channelInfo.uri.c_str());
				mSingleton->Tune(channelInfo.uri.c_str());
				break;
			}
		}
	}
	else if (sscanf(cmd, "seek %lf", &seconds) == 1)
	{
		mSingleton->Seek(seconds);
	}
	else if (strcmp(cmd, "sf") == 0)
	{
		mSingleton->SetRate(0.5);
	}
	else if (sscanf(cmd, "ff%d", &rate) == 1)
	{
		if (rate != 4 && rate != 16 && rate != 32)
		{
			logprintf("Speed not supported.\n");
		}
		else
		{
			mSingleton->SetRate((float)rate);
		}
	}
	else if (strcmp(cmd, "play") == 0)
	{
		mSingleton->SetRate(1);
	}
	else if (strcmp(cmd, "pause") == 0)
	{
		mSingleton->SetRate(0);
	}
	else if (sscanf(cmd, "rw%d", &rate) == 1)
	{
		if ((rate < 4 || rate > 32) || (rate % 4))
		{
			logprintf("Speed not supported.\n");
		}
		else
		{
			mSingleton->SetRate((float)(-rate));
		}
	}
	else if (sscanf(cmd, "bps %d", &rate) == 1)
	{
		logprintf("Set video bitrate %d.\n", rate);
		mSingleton->SetVideoBitrate(rate);
	}
	else if (strcmp(cmd, "flush") == 0)
	{
		mSingleton->aamp->mStreamSink->Flush();
	}
	else if (strcmp(cmd, "stop") == 0)
	{
		mSingleton->Stop();
	}
	else if (strcmp(cmd, "underflow") == 0)
	{
		mSingleton->aamp->ScheduleRetune(eGST_ERROR_UNDERFLOW, eMEDIATYPE_VIDEO);
	}
	else if (strcmp(cmd, "retune") == 0)
	{
		mSingleton->aamp->ScheduleRetune(eDASH_ERROR_STARTTIME_RESET, eMEDIATYPE_VIDEO);
	}
	else if (strcmp(cmd, "status") == 0)
	{
		mSingleton->aamp->mStreamSink->DumpStatus();
	}
	else if (strcmp(cmd, "live") == 0)
	{
		mSingleton->SeekToLive();
	}
	else if (strcmp(cmd, "exit") == 0)
	{
		mSingleton->Stop();
		delete mSingleton;
		mVirtualChannelMap.clear();
		TermPlayerLoop();
		exit(0);
	}
	else if (memcmp(cmd, "rect", 4) == 0)
	{
		int x, y, w, h;
		if (sscanf(cmd, "rect %d %d %d %d", &x, &y, &w, &h) == 4)
		{
			mSingleton->SetVideoRectangle(x, y, w, h);
		}
	}
	else if (memcmp(cmd, "zoom", 4) == 0)
	{
		int zoom;
		if (sscanf(cmd, "zoom %d", &zoom) == 1)
		{
			if (zoom)
			{
				logprintf("Set zoom to full\n");
				mSingleton->SetVideoZoom(VIDEO_ZOOM_FULL);
			}
			else
			{
				logprintf("Set zoom to none\n");
				mSingleton->SetVideoZoom(VIDEO_ZOOM_NONE);
			}
		}
	}
	else if (strcmp(cmd, "sap") == 0)
	{
		gpGlobalConfig->SAP = !gpGlobalConfig->SAP;
		logprintf("SAP %s\n", gpGlobalConfig->SAP ? "on" : "off");
		if (gpGlobalConfig->SAP)
		{
			mSingleton->SetLanguage("es");
		}
		else
		{
			mSingleton->SetLanguage("en");
		}
	}
}

static void * run_commnds(void *arg)
{
    char cmd[MAX_URI_LENGTH * 2];
    ShowHelp();
    char *ret = NULL;
    do
    {
        logprintf("aamp-cli> ");
        if((ret = fgets(cmd, sizeof(cmd), stdin))!=NULL)
            ProcessCliCommand(cmd);
    } while (ret != NULL);
    return NULL;
}

/**
 * @brief
 * @param argc
 * @param argv
 * @retval
 */
int main(int argc, char **argv)
{

#ifdef IARM_MGR
	char Init_Str[] = "aamp-cli";
	IARM_Bus_Init(Init_Str);
	IARM_Bus_Connect();
	try
	{
		device::Manager::Initialize();
		logprintf("device::Manager::Initialize() succeeded\n");

	}
	catch (...)
	{
		logprintf("device::Manager::Initialize() failed\n");
	}
#endif
	char driveName = (*argv)[0];
	AampLogManager mLogManager;
	AampLogManager::disableLogRedirection = true;
	ABRManager mAbrManager;

	/* Set log directory path for AAMP and ABR Manager */
	mLogManager.setLogAndCfgDirectory(driveName);
	mAbrManager.setLogDirectory(driveName);

	logprintf("**************************************************************************\n");
	logprintf("** ADVANCED ADAPTIVE MICRO PLAYER (AAMP) - COMMAND LINE INTERFACE (CLI) **\n");
	logprintf("**************************************************************************\n");

	InitPlayerLoop(0,NULL);

	mSingleton = new PlayerInstanceAAMP();
	mSingleton->aamp->mUseImageSink = false;
#ifdef LOG_CLI_EVENTS
	myEventListener = new myAAMPEventListener();
	mSingleton->RegisterEvents(myEventListener);
#endif

#ifdef WIN32
	FILE *f = fopen(mLogManager.getAampCliCfgPath(), "rb");
#elif defined(__APPLE__)
	std::string cfgPath(getenv("HOME"));
	cfgPath += "/aampcli.cfg";
	FILE *f = fopen(cfgPath.c_str(), "rb");
#else
	FILE *f = fopen("/opt/aampcli.cfg", "rb");
#endif
	if (f)
	{
		logprintf("opened aampcli.cfg\n");
		char buf[MAX_URI_LENGTH * 2];
		while (fgets(buf, sizeof(buf), f))
		{
			ProcessCLIConfEntry(buf);
		}
		fclose(f);
	}

#ifdef __APPLE__
    pthread_t cmdThreadId;
    pthread_create(&cmdThreadId,NULL,run_commnds,NULL);
    createAndRunCocoaWindow();
    mSingleton->Stop();
    delete mSingleton;
#else
    run_commnds(NULL);
#endif
}
