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
* @file AampDRMSessionManager.h
* @brief Header file for DRM session manager
*/
#ifndef AampDRMSessionManager_h
#define AampDRMSessionManager_h

#include "aampdrmsessionfactory.h"
#include "AampDrmSession.h"
#include "AampDRMutils.h"
#include "main_aamp.h"
#include <string>
#include <curl/curl.h>

#ifdef USE_SECCLIENT
#include "sec_client.h"
#endif

/**
 * @addtogroup AAMP_DRM_TYPES
 * @{
 */

#define MAX_DRM_SESSIONS 2
#define VIDEO_SESSION 0
#define AUDIO_SESSION 1
#define KEYID_TAG_START "<KID>"
#define KEYID_TAG_END "</KID>"

/**
 *  @struct	DrmSessionContext
 *  @brief	To store drmSession and keyId data.
 */
struct DrmSessionContext
{
	size_t dataLength;
	unsigned char* data;
	AampDrmSession * drmSession;
};

/**
 *  @struct	KeyID
 *  @brief	Structure to hold, keyId and session creation time for
 *  		keyId
 */
struct KeyID
{
	size_t len;
	unsigned char* data;
	long long creationTime;
	bool isFailedKeyId;
};

/**
 *  @brief	Enum to represent session manager state.
 *  		Session manager would abort any createDrmSession
 *  		request if in eSESSIONMGR_INACTIVE state.
 */
typedef enum{
	eSESSIONMGR_INACTIVE,
	eSESSIONMGR_ACTIVE
}SessionMgrState;


/**
 *  @class	AampDRMSessionManager
 *  @brief	Controller for managing DRM sessions.
 */
class AampDRMSessionManager
{

private:

	static DrmSessionContext drmSessionContexts[MAX_DRM_SESSIONS];
	static KeyID cachedKeyIDs[MAX_DRM_SESSIONS];
	static size_t write_callback(char *ptr, size_t size, size_t nmemb,
			void *userdata);
	static char* accessToken;
	static int accessTokenLen;
	static SessionMgrState sessionMgrState;
public:

	void initializeDrmSessions();

	AampDRMSessionManager();

	~AampDRMSessionManager();

	AampDrmSession * createDrmSession(const char* systemId,
			const unsigned char * initDataPtr, uint16_t dataLength, MediaType streamType, PrivateInstanceAAMP* aamp, AAMPEvent *e);
	AampDrmSession * createDrmSession(const char* systemId,
			const unsigned char * initDataPtr, uint16_t dataLength, MediaType streamType,
			const unsigned char *contentMetadata, PrivateInstanceAAMP* aamp, AAMPEvent *e);

	DrmData * getLicense(DrmData * keyChallenge, string destinationURL, long *httpError, bool isComcastStream = false, char* licenseProxy = NULL);

	static void clearSessionData();

	static void clearAccessToken();

	static void clearFailedKeyIds();

	static void setSessionMgrState(SessionMgrState state);

	static const char* getAccessToken(int &tokenLength, long &error_code);
};

unsigned char * _extractDataFromPssh(const char* psshData, int dataLength,
											const char* startStr, const char* endStr, int *len);
unsigned char * _extractKeyIdFromPssh(const char* psshData, int dataLength, int *len, bool isWidevine);
unsigned char * _extractWVContentMetadataFromPssh(const char* psshData, int dataLength, int *len);

#endif

/**
 * @}
 */
