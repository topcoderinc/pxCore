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
 * @file fragmentcollector_mpd.cpp
 * @brief Fragment collector implementation of MPEG DASH
 */


#include "fragmentcollector_mpd.h"
#include "priv_aamp.h"
#include "AampDRMSessionManager.h"
#include <stdlib.h>
#include <string.h>
#include "_base64.h"
#include "libdash/IMPD.h"
#include "libdash/INode.h"
#include "libdash/IDASHManager.h"
#include "libdash/xml/Node.h"
#include "libdash/helpers/Time.h"
#include "libdash/xml/DOMParser.h"
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>
#include <set>
#include <iomanip>
#include <ctime>
#include <inttypes.h>
#include <libxml/xmlreader.h>
//#define DEBUG_TIMELINE
//#define AAMP_HARVEST_SUPPORT_ENABLED
//#define AAMP_DISABLE_INJECT
//#define HARVEST_MPD

using namespace dash;
using namespace std;
using namespace dash::mpd;
using namespace dash::xml;
using namespace dash::helpers;

/**
 * @addtogroup AAMP_COMMON_TYPES
 * @{
 */

#define MAX_ID_SIZE 1024
#define SEGMENT_COUNT_FOR_ABR_CHECK 5
#define PLAYREADY_SYSTEM_ID "9a04f079-9840-4286-ab92-e65be0885f95"
#define WIDEVINE_SYSTEM_ID "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"
#define DEFAULT_INTERVAL_BETWEEN_MPD_UPDATES_MS 3000
#define TIMELINE_START_RESET_DIFF 4000000000
#define MAX_DELAY_BETWEEN_MPD_UPDATE_MS (6000)
#define MIN_DELAY_BETWEEN_MPD_UPDATE_MS (500) // 500mSec

//Comcast DRM Agnostic CENC for Content Metadata
#define COMCAST_DRM_INFO_ID "afbcb50e-bf74-3d13-be8f-13930c783962"

/**
 * @struct FragmentDescriptor
 * @brief Stores information of dash fragment
 */
struct FragmentDescriptor
{
	const char *manifestUrl;
	const std::vector<IBaseUrl *>*baseUrls;
	uint32_t Bandwidth;
	char RepresentationID[MAX_ID_SIZE]; // todo: retrieve from representation instead of making a copy
	uint64_t Number;
	uint64_t Time;
};

/**
 * @struct PeriodInfo
 * @brief Stores details about available periods in mpd
 */

struct PeriodInfo {
	std::string periodId;
	uint64_t startTime;
	double duration;

	PeriodInfo() {
		periodId = "";
		startTime = 0;
		duration = 0.0;
	}
};


static const char *mMediaTypeName[] = { "video", "audio" };

#ifdef AAMP_HARVEST_SUPPORT_ENABLED
#ifdef USE_PLAYERSINKBIN
#define HARVEST_BASE_PATH "/media/tsb/aamp-harvest/" // SD card friendly path
#else
#define HARVEST_BASE_PATH "aamp-harvest/"
#endif
static void GetFilePath(char filePath[MAX_URI_LENGTH], const FragmentDescriptor *fragmentDescriptor, std::string media);
static void WriteFile(char* fileName, const char* data, int len);
#endif // AAMP_HARVEST_SUPPORT_ENABLED

/**
 * @class MediaStreamContext
 * @brief MPD media track
 */
class MediaStreamContext : public MediaTrack
{
public:

/**
 * @}
 */

/**
 * @addtogroup AAMP_COMMON_API
 * @{
 */

	/**
	 * @brief MediaStreamContext Constructor
         *
	 * @param type Type of track
	 * @param context  MPD collector context
	 * @param aamp Pointer to associated aamp instance
	 * @param name Name of the track
	 */
	MediaStreamContext(TrackType type, StreamAbstractionAAMP_MPD* context, PrivateInstanceAAMP* aamp, const char* name) :
			MediaTrack(type, aamp, name),
			mediaType((MediaType)type), adaptationSet(NULL), representation(NULL),
			fragmentIndex(0), timeLineIndex(0), fragmentRepeatCount(0), fragmentOffset(0),
			eos(false), endTimeReached(false), fragmentTime(0),targetDnldPosition(0), index_ptr(NULL), index_len(0),
			lastSegmentTime(0), lastSegmentNumber(0), adaptationSetIdx(0), representationIndex(0), profileChanged(true),
			adaptationSetId(0)
	{
		mContext = context;
		memset(&fragmentDescriptor, 0, sizeof(FragmentDescriptor));
	}

	/**
	 * @brief MediaStreamContext Destructor
	 */
	~MediaStreamContext(){}


	/**
	 * @brief Get the context of media track. To be implemented by subclasses
	 * @retval Context of track.
	 */
	StreamAbstractionAAMP* GetContext()
	{
		return mContext;
	}

	/**
	 * @brief Receives cached fragment and injects to sink.
	 *
	 * @param[in]  cachedFragment      Contains fragment to be processed and injected
	 * @param[out] fragmentDiscarded   True if fragment is discarded.
	 */
	void InjectFragmentInternal(CachedFragment* cachedFragment, bool &fragmentDiscarded)
	{
#ifndef AAMP_DISABLE_INJECT
		aamp->SendStream((MediaType)type, &cachedFragment->fragment,
					cachedFragment->position, cachedFragment->position, cachedFragment->duration);
#endif
		fragmentDiscarded = false;
	} // InjectFragmentInternal

	/**
	 * @brief Fetch and cache a fragment
         *
	 * @param[in] fragmentUrl   URL of fragment
	 * @param[in] curlInstance  curl instance to be used to fetch
	 * @param[in] position      Position of fragment in seconds
	 * @param[in] duration      Duration of fragment in seconds
	 * @param[in] range         Byte range
	 * @param[in] initSegment   True if fragment is init fragment
	 * @param[in] discontinuity True if fragment is discontinuous
         *
	 * @retval true on success
	 */
	bool CacheFragment(const char *fragmentUrl, unsigned int curlInstance, double position, double duration, const char *range = NULL, bool initSegment= false, bool discontinuity = false
#ifdef AAMP_HARVEST_SUPPORT_ENABLED
		, std::string media = 0
#endif
	)
	{
		bool ret = false;

		fragmentDurationSeconds = duration;
		ProfilerBucketType bucketType = aamp->GetProfilerBucketForMedia(mediaType, initSegment);
		CachedFragment* cachedFragment = GetFetchBuffer(true);
		long http_code = 0;
		MediaType actualType = (MediaType)(initSegment?(eMEDIATYPE_INIT_VIDEO+mediaType):mediaType); //Need to revisit the logic
		ret = aamp->LoadFragment(bucketType, fragmentUrl, &cachedFragment->fragment, curlInstance,
			        range, actualType, &http_code);

		mContext->mCheckForRampdown = false;

		if (!ret)
		{
			aamp_Free(&cachedFragment->fragment.ptr);
			if( aamp->DownloadsAreEnabled())
			{
				logprintf("%s:%d LoadFragment failed\n", __FUNCTION__, __LINE__);
				if (initSegment)
				{
					logprintf("%s:%d Init fragment fetch failed. fragmentUrl %s\n", __FUNCTION__, __LINE__, fragmentUrl);
					aamp->SendDownloadErrorEvent(AAMP_TUNE_INIT_FRAGMENT_DOWNLOAD_FAILURE, http_code);
				}
				else
				{
					segDLFailCount += 1;
					if (MAX_SEG_DOWNLOAD_FAIL_COUNT <= segDLFailCount)
					{
						logprintf("%s:%d Not able to download fragments; reached failure threshold sending tune failed event\n",
								__FUNCTION__, __LINE__);
						aamp->SendDownloadErrorEvent(AAMP_TUNE_FRAGMENT_DOWNLOAD_FAILURE, http_code);
					} 
					// DELIA-32287 - Profile RampDown check and rampdown is needed only for Video . If audio fragment download fails 
					// should continue with next fragment,no retry needed .
					else if ((eTRACK_VIDEO == type) && mContext->CheckForRampDownProfile(http_code))
					{
						mContext->mCheckForRampdown = true;
						logprintf( "PrivateStreamAbstractionMPD::%s:%d > Error while fetching fragment:%s, failedCount:%d. decrementing profile\n",
								__FUNCTION__, __LINE__, fragmentUrl, segDLFailCount);
					}
					else if (AAMP_IS_LOG_WORTHY_ERROR(http_code))
					{
						logprintf("PrivateStreamAbstractionMPD::%s:%d > Error on fetching %s fragment. failedCount:%d\n",
								__FUNCTION__, __LINE__, name, segDLFailCount);
					}
				}
			}
		}
		else
		{
#ifdef AAMP_HARVEST_SUPPORT_ENABLED
			if (aamp->HarvestFragments())
			{
				char fileName[MAX_URI_LENGTH];
				strcpy(fileName, fragmentUrl);
				GetFilePath(fileName, &fragmentDescriptor, media);
				logprintf("%s:%d filePath %s\n", __FUNCTION__, __LINE__, fileName);
				WriteFile(fileName, cachedFragment->fragment.ptr, cachedFragment->fragment.len);
			}
#endif
			cachedFragment->position = position;
			cachedFragment->duration = duration;
			cachedFragment->discontinuity = discontinuity;
#ifdef AAMP_DEBUG_INJECT
			if (discontinuity)
			{
				logprintf("%s:%d Discontinuous fragment\n", __FUNCTION__, __LINE__);
			}
			if ((1 << type) & AAMP_DEBUG_INJECT)
			{
				strcpy(cachedFragment->uri, fragmentUrl);
			}
#endif
			segDLFailCount = 0;
			UpdateTSAfterFetch();
			ret = true;
		}
		return ret;
	}


	/**
	 * @brief Listener to ABR profile change
	 */
	void ABRProfileChanged(void)
	{
		if (representationIndex != mContext->currentProfileIndex)
		{
			IRepresentation *pNewRepresentation = adaptationSet->GetRepresentation().at(mContext->currentProfileIndex);
			logprintf("PrivateStreamAbstractionMPD::%s:%d - ABR %dx%d[%d] -> %dx%d[%d]\n", __FUNCTION__, __LINE__,
					representation->GetWidth(), representation->GetHeight(), representation->GetBandwidth(),
					pNewRepresentation->GetWidth(), pNewRepresentation->GetHeight(), pNewRepresentation->GetBandwidth());
			representationIndex = mContext->currentProfileIndex;
			representation = adaptationSet->GetRepresentation().at(mContext->currentProfileIndex);
			const std::vector<IBaseUrl *>*baseUrls = &representation->GetBaseURLs();
			if (baseUrls->size() != 0)
			{
				fragmentDescriptor.baseUrls = &representation->GetBaseURLs();
			}
			fragmentDescriptor.Bandwidth = representation->GetBandwidth();
			strcpy(fragmentDescriptor.RepresentationID, representation->GetId().c_str());
			profileChanged = true;
		}
		else
		{
			traceprintf("PrivateStreamAbstractionMPD::%s:%d - Not switching ABR %dx%d[%d] \n", __FUNCTION__, __LINE__,
					representation->GetWidth(), representation->GetHeight(), representation->GetBandwidth());
		}

	}

	MediaType mediaType;
	struct FragmentDescriptor fragmentDescriptor;
	IAdaptationSet *adaptationSet;
	IRepresentation *representation;
	int fragmentIndex;
	int timeLineIndex;
	int fragmentRepeatCount;
	int fragmentOffset;
	bool eos;
	bool endTimeReached;
	bool profileChanged;

	double fragmentTime;
	double targetDnldPosition;
	char *index_ptr;
	size_t index_len;
	uint64_t lastSegmentTime;
	uint64_t lastSegmentNumber;
	int adaptationSetIdx;
	int representationIndex;
	StreamAbstractionAAMP_MPD* mContext;
	std::string initialization;
	uint32_t adaptationSetId;
};

/**
 * @}
 */


/**
 * @addtogroup AAMP_COMMON_TYPES
 * @{
 */

/**
 * @struct HeaderFetchParams
 * @brief Holds information regarding initialization fragment
 */
struct HeaderFetchParams
{
	class PrivateStreamAbstractionMPD *context;
	struct MediaStreamContext *pMediaStreamContext;
	string initialization;
	double fragmentduration;
	bool isinitialization;
	bool discontinuity;
};

/**
 * @struct FragmentDownloadParams
 * @brief Holds data of fragment to be downloaded
 */
struct FragmentDownloadParams
{
	class PrivateStreamAbstractionMPD *context;
	struct MediaStreamContext *pMediaStreamContext;
	bool playingLastPeriod;
	long long lastPlaylistUpdateMS;
};

/**
 * @struct DrmSessionParams
 * @brief Holds data regarding drm session
 */
struct DrmSessionParams
{
	unsigned char *initData;
	int initDataLen;
	MediaType stream_type;
	PrivateInstanceAAMP *aamp;
	bool isWidevine;
	unsigned char *contentMetadata;
};

static bool IsIframeTrack(IAdaptationSet *adaptationSet);

/**
 * @}
 */


/**
 * @class PrivateStreamAbstractionMPD
 * @brief Private implementation of MPD fragment collector
 */
class PrivateStreamAbstractionMPD
{
public:
	PrivateStreamAbstractionMPD( StreamAbstractionAAMP_MPD* context, PrivateInstanceAAMP *aamp,double seekpos, float rate);
	~PrivateStreamAbstractionMPD();
	void SetEndPos(double endPosition);
	void Start();
	void Stop();
	AAMPStatusType Init(TuneType tuneType);
	void GetStreamFormat(StreamOutputFormat &primaryOutputFormat, StreamOutputFormat &audioOutputFormat);

	/**
	 * @brief Get current stream position.
	 *
	 * @retval current position of stream.
	 */
	double GetStreamPosition() { return seekPosition; }

	void FetcherLoop();
	bool PushNextFragment( MediaStreamContext *pMediaStreamContext, unsigned int curlInstance = 0);
	bool FetchFragment(MediaStreamContext *pMediaStreamContext, std::string media, double fragmentDuration, bool isInitializationSegment, unsigned int curlInstance = 0, bool discontinuity = false );
	uint64_t GetPeriodEndTime();
	int GetProfileCount();
	StreamInfo* GetStreamInfo(int idx);
	MediaTrack* GetMediaTrack(TrackType type);
	double GetFirstPTS();
	int64_t GetMinUpdateDuration() { return mMinUpdateDurationMs;}
	PrivateInstanceAAMP *aamp;

	int GetBWIndex(long bitrate);
	std::vector<long> GetVideoBitrates(void);
	std::vector<long> GetAudioBitrates(void);
	void StopInjection();
	void StartInjection();

private:
	AAMPStatusType UpdateMPD(bool retrievePlaylistFromCache = false);
	void FindTimedMetadata(MPD* mpd, Node* root);
	void ProcessPeriodSupplementalProperty(Node* node, std::string& AdID, uint64_t startMS, uint64_t durationMS);
	void ProcessPeriodAssetIdentifier(Node* node, uint64_t startMS, uint64_t durationMS, std::string& assetID, std::string& providerID);
	void ProcessStreamRestrictionList(Node* node, const std::string& AdID, uint64_t startMS);
	void ProcessStreamRestriction(Node* node, const std::string& AdID, uint64_t startMS);
	void ProcessStreamRestrictionExt(Node* node, const std::string& AdID, uint64_t startMS);
	void ProcessTrickModeRestriction(Node* node, const std::string& AdID, uint64_t startMS);
	void FetchAndInjectInitialization(bool discontinuity = false);
	void StreamSelection(bool newTune = false);
	bool CheckForInitalClearPeriod();
	void PushEncryptedHeaders();
	void UpdateTrackInfo(bool modifyDefaultBW, bool periodChanged, bool resetTimeLineIndex=false);
	double SkipFragments( MediaStreamContext *pMediaStreamContext, double skipTime, bool updateFirstPTS = false);
	void SkipToEnd( MediaStreamContext *pMediaStreamContext); //Added to support rewind in multiperiod assets
	void ProcessContentProtection(IAdaptationSet * adaptationSet,MediaType mediaType);
	void SeekInPeriod( double seekPositionSeconds);
	uint64_t GetDurationFromRepresentation();
	void UpdateCullingState();
	void UpdateLanguageList();

	bool fragmentCollectorThreadStarted;
	std::set<std::string> mLangList;
	double seekPosition;
	float rate;
	pthread_t fragmentCollectorThreadID;
	pthread_t createDRMSessionThreadID;
	bool drmSessionThreadStarted;
	dash::mpd::IMPD *mpd;
	MediaStreamContext *mMediaStreamContext[AAMP_TRACK_COUNT];
	int mNumberOfTracks;
	int mCurrentPeriodIdx;
	double mEndPosition;
	bool mIsLive;
	StreamAbstractionAAMP_MPD* mContext;
	StreamInfo* mStreamInfo;
	double mPrevStartTimeSeconds;
	std::string mPrevLastSegurlMedia;
	long mPrevLastSegurlOffset; //duration offset from beginning of TSB
	unsigned char *lastProcessedKeyId;
	int lastProcessedKeyIdLen;
	uint64_t mPeriodEndTime;
	uint64_t mPeriodStartTime;
	int64_t mMinUpdateDurationMs;
	uint64_t mLastPlaylistDownloadTimeMs;
	double mFirstPTS;
	AudioType mAudioType;
	std::string mPeriodId;
	bool mPushEncInitFragment;
	int mPrevAdaptationSetCount;
	std::unordered_map<long, int> mBitrateIndexMap;
	bool mIsFogTSB;
	bool mIsIframeTrackPresent;
	vector<PeriodInfo> mMPDPeriodsInfo;
};

/**
 * @addtogroup AAMP_COMMON_API
 * @{
 */

/**
 * @brief PrivateStreamAbstractionMPD Constructor
 *
 * @param[in] context  MPD fragment collector context
 * @param[in] aamp     Pointer to associated aamp private object
 * @param[in] seekpos  Seek positon
 * @param[in] rate     Playback rate
 */
PrivateStreamAbstractionMPD::PrivateStreamAbstractionMPD( StreamAbstractionAAMP_MPD* context, PrivateInstanceAAMP *aamp,double seekpos, float rate)
{
	this->aamp = aamp;
	seekPosition = seekpos;
	this->rate = rate;
	fragmentCollectorThreadID = 0;
	createDRMSessionThreadID = 0;
	mpd = NULL;
	fragmentCollectorThreadStarted = false;
	drmSessionThreadStarted = false;
	memset(&mMediaStreamContext, 0, sizeof(mMediaStreamContext));
	mNumberOfTracks = 0;
	mCurrentPeriodIdx = 0;
	mEndPosition = 0;
	mIsLive = true;
	mContext = context;
	mStreamInfo = NULL;
	mContext->GetABRManager().clearProfiles();
	mPrevStartTimeSeconds = 0;
	mPrevLastSegurlOffset = 0;
	mPeriodEndTime = 0;
	mPeriodStartTime = 0;
	lastProcessedKeyId = NULL;
	lastProcessedKeyIdLen = 0;
	mFirstPTS = 0;
	mAudioType = eAUDIO_UNKNOWN;
	mPushEncInitFragment = false;
	mMinUpdateDurationMs = DEFAULT_INTERVAL_BETWEEN_MPD_UPDATES_MS;
	mLastPlaylistDownloadTimeMs = aamp_GetCurrentTimeMS();
	mPrevAdaptationSetCount = 0;
	mIsFogTSB = false;
	mIsIframeTrackPresent = false;
};


/**
 * @brief Check if mime type is compatible with media type
 *
 * @param[in] mimeType   Mime type
 * @param[in] mediaType  Media type
 *
 * @retval true if compatible
 */
static bool IsCompatibleMimeType(std::string mimeType, MediaType mediaType)
{
	switch ( mediaType )
	{
	case eMEDIATYPE_VIDEO:
		if (mimeType == "video/mp4")
		{
			return true;
		}
		break;

	case eMEDIATYPE_AUDIO:
		if (mimeType == "audio/webm")
		{
			return true;
		}
		if (mimeType == "audio/mp4")
		{
			return true;
		}
		break;
	}
	return false;
}


/**
 * @brief Get representation index of desired codec
 *
 * @param[in]  adaptationSet   Adaptation set object
 * @param[out] selectedRepType Type of desired representation
 *
 * @retval index of desired representation
 */
static int GetDesiredCodecIndex(IAdaptationSet *adaptationSet, AudioType &selectedRepType)
{
	const std::vector<IRepresentation *> representation = adaptationSet->GetRepresentation();
	int selectedRepIdx = -1;
	uint32_t selectedRepBandwidth = 0;
	// check for codec defined in Adaptation Set
	const std::vector<string> adapCodecs = adaptationSet->GetCodecs();
	for (int representationIndex = 0; representationIndex < representation.size(); representationIndex++)
	{
		const dash::mpd::IRepresentation *rep = representation.at(representationIndex);
		uint32_t bandwidth = rep->GetBandwidth();
		const std::vector<string> codecs = rep->GetCodecs();
		AudioType audioType = eAUDIO_UNKNOWN;
		string codecValue="";
		// check if Representation includec codec
		if(codecs.size())
			codecValue=codecs.at(0);
		else if(adapCodecs.size()) // else check if Adaptation has codec defn
			codecValue = adapCodecs.at(0);
		// else no codec defined , go with unknown

		if (codecValue == "ec+3")
		{
#ifndef __APPLE__
			audioType = eAUDIO_ATMOS;
#endif
		}
		else if (codecValue == "ec-3")
		{
#ifndef __APPLE__
			audioType = eAUDIO_DDPLUS;
#endif
		}
		else // if( codecValue == "aac" || codecValue.find("mp4") != std::string::npos ) // needed?
		{
			audioType = eAUDIO_AAC;
		}
		/*
		* By default the audio profile selection priority is set as ATMOS then DD+ then AAC
		* Note that this check comes after the check of selected language.
		* disableATMOS: avoid use of ATMOS track
		* disableEC3: avoid use of DDPLUS and ATMOS tracks
		*/
		if (selectedRepType == eAUDIO_UNKNOWN || // anything better than nothing
			(selectedRepType == audioType && bandwidth>selectedRepBandwidth) || // same type but better quality
			(selectedRepType < eAUDIO_ATMOS && audioType == eAUDIO_ATMOS && !gpGlobalConfig->disableATMOS && !gpGlobalConfig->disableEC3) || // promote to atmos
			(selectedRepType < eAUDIO_DDPLUS && audioType == eAUDIO_DDPLUS && !gpGlobalConfig->disableEC3) || // promote to ddplus
			(selectedRepType != eAUDIO_AAC && audioType == eAUDIO_AAC && gpGlobalConfig->disableEC3) // force AAC
			)
		{
			selectedRepIdx = representationIndex;
			selectedRepType = audioType;
			selectedRepBandwidth = bandwidth;
		}
	}
	return selectedRepIdx;
}


/**
 * @brief Check if adaptation set is of a given media type
 *
 * @param[in]  adaptationSet Adaptation set
 * @param[in]  mediaType     Media type
 *
 * @retval true if adaptation set is of the given media type
 */
static bool IsContentType(IAdaptationSet *adaptationSet, MediaType mediaType )
{
	const char *name = mMediaTypeName[mediaType];

	if (adaptationSet->GetContentType() == name)
	{
		return true;
	}
	else if (adaptationSet->GetContentType() == "muxed")
	{
		logprintf("excluding muxed content\n");
	}
	else
	{
		if (IsCompatibleMimeType(adaptationSet->GetMimeType(), mediaType) )
		{
			return true;
		}
		const std::vector<IRepresentation *>representation = adaptationSet->GetRepresentation();
		for (int i = 0; i < representation.size(); i++)
		{
			if (IsCompatibleMimeType(representation.at(i)->GetMimeType(), mediaType) )
			{
				return true;
			}
		}

		const std::vector<IContentComponent *>contentComponent = adaptationSet->GetContentComponent();
		for( int i = 0; i < contentComponent.size(); i++)
		{
			if (contentComponent.at(i)->GetContentType() == name)
			{
				return true;
			}
		}
	}
	return false;
}

/**
 * @brief Read unsigned 32 bit value and update buffer pointer
 *
 * @param[in][out] pptr buffer
 *
 * @retval 32 bit value
 */
static unsigned int Read32( const char **pptr)
{
	const char *ptr = *pptr;
	unsigned int rc = 0;
	for (int i = 0; i < 4; i++)
	{
		rc <<= 8;
		rc |= (unsigned char)*ptr++;
	}
	*pptr = ptr;
	return rc;
}


/**
 * @brief Parse segment index box
 *
 * @param[in]  start                Start of box
 * @param[in]  size                 Size of box
 * @param[in]  segmentIndex         Segment index
 * @param[out] referenced_size      Referenced size
 * @param[out] referenced_duration  Referenced duration
 *
 * @retval true on success
 *
 * @note The SegmentBase indexRange attribute points to Segment Index Box location with segments and random access points.
 */
static bool ParseSegmentIndexBox( const char *start, size_t size, int segmentIndex, unsigned int *referenced_size, float *referenced_duration )
{
	const char **f = &start;
	unsigned int len = Read32(f);
	assert(len == size);
	unsigned int type = Read32(f);
	assert(type == 'sidx');
	unsigned int version = Read32(f);
	unsigned int reference_ID = Read32(f);
	unsigned int timescale = Read32(f);
	unsigned int earliest_presentation_time = Read32(f);
	unsigned int first_offset = Read32(f);
	unsigned int count = Read32(f);
	for (unsigned int i = 0; i < count; i++)
	{
		*referenced_size = Read32(f);
		*referenced_duration = Read32(f)/(float)timescale;
		unsigned int flags = Read32(f);
		if (i == segmentIndex) return true;
	}
	return false;
}


/**
 * @brief Replace matching token with given number
 *
 * @param[in] str       String in which operation to be performed
 * @param[in] from      Token
 * @param[in] toNumber  Number to replace token
 *
 * @retval position
 */
static int replace(std::string& str, const std::string& from, uint64_t toNumber )
{
	int rc = 0;
	size_t tokenLength = from.length();

	for (;;)
	{
		bool done = true;
		size_t pos = 0;
		for (;;)
		{
			pos = str.find('$', pos);
			if (pos == std::string::npos)
			{
				break;
			}
			size_t next = str.find('$', pos + 1);
			if (str.substr(pos + 1, tokenLength) == from)
			{
				size_t formatLen = next - pos - tokenLength - 1;
				char buf[256];
				if (formatLen > 0)
				{
					std::string format = str.substr(pos + tokenLength + 1, formatLen);
					sprintf(buf, format.c_str(), toNumber);
					tokenLength += formatLen;
				}
				else
				{
					sprintf(buf, "%" PRIu64 "", toNumber);
				}
				str.replace(pos, tokenLength + 2, buf);
				done = false;
				rc++;
				break;
			}
			pos = next + 1;
		}
		if (done) break;
	}

	return rc;
}


/**
 * @brief Replace matching token with given string
 *
 * @param[in] str      String in which operation to be performed
 * @param[in] from     Token
 * @param[in] toString String to replace token
 *
 * @retval position
 */
static int replace(std::string& str, const std::string& from, const std::string& toString )
{
	int rc = 0;
	size_t tokenLength = from.length();

	for (;;)
	{
		bool done = true;
		size_t pos = 0;
		for (;;)
		{
			pos = str.find('$', pos);
			if (pos == std::string::npos)
			{
				break;
			}
			size_t next = str.find('$', pos + 1);
			if (str.substr(pos + 1, tokenLength) == from)
			{
				str.replace(pos, tokenLength + 2, toString);
				done = false;
				rc++;
				break;
			}
			pos = next + 1;
		}

		if (done) break;
	}

	return rc;
}


/**
 * @brief Generates fragment URL from media information
 *
 * @param[out] fragmentUrl         Fragment URL
 * @param[in]  fragmentDescriptor  Descriptor
 * @param[in]  media               Media information string
 */
static void GetFragmentUrl( char fragmentUrl[MAX_URI_LENGTH], const FragmentDescriptor *fragmentDescriptor, std::string media)
{
	std::string constructedUri;
	if (fragmentDescriptor->baseUrls->size() > 0)
	{
		constructedUri = fragmentDescriptor->baseUrls->at(0)->GetUrl();
		if(gpGlobalConfig->dashIgnoreBaseURLIfSlash)
		{
			if (constructedUri == "/")
			{
				logprintf("%s:%d ignoring baseurl /\n", __FUNCTION__, __LINE__);
				constructedUri.clear();
			}
		}

		//Add '/' to BaseURL if not already available.
		if( constructedUri.compare(0, 7, "http://")==0 || constructedUri.compare(0, 8, "https://")==0 )
		{
			if( constructedUri.back() != '/' )
			{
				constructedUri += '/';
			}
		}
	}
	else
	{
		traceprintf("%s:%d BaseURL not available\n", __FUNCTION__, __LINE__);
	}
	constructedUri += media;

	replace(constructedUri, "Bandwidth", fragmentDescriptor->Bandwidth);
	replace(constructedUri, "RepresentationID", fragmentDescriptor->RepresentationID);
	replace(constructedUri, "Number", fragmentDescriptor->Number);
	replace(constructedUri, "Time", fragmentDescriptor->Time );

	aamp_ResolveURL(fragmentUrl, fragmentDescriptor->manifestUrl, constructedUri.c_str());
}

#ifdef AAMP_HARVEST_SUPPORT_ENABLED

#include <sys/stat.h>

/**
 * @brief Gets file path to havest
 *
 * @param[out] filePath             Path of file
 * @param[in]  fragmentDescriptor   Fragment descriptor
 * @param[in]  media                String containing media info
 */
static void GetFilePath(char filePath[MAX_URI_LENGTH], const FragmentDescriptor *fragmentDescriptor, std::string media)
{
	std::string constructedUri = HARVEST_BASE_PATH;
	constructedUri += media;
	replace(constructedUri, "Bandwidth", fragmentDescriptor->Bandwidth);
	replace(constructedUri, "RepresentationID", fragmentDescriptor->RepresentationID);
	replace(constructedUri, "Number", fragmentDescriptor->Number);
	replace(constructedUri, "Time", fragmentDescriptor->Time);
	strcpy(filePath, constructedUri.c_str());
}


/**
 * @brief Write file to storage
 *
 * @param[in]  fileName  Out file name
 * @param[in]  data      Buffer
 * @param[in]  len       Length of buffer
 */
static void WriteFile(char* fileName, const char* data, int len)
{
	struct stat st = { 0 };
	for (unsigned int i = 0; i < strlen(fileName); i++)
	{
		if (fileName[i] == '/')
		{
			fileName[i] = '\0';
			if (-1 == stat(fileName, &st))
			{
				mkdir(fileName, 0777);
			}
			fileName[i] = '/';
		}
	}
	FILE *fp = fopen(fileName, "wb");
	if (NULL == fp)
	{
		logprintf("File open failed. outfile = %s \n", fileName);
		return;
	}
	fwrite(data, len, 1, fp);
	fclose(fp);
}
#endif // AAMP_HARVEST_SUPPORT_ENABLED

/**
 * @brief Fetch and cache a fragment
 *
 * @param[in]  pMediaStreamContext      Track object pointer
 * @param[in]  media                    Media descriptor string
 * @param[in]  fragmentDuration         Duration of fragment in seconds
 * @param[in]  isInitializationSegment  True if fragment is init fragment
 * @param[in]  curlInstance             Curl instance to be used to fetch
 * @param[in]  discontinuity            true if fragment is discontinuous
 *
 * @retval true on fetch success
 */
bool PrivateStreamAbstractionMPD::FetchFragment(MediaStreamContext *pMediaStreamContext, std::string media, double fragmentDuration, bool isInitializationSegment, unsigned int curlInstance, bool discontinuity)
{ // given url, synchronously download and transmit associated fragment
	bool retval = true;
	char fragmentUrl[MAX_URI_LENGTH];
	GetFragmentUrl(fragmentUrl, &pMediaStreamContext->fragmentDescriptor, media);
	size_t len = 0;
	float position;
	if(isInitializationSegment)
	{
		if(!(pMediaStreamContext->initialization.empty()) && (0 == strcmp(pMediaStreamContext->initialization.c_str(),(const char*)fragmentUrl))&& !discontinuity)
		{
			AAMPLOG_TRACE("We have pushed the same initailization segment for %s skipping\n", mMediaTypeName[pMediaStreamContext->type]);
			return retval;
		}
		else
		{
			pMediaStreamContext->initialization = std::string(fragmentUrl);
		}
		position = mFirstPTS;
	}
	else
	{
		position = pMediaStreamContext->fragmentTime;
	}

	float duration = fragmentDuration;
	if(rate > AAMP_NORMAL_PLAY_RATE)
	{
		position = position/rate;
		AAMPLOG_INFO("PrivateStreamAbstractionMPD::%s:%d rate %f pMediaStreamContext->fragmentTime %f updated position %f\n",
				__FUNCTION__, __LINE__, rate, pMediaStreamContext->fragmentTime, position);
		duration = duration/rate * gpGlobalConfig->vodTrickplayFPS;
		//aamp->disContinuity();
	}
	if(!pMediaStreamContext->CacheFragment(fragmentUrl, curlInstance, position, duration, NULL, isInitializationSegment, discontinuity
#ifdef AAMP_HARVEST_SUPPORT_ENABLED
		, media
#endif
	))
	{
		logprintf("PrivateStreamAbstractionMPD::%s:%d failed. fragmentUrl %s fragmentTime %f\n", __FUNCTION__, __LINE__, fragmentUrl, pMediaStreamContext->fragmentTime);
		retval = false;
	}
	else
	{
		pMediaStreamContext->fragmentTime += fragmentDuration;
	}
	return retval;
}


/**
 * @brief Fetch and push next fragment
 *
 * @param[in] pMediaStreamContext  Track object
 * @param[in] curlInstance         Instance of curl to be used to fetch
 *
 * @retval true if push is done successfully
 */
bool PrivateStreamAbstractionMPD::PushNextFragment( struct MediaStreamContext *pMediaStreamContext, unsigned int curlInstance)
{
	ISegmentTemplate *segmentTemplate = pMediaStreamContext->adaptationSet->GetSegmentTemplate();
	bool retval=false;
	if (!segmentTemplate)
	{
		segmentTemplate = pMediaStreamContext->representation->GetSegmentTemplate();
	}

#ifdef DEBUG_TIMELINE
	logprintf("%s:%d Type[%d] timeLineIndex %d segmentTemplate %p fragmentRepeatCount %u\n", __FUNCTION__, __LINE__,pMediaStreamContext->type,
	        pMediaStreamContext->timeLineIndex, segmentTemplate, pMediaStreamContext->fragmentRepeatCount);
#endif
	if (segmentTemplate)
	{
		std::string media = segmentTemplate->Getmedia();
		const ISegmentTimeline *segmentTimeline = segmentTemplate->GetSegmentTimeline();
		if (segmentTimeline)
		{
			uint32_t timeScale = segmentTemplate->GetTimescale();
			std::vector<ITimeline *>&timelines = segmentTimeline->GetTimelines();
#ifdef DEBUG_TIMELINE
			logprintf("%s:%d Type[%d] timelineCnt=%d timeLineIndex:%d fragTime=%" PRIu64 " L=%" PRIu64 "\n", __FUNCTION__, __LINE__,
				pMediaStreamContext->type ,timelines.size(),pMediaStreamContext->timeLineIndex,pMediaStreamContext->fragmentDescriptor.Time,pMediaStreamContext->lastSegmentTime);
#endif
			if ((pMediaStreamContext->timeLineIndex >= timelines.size()) || (pMediaStreamContext->timeLineIndex < 0))
			{
				AAMPLOG_INFO("%s:%d Type[%d] EOS. timeLineIndex[%d] size [%lu]\n",__FUNCTION__, __LINE__,pMediaStreamContext->type, pMediaStreamContext->timeLineIndex, timelines.size());
				pMediaStreamContext->eos = true;
			}
			else
			{
				if (pMediaStreamContext->fragmentRepeatCount == 0)
				{
					ITimeline *timeline = timelines.at(pMediaStreamContext->timeLineIndex);
					uint64_t startTime = timeline->GetStartTime();
					if(startTime)
					{
						// After mpd refresh , Time will be 0. Need to traverse to the right fragment for playback
						if(0 == pMediaStreamContext->fragmentDescriptor.Time)
						{
							uint32_t duration =0;
							uint32_t repeatCount =0;
							int index=pMediaStreamContext->timeLineIndex;
							// This for loop is to go to the right index based on LastSegmentTime
							for(;index<timelines.size();index++)
							{
								timeline = timelines.at(index);
								startTime = timeline->GetStartTime();
								duration = timeline->GetDuration();
								repeatCount = timeline->GetRepeatCount();
								if(pMediaStreamContext->lastSegmentTime < (startTime+((repeatCount+1)*duration)))
								{
									break;
								}
								pMediaStreamContext->fragmentDescriptor.Number += (repeatCount+1);
							}// end of for

							/*
							*  Boundary check added to handle the edge case leading to crash,
							*  reported in DELIA-30316.
							*/
							if(index == timelines.size())
							{
								logprintf("%s:%d Type[%d] Boundary Condition !!! Index(%d) reached Max.Start=%" PRIu64 " Last=%" PRIu64 " \n",__FUNCTION__, __LINE__,
									pMediaStreamContext->type,index,startTime,pMediaStreamContext->lastSegmentTime);
								index--;
								startTime = pMediaStreamContext->lastSegmentTime;
								pMediaStreamContext->fragmentRepeatCount = repeatCount+1;
							}

#ifdef DEBUG_TIMELINE
							logprintf("%s:%d Type[%d] t=%" PRIu64 " L=%" PRIu64 " d=%d r=%d Index=%d Num=%" PRIu64 " FTime=%f\n",__FUNCTION__, __LINE__, pMediaStreamContext->type,
							startTime,pMediaStreamContext->lastSegmentTime, duration, repeatCount,index,
							pMediaStreamContext->fragmentDescriptor.Number,pMediaStreamContext->fragmentTime);
#endif
							pMediaStreamContext->timeLineIndex = index;
							// Now we reached the right row , need to traverse the repeat index to reach right node
							while(startTime < pMediaStreamContext->lastSegmentTime &&
								pMediaStreamContext->fragmentRepeatCount < repeatCount )
							{
								startTime += duration;
								pMediaStreamContext->fragmentDescriptor.Number++;
								pMediaStreamContext->fragmentRepeatCount++;
							}
#ifdef DEBUG_TIMELINE
							logprintf("%s:%d Type[%d] t=%" PRIu64 " L=%" PRIu64 " d=%d r=%d fragRep=%d Index=%d Num=%" PRIu64 " FTime=%f\n",__FUNCTION__, __LINE__, pMediaStreamContext->type,
							startTime,pMediaStreamContext->lastSegmentTime, duration, repeatCount,pMediaStreamContext->fragmentRepeatCount,pMediaStreamContext->timeLineIndex,
							pMediaStreamContext->fragmentDescriptor.Number,pMediaStreamContext->fragmentTime);
#endif
						}
					}// if starttime
					if(0 == pMediaStreamContext->timeLineIndex)
					{
						AAMPLOG_INFO("%s:%d Type[%d] update startTime to %" PRIu64 "\n", __FUNCTION__, __LINE__,pMediaStreamContext->type, startTime);
					}
					pMediaStreamContext->fragmentDescriptor.Time = startTime;
#ifdef DEBUG_TIMELINE
					logprintf("%s:%d Type[%d] Setting startTime to %" PRIu64 "\n", __FUNCTION__, __LINE__,pMediaStreamContext->type, startTime);
#endif
				}// if fragRepeat == 0

				ITimeline *timeline = timelines.at(pMediaStreamContext->timeLineIndex);
				uint32_t repeatCount = timeline->GetRepeatCount();
				uint32_t duration = timeline->GetDuration();
#ifdef DEBUG_TIMELINE
				logprintf("%s:%d Type[%d] t=%" PRIu64 " L=%" PRIu64 " d=%d r=%d fragrep=%d x=%d num=%lld\n",__FUNCTION__, __LINE__,
				pMediaStreamContext->type,pMediaStreamContext->fragmentDescriptor.Time,
				pMediaStreamContext->lastSegmentTime, duration, repeatCount,pMediaStreamContext->fragmentRepeatCount,
				pMediaStreamContext->timeLineIndex,pMediaStreamContext->fragmentDescriptor.Number);
#endif
				if ((pMediaStreamContext->fragmentDescriptor.Time > pMediaStreamContext->lastSegmentTime) || (0 == pMediaStreamContext->lastSegmentTime))
				{
					if(mIsFogTSB && pMediaStreamContext->mediaType == eMEDIATYPE_VIDEO)
					{
						long long bitrate = 0;
						std::map<string,string> rawAttributes =  timeline->GetRawAttributes();
						if(rawAttributes.find("b") == rawAttributes.end())
						{
							bitrate = pMediaStreamContext->fragmentDescriptor.Bandwidth;
						}
						else
						{
							string bitrateStr = rawAttributes["b"];
							bitrate = stoll(bitrateStr);
						}
						if(pMediaStreamContext->fragmentDescriptor.Bandwidth != bitrate || pMediaStreamContext->profileChanged)
						{
							pMediaStreamContext->fragmentDescriptor.Bandwidth = bitrate;
							pMediaStreamContext->profileChanged = true;
							mContext->profileIdxForBandwidthNotification = mBitrateIndexMap[bitrate];
							FetchAndInjectInitialization();
							return false; //Since we need to check WaitForFreeFragmentCache
						}
					}
#ifdef DEBUG_TIMELINE
					logprintf("%s:%d Type[%d] presenting %" PRIu64 " Number(%lld) Last=%" PRIu64 " Duration(%d) FTime(%f) \n",__FUNCTION__, __LINE__,
					pMediaStreamContext->type,pMediaStreamContext->fragmentDescriptor.Time,pMediaStreamContext->fragmentDescriptor.Number,pMediaStreamContext->lastSegmentTime,duration,pMediaStreamContext->fragmentTime);
#endif
					pMediaStreamContext->lastSegmentTime = pMediaStreamContext->fragmentDescriptor.Time;
					double fragmentDuration = (double)duration/timeScale;
					retval = FetchFragment( pMediaStreamContext, media, fragmentDuration, false, curlInstance);
					if(retval)
					{
						//logprintf("VOD/CDVR Line:%d fragmentDuration:%f target:%f SegTime%f rate:%f\n",__LINE__,fragmentDuration,pMediaStreamContext->targetDnldPosition,pMediaStreamContext->fragmentTime,rate);
						if(rate > AAMP_NORMAL_PLAY_RATE)
						{
							pMediaStreamContext->targetDnldPosition = pMediaStreamContext->fragmentTime;
						}
						else
						{
							pMediaStreamContext->targetDnldPosition += fragmentDuration;
						}
					}
					if(mContext->mCheckForRampdown && pMediaStreamContext->mediaType == eMEDIATYPE_VIDEO)
					{
						// DELIA-31780 - On audio fragment download failure (http500), rampdown was attempted .
						// rampdown is only needed for video fragments not for audio.
						// second issue : after rampdown lastSegmentTime was going into "0" . When this combined with mpd refresh immediately after rampdown ,
						// startTime is set to start of Period . This caused audio fragment download from "0" resulting in PTS mismatch and mute
						// Fix : Only do lastSegmentTime correction for video not for audio
						//	 lastSegmentTime to be corrected with duration of last segment attempted .
						if(pMediaStreamContext->lastSegmentTime)
							pMediaStreamContext->lastSegmentTime -= duration;
						return retval; /* Incase of fragment download fail, no need to increase the fragment number to download next fragment,
								 * instead check the same fragment in lower profile. */
					}
				}
				else if (rate < 0)
				{
#ifdef DEBUG_TIMELINE
					logprintf("%s:%d Type[%d] presenting %" PRIu64 "\n", __FUNCTION__, __LINE__,pMediaStreamContext->type,pMediaStreamContext->fragmentDescriptor.Time);
#endif
					pMediaStreamContext->lastSegmentTime = pMediaStreamContext->fragmentDescriptor.Time;
					double fragmentDuration = (double)duration/timeScale;
					retval = FetchFragment( pMediaStreamContext, media, fragmentDuration, false, curlInstance);
				}
				else if(pMediaStreamContext->mediaType == eMEDIATYPE_VIDEO &&
						((pMediaStreamContext->lastSegmentTime - pMediaStreamContext->fragmentDescriptor.Time) > TIMELINE_START_RESET_DIFF))
				{
					if(!mIsLive || aamp->IsVodOrCdvrAsset())
					{
						pMediaStreamContext->lastSegmentTime = pMediaStreamContext->fragmentDescriptor.Time - 1;
						return false;
					}
					logprintf("%s:%d Calling ScheduleRetune to handle start-time reset lastSegmentTime=%" PRIu64 " start-time=%" PRIu64 "\n", __FUNCTION__, __LINE__, pMediaStreamContext->lastSegmentTime, pMediaStreamContext->fragmentDescriptor.Time);
					aamp->ScheduleRetune(eDASH_ERROR_STARTTIME_RESET, pMediaStreamContext->mediaType);
				}
				else
				{
#ifdef DEBUG_TIMELINE
					logprintf("%s:%d Type[%d] Before skipping. fragmentDescriptor.Time %" PRIu64 " lastSegmentTime %" PRIu64 " Index=%d fragRep=%d,repMax=%d Number=%lld\n",__FUNCTION__, __LINE__,pMediaStreamContext->type,
						pMediaStreamContext->fragmentDescriptor.Time, pMediaStreamContext->lastSegmentTime,pMediaStreamContext->timeLineIndex,
						pMediaStreamContext->fragmentRepeatCount , repeatCount,pMediaStreamContext->fragmentDescriptor.Number);
#endif
					while(pMediaStreamContext->fragmentDescriptor.Time < pMediaStreamContext->lastSegmentTime &&
							pMediaStreamContext->fragmentRepeatCount < repeatCount )
						{
							if(rate > 0)
							{
								pMediaStreamContext->fragmentDescriptor.Time += duration;
								pMediaStreamContext->fragmentDescriptor.Number++;
								pMediaStreamContext->fragmentRepeatCount++;
							}
						}
#ifdef DEBUG_TIMELINE
					logprintf("%s:%d Type[%d] After skipping. fragmentDescriptor.Time %" PRIu64 " lastSegmentTime %" PRIu64 " Index=%d Number=%lld\n",__FUNCTION__, __LINE__,pMediaStreamContext->type,
							pMediaStreamContext->fragmentDescriptor.Time, pMediaStreamContext->lastSegmentTime,pMediaStreamContext->timeLineIndex,pMediaStreamContext->fragmentDescriptor.Number);
#endif
				}
				if(rate > 0)
				{
					pMediaStreamContext->fragmentDescriptor.Time += duration;
					pMediaStreamContext->fragmentDescriptor.Number++;
					pMediaStreamContext->fragmentRepeatCount++;
					if( pMediaStreamContext->fragmentRepeatCount > repeatCount)
					{
						pMediaStreamContext->fragmentRepeatCount = 0;
						pMediaStreamContext->timeLineIndex++;
					}
#ifdef DEBUG_TIMELINE
						logprintf("%s:%d Type[%d] After Incr. fragmentDescriptor.Time %" PRIu64 " lastSegmentTime %" PRIu64 " Index=%d fragRep=%d,repMax=%d Number=%lld\n",__FUNCTION__, __LINE__,pMediaStreamContext->type,
						pMediaStreamContext->fragmentDescriptor.Time, pMediaStreamContext->lastSegmentTime,pMediaStreamContext->timeLineIndex,
						pMediaStreamContext->fragmentRepeatCount , repeatCount,pMediaStreamContext->fragmentDescriptor.Number);
#endif
				}
				else
				{
					pMediaStreamContext->fragmentDescriptor.Time -= duration;
					pMediaStreamContext->fragmentDescriptor.Number--;
					pMediaStreamContext->fragmentRepeatCount--;
					if( pMediaStreamContext->fragmentRepeatCount < 0)
					{
						pMediaStreamContext->timeLineIndex--;
						if(pMediaStreamContext->timeLineIndex >= 0)
						{
							pMediaStreamContext->fragmentRepeatCount = timelines.at(pMediaStreamContext->timeLineIndex)->GetRepeatCount();
						}
					}
				}
			}
		}
		else
		{
#ifdef DEBUG_TIMELINE
			logprintf("%s:%d segmentTimeline not available\n", __FUNCTION__, __LINE__);
#endif
			string startTimestr = mpd->GetAvailabilityStarttime();
			std::tm time = { 0 };
			strptime(startTimestr.c_str(), "%Y-%m-%dT%H:%M:%SZ", &time);
			double availabilityStartTime = (double)mktime(&time);
			double currentTimeSeconds = aamp_GetCurrentTimeMS() / 1000;
			double fragmentDuration = ((double)segmentTemplate->GetDuration()) / segmentTemplate->GetTimescale();
			if (!fragmentDuration)
			{
				fragmentDuration = 2; // hack
			}
			if (0 == pMediaStreamContext->lastSegmentNumber)
			{
				if (mIsLive)
				{
					double liveTime = currentTimeSeconds - gpGlobalConfig->liveOffset;
					pMediaStreamContext->lastSegmentNumber = (long long)((liveTime - availabilityStartTime - mPeriodStartTime) / fragmentDuration) + segmentTemplate->GetStartNumber();
					pMediaStreamContext->fragmentDescriptor.Time = liveTime;
					AAMPLOG_INFO("%s %d Printing fragmentDescriptor.Number %" PRIu64 " Time=%" PRIu64 "  \n", __FUNCTION__, __LINE__, pMediaStreamContext->lastSegmentNumber, pMediaStreamContext->fragmentDescriptor.Time);
				}
				else
				{
					if (rate < 0)
					{
						pMediaStreamContext->fragmentDescriptor.Time = mPeriodEndTime;
					}
					else
					{
						pMediaStreamContext->fragmentDescriptor.Time = mPeriodStartTime;
					}
				}
			}

			// Recalculate the fragmentDescriptor.Time after periodic manifest updates
			if (mIsLive && 0 == pMediaStreamContext->fragmentDescriptor.Time)
			{
				pMediaStreamContext->fragmentDescriptor.Time = availabilityStartTime + mPeriodStartTime + ((pMediaStreamContext->lastSegmentNumber - segmentTemplate->GetStartNumber()) * fragmentDuration);
			}

			/**
			 *Find out if we reached end/beginning of period.
			 *First block in this 'if' is for VOD, where boundaries are 0 and PeriodEndTime
			 *Second block is for LIVE, where boundaries are
                         * (availabilityStartTime + mPeriodStartTime) and currentTime
			 */
			if ((!mIsLive && ((mPeriodEndTime && (pMediaStreamContext->fragmentDescriptor.Time > mPeriodEndTime))
							|| (rate < 0 && pMediaStreamContext->fragmentDescriptor.Time < 0)))
					|| (mIsLive && ((pMediaStreamContext->fragmentDescriptor.Time >= currentTimeSeconds)
							|| (pMediaStreamContext->fragmentDescriptor.Time < (availabilityStartTime + mPeriodStartTime)))))
			{
				AAMPLOG_INFO("%s:%d EOS. fragmentDescriptor.Time=%" PRIu64 " mPeriodEndTime=%f FTime=%f\n",__FUNCTION__, __LINE__, pMediaStreamContext->fragmentDescriptor.Time, mPeriodEndTime,pMediaStreamContext->fragmentTime);
				pMediaStreamContext->eos = true;
			}
			else
			{
				if (mIsLive)
				{
					pMediaStreamContext->fragmentDescriptor.Number = pMediaStreamContext->lastSegmentNumber;
				}
				FetchFragment(pMediaStreamContext, media, fragmentDuration, false, curlInstance);
				if (mContext->mCheckForRampdown)
				{
					/* NOTE : This case needs to be validated with the segmentTimeline not available stream */
					return retval;
				}

				if (rate > 0)
				{
					pMediaStreamContext->fragmentDescriptor.Number++;
					pMediaStreamContext->fragmentDescriptor.Time += fragmentDuration;
				}
				else
				{
					pMediaStreamContext->fragmentDescriptor.Number--;
					pMediaStreamContext->fragmentDescriptor.Time -= fragmentDuration;
				}
				pMediaStreamContext->lastSegmentNumber = pMediaStreamContext->fragmentDescriptor.Number;
			}
		}
	}
	else
	{
		ISegmentBase *segmentBase = pMediaStreamContext->representation->GetSegmentBase();
		if (segmentBase)
		{ // single-segment
			char fragmentUrl[MAX_URI_LENGTH];
			GetFragmentUrl(fragmentUrl, &pMediaStreamContext->fragmentDescriptor, "");
			if (!pMediaStreamContext->index_ptr)
			{ // lazily load index
				std::string range = segmentBase->GetIndexRange();
				int start;
				sscanf(range.c_str(), "%d-%d", &start, &pMediaStreamContext->fragmentOffset);
				ProfilerBucketType bucketType = aamp->GetProfilerBucketForMedia(pMediaStreamContext->mediaType, true);
				MediaType actualType = (MediaType)(eMEDIATYPE_INIT_VIDEO+pMediaStreamContext->mediaType);
				pMediaStreamContext->index_ptr = aamp->LoadFragment(bucketType, fragmentUrl, &pMediaStreamContext->index_len, curlInstance, range.c_str(),actualType);

				pMediaStreamContext->fragmentOffset++; // first byte following packed index

				if (pMediaStreamContext->fragmentIndex != 0)
				{
					unsigned int referenced_size;
					float fragmentDuration;
					AAMPLOG_INFO("%s:%d current fragmentIndex = %d\n", __FUNCTION__, __LINE__, pMediaStreamContext->fragmentIndex);
					//Find the offset of previous fragment in new representation
					for (int i = 0; i < pMediaStreamContext->fragmentIndex; i++)
					{
						if (ParseSegmentIndexBox(pMediaStreamContext->index_ptr, pMediaStreamContext->index_len, i,
							&referenced_size, &fragmentDuration))
						{
							pMediaStreamContext->fragmentOffset += referenced_size;
						}
					}
				}
			}
			if (pMediaStreamContext->index_ptr)
			{
				unsigned int referenced_size;
				float fragmentDuration;
				if (ParseSegmentIndexBox(pMediaStreamContext->index_ptr, pMediaStreamContext->index_len, pMediaStreamContext->fragmentIndex++, &referenced_size, &fragmentDuration))
				{
					char range[128];
					sprintf(range, "%d-%d", pMediaStreamContext->fragmentOffset, pMediaStreamContext->fragmentOffset + referenced_size - 1);
					AAMPLOG_INFO("%s:%d %s [%s]\n", __FUNCTION__, __LINE__,mMediaTypeName[pMediaStreamContext->mediaType], range);
					if(!pMediaStreamContext->CacheFragment(fragmentUrl, curlInstance, pMediaStreamContext->fragmentTime, 0.0, range ))
					{
						logprintf("PrivateStreamAbstractionMPD::%s:%d failed. fragmentUrl %s fragmentTime %f\n", __FUNCTION__, __LINE__, fragmentUrl, pMediaStreamContext->fragmentTime);
					}
					pMediaStreamContext->fragmentTime += fragmentDuration;
					pMediaStreamContext->fragmentOffset += referenced_size;
				}
				else
				{ // done with index
					aamp_Free(&pMediaStreamContext->index_ptr);
					pMediaStreamContext->eos = true;
				}
			}
			else
			{
				pMediaStreamContext->eos = true;
			}
		}
		else
		{
			ISegmentList *segmentList = pMediaStreamContext->representation->GetSegmentList();
			if (segmentList)
			{
				const std::vector<ISegmentURL*>segmentURLs = segmentList->GetSegmentURLs();
				if (pMediaStreamContext->fragmentIndex >= segmentURLs.size() ||  pMediaStreamContext->fragmentIndex < 0)
				{
					pMediaStreamContext->eos = true;
				}
				else if(!segmentURLs.empty())
				{
					ISegmentURL *segmentURL = segmentURLs.at(pMediaStreamContext->fragmentIndex);

					std::map<string,string> rawAttributes = segmentList->GetRawAttributes();
					if(rawAttributes.find("customlist") == rawAttributes.end()) //"CheckForFogSegmentList")
					{
						char fragmentUrl[MAX_URI_LENGTH];
						GetFragmentUrl(fragmentUrl, &pMediaStreamContext->fragmentDescriptor,  segmentURL->GetMediaURI());
						AAMPLOG_INFO("%s [%s]\n", mMediaTypeName[pMediaStreamContext->mediaType], segmentURL->GetMediaRange().c_str());
						if(!pMediaStreamContext->CacheFragment(fragmentUrl, curlInstance, pMediaStreamContext->fragmentTime, 0.0, segmentURL->GetMediaRange().c_str() ))
						{
							logprintf("PrivateStreamAbstractionMPD::%s:%d failed. fragmentUrl %s fragmentTime %f\n", __FUNCTION__, __LINE__, fragmentUrl, pMediaStreamContext->fragmentTime);
						}
					}
					else //We are procesing the custom segment list provided by Fog for DASH TSB
					{
						uint32_t timescale = segmentList->GetTimescale();
						string durationStr = segmentURL->GetRawAttributes().at("d");
						string startTimestr = segmentURL->GetRawAttributes().at("s");
						long long duration = stoll(durationStr);
						long long startTime = stoll(startTimestr);
						if(startTime > pMediaStreamContext->lastSegmentTime || 0 == pMediaStreamContext->lastSegmentTime || rate < 0 )
						{
							/*
								Added to inject appropriate initialization header in
								the case of fog custom mpd
							*/
							if(eMEDIATYPE_VIDEO == pMediaStreamContext->mediaType)
							{
								long long bitrate = 0;
								std::map<string,string> rawAttributes =  segmentURL->GetRawAttributes();
								if(rawAttributes.find("bitrate") == rawAttributes.end()){
									bitrate = pMediaStreamContext->fragmentDescriptor.Bandwidth;
								}else{
									string bitrateStr = rawAttributes["bitrate"];
									bitrate = stoll(bitrateStr);
								}
								if(pMediaStreamContext->fragmentDescriptor.Bandwidth != bitrate || pMediaStreamContext->profileChanged)
								{
									pMediaStreamContext->fragmentDescriptor.Bandwidth = bitrate;
									pMediaStreamContext->profileChanged = true;
									mContext->profileIdxForBandwidthNotification = mBitrateIndexMap[bitrate];
									FetchAndInjectInitialization();
									return false; //Since we need to check WaitForFreeFragmentCache
								}
							}
							double fragmentDuration = (double)duration / timescale;
							pMediaStreamContext->lastSegmentTime = startTime;
							retval = FetchFragment(pMediaStreamContext, segmentURL->GetMediaURI(), fragmentDuration, false, curlInstance);
							if(retval && rate > 0)
							{
								//logprintf("Live update Line:%d fragmentDuration:%f target:%f FragTime%f rate:%f\n",__LINE__,fragmentDuration,pMediaStreamContext->targetDnldPosition,pMediaStreamContext->fragmentTime,rate);
								if(rate > AAMP_NORMAL_PLAY_RATE)
								{
									pMediaStreamContext->targetDnldPosition = pMediaStreamContext->fragmentTime;
								}
								else
								{
									pMediaStreamContext->targetDnldPosition += fragmentDuration;
								}
							}
							if(mContext->mCheckForRampdown)
							{
								/* This case needs to be validated with the segmentList available stream */

								return retval;
							}
						}
						else if(pMediaStreamContext->mediaType == eMEDIATYPE_VIDEO && duration > 0 && ((pMediaStreamContext->lastSegmentTime - startTime) > TIMELINE_START_RESET_DIFF))
						{
							logprintf("%s:%d START-TIME RESET in TSB period, lastSegmentTime=%" PRIu64 " start-time=%lld duration=%lld\n", __FUNCTION__, __LINE__, pMediaStreamContext->lastSegmentTime, startTime, duration);
							pMediaStreamContext->lastSegmentTime = startTime - 1;
							return retval;
						}
						else
						{
							int index = pMediaStreamContext->fragmentIndex + 1;
							int listSize = segmentURLs.size();

							/*Added this block to reduce the skip overhead for custom mpd after
							 *MPD refresh
							*/
							int nextIndex = ((pMediaStreamContext->lastSegmentTime - startTime) / duration) - 5;
							while(nextIndex > 0 && nextIndex < listSize)
							{
								segmentURL = segmentURLs.at(nextIndex);
								string startTimestr = segmentURL->GetRawAttributes().at("s");
								startTime = stoll(startTimestr);
								if(startTime > pMediaStreamContext->lastSegmentTime)
								{
									nextIndex -= 5;
									continue;
								}
								else
								{
									index = nextIndex;
									break;
								}
							}

							while(startTime < pMediaStreamContext->lastSegmentTime && index < listSize)
							{
								segmentURL = segmentURLs.at(index);
								string startTimestr = segmentURL->GetRawAttributes().at("s");
								startTime = stoll(startTimestr);
								index++;
							}
							pMediaStreamContext->fragmentIndex = index - 1;
							AAMPLOG_TRACE("%s:%d PushNextFragment Exit : startTime %lld lastSegmentTime %lld index = %d\n", __FUNCTION__, __LINE__, startTime, pMediaStreamContext->lastSegmentTime, pMediaStreamContext->fragmentIndex);
						}
					}
					if(rate > 0)
					{
						pMediaStreamContext->fragmentIndex++;
					}
					else
					{
						pMediaStreamContext->fragmentIndex--;
					}
				}
				else
				{
					logprintf("PrivateStreamAbstractionMPD::%s:%d SegmentUrl is empty\n", __FUNCTION__, __LINE__);
				}
			}
			else
			{
				aamp_Error("not-yet-supported mpd format");
			}
		}
	}
	if ((mEndPosition > 0) && pMediaStreamContext->fragmentTime >= mEndPosition)
	{
		pMediaStreamContext->endTimeReached = true;
	}
	return retval;
}


/**
 * @brief Seek current period by a given time
 *
 * @param[in] seekPositionSeconds seek positon in seconds
 */
void PrivateStreamAbstractionMPD::SeekInPeriod( double seekPositionSeconds)
{
	for (int i = 0; i < mNumberOfTracks; i++)
	{
		SkipFragments(mMediaStreamContext[i], seekPositionSeconds, true);
	}
}



/**
 * @brief Skip to end of track
 *
 * @param[in] pMediaStreamContext Track object pointer
 */
void PrivateStreamAbstractionMPD::SkipToEnd( MediaStreamContext *pMediaStreamContext)
{
	ISegmentTemplate *segmentTemplate = pMediaStreamContext->adaptationSet->GetSegmentTemplate();
	if (!segmentTemplate)
	{
		segmentTemplate = pMediaStreamContext->representation->GetSegmentTemplate();
	}
	if (segmentTemplate)
	{
		const ISegmentTimeline *segmentTimeline = segmentTemplate->GetSegmentTimeline();
		if (segmentTimeline)
		{
			std::vector<ITimeline *>&timelines = segmentTimeline->GetTimelines();
			uint32_t repeatCount = 0;
			for(int i = 0; i < timelines.size(); i++)
			{
				ITimeline *timeline = timelines.at(i);
				repeatCount += (timeline->GetRepeatCount() + 1);
			}
			pMediaStreamContext->fragmentDescriptor.Number = pMediaStreamContext->fragmentDescriptor.Number + repeatCount - 1;
			pMediaStreamContext->timeLineIndex = timelines.size() - 1;
			pMediaStreamContext->fragmentRepeatCount = timelines.at(pMediaStreamContext->timeLineIndex)->GetRepeatCount();
		}
		else
		{
			double segmentDuration = ((double)segmentTemplate->GetDuration())/segmentTemplate->GetTimescale();
			uint64_t startTime = mPeriodStartTime;
			int number = 0;
			while(startTime < mPeriodEndTime)
			{
				startTime += segmentDuration;
				number++;
			}
			pMediaStreamContext->fragmentDescriptor.Number = pMediaStreamContext->fragmentDescriptor.Number + number - 1;
		}
	}
	else
	{
		ISegmentList *segmentList = pMediaStreamContext->representation->GetSegmentList();
		if (segmentList)
		{
			const std::vector<ISegmentURL*> segmentURLs = segmentList->GetSegmentURLs();
			pMediaStreamContext->fragmentIndex = segmentURLs.size() - 1;
		}
		else
		{
			aamp_Error("not-yet-supported mpd format");
		}
	}
}


/**
 * @brief Skip fragments by given time
 *
 * @param[in] pMediaStreamContext Media track object
 * @param[in] skipTime            Time to skip in seconds
 * @param[in] updateFirstPTS      True to update first pts state variable
 *
 * @retval
 */
double PrivateStreamAbstractionMPD::SkipFragments( MediaStreamContext *pMediaStreamContext, double skipTime, bool updateFirstPTS)
{
	ISegmentTemplate *segmentTemplate = pMediaStreamContext->adaptationSet->GetSegmentTemplate();
	if (!segmentTemplate)
	{
		segmentTemplate = pMediaStreamContext->representation->GetSegmentTemplate();
	}
	if (segmentTemplate)
	{
		 AAMPLOG_INFO("%s:%d Enter : Type[%d] timeLineIndex %d fragmentRepeatCount %d skipTime %f\n", __FUNCTION__, __LINE__,pMediaStreamContext->type,
                                pMediaStreamContext->timeLineIndex, pMediaStreamContext->fragmentRepeatCount, skipTime);

		std::string media = segmentTemplate->Getmedia();
		const ISegmentTimeline *segmentTimeline = segmentTemplate->GetSegmentTimeline();
		do
		{
			if (segmentTimeline)
			{
				uint32_t timeScale = segmentTemplate->GetTimescale();
				std::vector<ITimeline *>&timelines = segmentTimeline->GetTimelines();
				if (pMediaStreamContext->timeLineIndex >= timelines.size())
				{
					AAMPLOG_INFO("%s:%d Type[%d] EOS. timeLineIndex[%d] size [%lu]\n",__FUNCTION__,__LINE__,pMediaStreamContext->type, pMediaStreamContext->timeLineIndex, timelines.size());
					pMediaStreamContext->eos = true;
					break;
				}
				else
				{
					ITimeline *timeline = timelines.at(pMediaStreamContext->timeLineIndex);
					uint32_t repeatCount = timeline->GetRepeatCount();
					if (pMediaStreamContext->fragmentRepeatCount == 0)
					{
						uint64_t startTime = timeline->GetStartTime();
						if(startTime)
						{
							pMediaStreamContext->fragmentDescriptor.Time = startTime;
						}
					}
					uint32_t duration = timeline->GetDuration();
					double fragmentDuration = ((double)duration)/timeScale;
					double nextPTS = (double)(pMediaStreamContext->fragmentDescriptor.Time + duration)/timeScale;
					double firstPTS = (double)pMediaStreamContext->fragmentDescriptor.Time/timeScale;
					bool skipFlag = true;
					if ((pMediaStreamContext->type == eTRACK_AUDIO) && (nextPTS>mFirstPTS))
					{
						if ( ((nextPTS - mFirstPTS) >= ((fragmentDuration)/2.0)) &&
                             ((nextPTS - mFirstPTS) <= ((fragmentDuration * 3.0)/2.0)))
							skipFlag = false;
						AAMPLOG_INFO("%s:%d [%s] firstPTS %f, nextPTS %f, mFirstPTS %f skipFlag %d\n", __FUNCTION__, __LINE__, pMediaStreamContext->name, firstPTS, nextPTS, mFirstPTS, skipFlag);
					}
					if (skipTime >= fragmentDuration && skipFlag)
					{
						skipTime -= fragmentDuration;
						pMediaStreamContext->fragmentTime += fragmentDuration;
						pMediaStreamContext->fragmentDescriptor.Time += duration;
						pMediaStreamContext->fragmentDescriptor.Number++;
						pMediaStreamContext->fragmentRepeatCount++;
						if( pMediaStreamContext->fragmentRepeatCount > repeatCount)
						{
							pMediaStreamContext->fragmentRepeatCount= 0;
							pMediaStreamContext->timeLineIndex++;
						}
					}
					else if (-(skipTime) >= fragmentDuration)
					{
						skipTime += fragmentDuration;
						pMediaStreamContext->fragmentTime -= fragmentDuration;
						pMediaStreamContext->fragmentDescriptor.Time -= duration;
						pMediaStreamContext->fragmentDescriptor.Number--;
						pMediaStreamContext->fragmentRepeatCount--;
						if( pMediaStreamContext->fragmentRepeatCount < 0)
						{
							pMediaStreamContext->timeLineIndex--;
							if(pMediaStreamContext->timeLineIndex >= 0)
							{
								pMediaStreamContext->fragmentRepeatCount = timelines.at(pMediaStreamContext->timeLineIndex)->GetRepeatCount();
							}
						}
					}
					else
					{
						if (updateFirstPTS)
						{
							AAMPLOG_INFO("%s:%d [%s] newPTS %f, nextPTS %f \n", __FUNCTION__, __LINE__, pMediaStreamContext->name, firstPTS, nextPTS);
							/*Keep the lower PTS */
							if ( ((mFirstPTS == 0) || (firstPTS < mFirstPTS)) && (pMediaStreamContext->type == eTRACK_VIDEO))
							{
								AAMPLOG_INFO("%s:%d [%s] mFirstPTS %f -> %f \n", __FUNCTION__, __LINE__, pMediaStreamContext->name, mFirstPTS, firstPTS);
								mFirstPTS = firstPTS; 
								AAMPLOG_INFO("%s:%d [%s] mFirstPTS %f \n", __FUNCTION__, __LINE__, pMediaStreamContext->name, mFirstPTS);
							}
						}
						skipTime = 0;
						break;
					}
				}
			}
			else
			{
				if(0 == pMediaStreamContext->fragmentDescriptor.Time)
				{
					if(rate < 0)
					{
						pMediaStreamContext->fragmentDescriptor.Time = mPeriodEndTime;
					}
					else
					{
						pMediaStreamContext->fragmentDescriptor.Time = mPeriodStartTime;
					}
				}
				if(pMediaStreamContext->fragmentDescriptor.Time > mPeriodEndTime || (rate < 0 && pMediaStreamContext->fragmentDescriptor.Time <= 0))
				{
					AAMPLOG_INFO("%s:%d Type[%d] EOS. fragmentDescriptor.Time=%" PRIu64 " \n",__FUNCTION__,__LINE__,pMediaStreamContext->type, pMediaStreamContext->fragmentDescriptor.Time);
					pMediaStreamContext->eos = true;
					break;
				}
				else
				{
					double segmentDuration = ((double)segmentTemplate->GetDuration())/segmentTemplate->GetTimescale();
					if(!segmentDuration)
					{
						segmentDuration = 2;
					}

					if (skipTime >= segmentDuration)
					{
						pMediaStreamContext->fragmentDescriptor.Number++;
						skipTime -= segmentDuration;
					}
					else if (-(skipTime) >= segmentDuration)
					{
						pMediaStreamContext->fragmentDescriptor.Number--;
						skipTime += segmentDuration;
					}
					else
					{
						break;
					}
				}
			}
		}while(skipTime != 0);
		AAMPLOG_INFO("%s:%d Exit :Type[%d] timeLineIndex %d fragmentRepeatCount %d fragmentDescriptor.Number %" PRIu64 " fragmentTime %f\n", __FUNCTION__, __LINE__,pMediaStreamContext->type,
				pMediaStreamContext->timeLineIndex, pMediaStreamContext->fragmentRepeatCount, pMediaStreamContext->fragmentDescriptor.Number, pMediaStreamContext->fragmentTime);
	}
	else
	{
		ISegmentList *segmentList = pMediaStreamContext->representation->GetSegmentList();
		if (segmentList)
		{
			AAMPLOG_INFO("%s:%d Enter : fragmentIndex %d skipTime %f\n", __FUNCTION__, __LINE__,
					pMediaStreamContext->fragmentIndex, skipTime);
			const std::vector<ISegmentURL*> segmentURLs = segmentList->GetSegmentURLs();
			double segmentDuration = 0;
			if(!segmentURLs.empty())
			{
				std::map<string,string> rawAttributes = segmentList->GetRawAttributes();
				uint32_t timescale = segmentList->GetTimescale();
				bool isFogTsb = !(rawAttributes.find("customlist") == rawAttributes.end());
				if(!isFogTsb)
				{
					segmentDuration = segmentList->GetDuration() / timescale;
				}
				else if(pMediaStreamContext->type == eTRACK_AUDIO)
				{
					MediaStreamContext *videoContext = mMediaStreamContext[eMEDIATYPE_VIDEO];
					const std::vector<ISegmentURL*> vidSegmentURLs = videoContext->representation->GetSegmentList()->GetSegmentURLs();
					if(!vidSegmentURLs.empty())
					{
						string videoStartStr = vidSegmentURLs.at(0)->GetRawAttributes().at("s");
						string audioStartStr = segmentURLs.at(0)->GetRawAttributes().at("s");
						long long videoStart = stoll(videoStartStr);
						long long audioStart = stoll(audioStartStr);
						long long diff = audioStart - videoStart;
						logprintf("Printing diff value for adjusting %lld\n",diff);
						if(diff > 0)
						{
							double diffSeconds = double(diff) / timescale;
							skipTime -= diffSeconds;
						}
					}
					else
					{
						logprintf("PrivateStreamAbstractionMPD::%s:%d Video SegmentUrl is empty\n", __FUNCTION__, __LINE__);
					}
				}

				while (skipTime != 0)
				{

					if ((pMediaStreamContext->fragmentIndex >= segmentURLs.size()) || (pMediaStreamContext->fragmentIndex < 0))
					{
						pMediaStreamContext->eos = true;
						break;
					}
					else
					{
						//Calculate the individual segment duration for fog tsb
						if(isFogTsb)
						{
							ISegmentURL* segmentURL = segmentURLs.at(pMediaStreamContext->fragmentIndex);
							string durationStr = segmentURL->GetRawAttributes().at("d");
							long long duration = stoll(durationStr);
							segmentDuration = (double) duration / timescale;
						}
						if (skipTime >= segmentDuration)
						{
							pMediaStreamContext->fragmentIndex++;
							skipTime -= segmentDuration;
							pMediaStreamContext->fragmentTime += segmentDuration;
						}
						else if (-(skipTime) >= segmentDuration)
						{
							pMediaStreamContext->fragmentIndex--;
							skipTime += segmentDuration;
							pMediaStreamContext->fragmentTime -= segmentDuration;
						}
						else
						{
							skipTime = 0;
							break;
						}
					}
				}
			}
			else
			{
				logprintf("PrivateStreamAbstractionMPD::%s:%d SegmentUrl is empty\n", __FUNCTION__, __LINE__);
			}

			AAMPLOG_INFO("%s:%d Exit : fragmentIndex %d segmentDuration %f\n", __FUNCTION__, __LINE__,
					pMediaStreamContext->fragmentIndex, segmentDuration);
		}
		else
		{
			ISegmentBase *segmentBase = pMediaStreamContext->representation->GetSegmentBase();
			if(segmentBase)
			{
				skipTime=0;
			}
			else
			{
				aamp_Error("not-yet-supported mpd format");
			}
		}
	}
	return skipTime;
}


/**
 * @brief Add attriblutes to xml node
 *
 * @param[in] reader xmlTextReaderPtr
 * @param[in] node   xml Node
 */
static void AddAttributesToNode(xmlTextReaderPtr *reader, Node *node)
{
	if (xmlTextReaderHasAttributes(*reader))
	{
		while (xmlTextReaderMoveToNextAttribute(*reader))
		{
			std::string key = (const char *)xmlTextReaderConstName(*reader);
			std::string value = (const char *)xmlTextReaderConstValue(*reader);
			node->AddAttribute(key, value);
		}
	}
}


/**
 * @brief Get xml node form reader
 *
 * @param[in] reader Pointer to reader object
 * @param[in] url    manifest url
 *
 * @retval xml node
 */
static Node* ProcessNode(xmlTextReaderPtr *reader, char *url)
{
	int type = xmlTextReaderNodeType(*reader);

	if (type != WhiteSpace && type != Text)
	{
		while (type == Comment || type == WhiteSpace)
		{
			xmlTextReaderRead(*reader);
			type = xmlTextReaderNodeType(*reader);
		}

		Node *node = new Node();
		node->SetType(type);
		node->SetMPDPath(Path::GetDirectoryPath(url));

		if (xmlTextReaderConstName(*reader) == NULL)
		{
			delete node;
			return NULL;
		}

		std::string name = (const char *)xmlTextReaderConstName(*reader);
		int         isEmpty = xmlTextReaderIsEmptyElement(*reader);

		node->SetName(name);

		AddAttributesToNode(reader, node);

		if (isEmpty)
			return node;

		Node    *subnode = NULL;
		int     ret = xmlTextReaderRead(*reader);

		while (ret == 1)
		{
			if (!strcmp(name.c_str(), (const char *)xmlTextReaderConstName(*reader)))
			{
				return node;
			}

			subnode = ProcessNode(reader, url);

			if (subnode != NULL)
				node->AddSubNode(subnode);

			ret = xmlTextReaderRead(*reader);
		}

		return node;
	}
	else if (type == Text)
	{
		xmlChar * text = xmlTextReaderReadString(*reader);

		if (text != NULL)
		{
			Node *node = new Node();
			node->SetType(type);
			node->SetText((const char*)text);
			xmlFree(text);
			return node;
		}
	}
	return NULL;
}


/**
 *   @brief  Initialize a newly created object.
 *
 *   @param[in]  tuneType  To set type of object.
 *
 *   @retval true on success
 *   @retval false on failure
 *
 *   @note   To be implemented by sub classes
 */
AAMPStatusType StreamAbstractionAAMP_MPD::Init(TuneType tuneType)
{
	return mPriv->Init(tuneType);
}


/**
 * @brief Parse duration from ISO8601 string
 *
 * @param[in]  ptr         ISO8601 string
 * @param[out] durationMs  Duration in milliseconds
 */
static void ParseISO8601Duration(const char *ptr, uint64_t &durationMs)
{
	durationMs = 0;
	int hour = 0;
	int minute = 0;
	float seconds = 0;
	if (ptr[0] == 'P' && ptr[1] == 'T')
	{
		ptr += 2;
		const char* temp = strchr(ptr, 'H');
		if (temp)
		{
			sscanf(ptr, "%dH", &hour);
			ptr = temp + 1;
		}
		temp = strchr(ptr, 'M');
		if (temp)
		{
			sscanf(ptr, "%dM", &minute);
			ptr = temp + 1;
		}
		temp = strchr(ptr, 'S');
		if (temp)
		{
			sscanf(ptr, "%fS", &seconds);
			ptr = temp + 1;
		}
	}
	else
	{
		logprintf("%s:%d - Invalid input %s\n", __FUNCTION__, __LINE__, ptr);
	}
	durationMs = (float(((hour * 60) + minute) * 60 + seconds)) * 1000;
}


/**
 * @brief Parse XML NS
 *
 * @param[in]  fullName  Full name of node
 * @param[out] ns        Namespace
 * @param[out] name      Name after 
 */
static void ParseXmlNS(const std::string& fullName, std::string& ns, std::string& name)
{
	size_t found = fullName.find(':');
	if (found != std::string::npos)
	{
		ns = fullName.substr(0, found);
		name = fullName.substr(found+1);
	}
	else
	{
		ns = "";
		name = fullName;
	}
}

#ifdef AAMP_MPD_DRM

/**
 * @brief Create DRM Session
 *
 * @param[in] arg DrmSessionParams object pointer
 */
void *CreateDRMSession(void *arg)
{
	if(aamp_pthread_setname(pthread_self(), "aampDRM"))
	{
		logprintf("%s:%d: aamp_pthread_setname failed\n", __FUNCTION__, __LINE__);
	}
	struct DrmSessionParams* sessionParams = (struct DrmSessionParams*)arg;
	AampDRMSessionManager* sessionManger = new AampDRMSessionManager();
	sessionParams->aamp->profiler.ProfileBegin(PROFILE_BUCKET_LA_TOTAL);
	AAMPEvent e;
	e.type = AAMP_EVENT_DRM_METADATA;
	e.data.dash_drmmetadata.failure = AAMP_TUNE_FAILURE_UNKNOWN;
	e.data.dash_drmmetadata.responseCode = 0;
	unsigned char * data = sessionParams->initData;
	int dataLength = sessionParams->initDataLen;

	unsigned char *contentMetadata = sessionParams->contentMetadata;
	AampDrmSession *drmSession = NULL;
	const char * systemId = WIDEVINE_SYSTEM_ID;
	if (sessionParams->isWidevine)
	{
		logprintf("Found Widevine encryption from manifest\n");
	}
	else
	{
		logprintf("Found Playready encryption from manifest\n");
		systemId = PLAYREADY_SYSTEM_ID;
	}
	sessionParams->aamp->mStreamSink->QueueProtectionEvent(systemId, data, dataLength);
	//Hao Li: review changes for Widevine, contentMetadata is freed inside the following calls
	drmSession = sessionManger->createDrmSession(systemId, data, dataLength, sessionParams->stream_type,
					contentMetadata, sessionParams->aamp, &e);
	if(NULL == drmSession)
	{
		AAMPTuneFailure failure = e.data.dash_drmmetadata.failure;
		bool isRetryEnabled =      (failure != AAMP_TUNE_AUTHORISATION_FAILURE)
		                        && (failure != AAMP_TUNE_LICENCE_REQUEST_FAILED)
								&& (failure != AAMP_TUNE_LICENCE_TIMEOUT)
		                        && (failure != AAMP_TUNE_DEVICE_NOT_PROVISIONED);
		sessionParams->aamp->SendDrmErrorEvent(e.data.dash_drmmetadata.failure, e.data.dash_drmmetadata.responseCode, isRetryEnabled);
		sessionParams->aamp->profiler.SetDrmErrorCode((int)e.data.dash_drmmetadata.failure);
		sessionParams->aamp->profiler.ProfileError(PROFILE_BUCKET_LA_TOTAL, (int)e.data.dash_drmmetadata.failure);
	}
	else
	{
		if(e.data.dash_drmmetadata.accessStatus_value != 3)
		{
			AAMPLOG_INFO("Sending DRMMetaData\n");
			sessionParams->aamp->SendDRMMetaData(e);
		}
		sessionParams->aamp->profiler.ProfileEnd(PROFILE_BUCKET_LA_TOTAL);
	}
	delete sessionManger;
	free(data);
	if(contentMetadata != NULL)
		free(contentMetadata);
	free(sessionParams);
	return NULL;
}


/**
 * @brief Process content protection of adaptation
 *
 * @param[in] adaptationSet Adaptation set object
 * @param[in] mediaType     Type of track
 */
void PrivateStreamAbstractionMPD::ProcessContentProtection(IAdaptationSet * adaptationSet,MediaType mediaType)
{
	const vector<IDescriptor*> contentProt = adaptationSet->GetContentProtection();
	unsigned char* data   = NULL;
	unsigned char* wvData = NULL;
	unsigned char* prData = NULL;
	size_t dataLength     = 0;
	size_t wvDataLength   = 0;
	size_t prDataLength   = 0;
	bool isWidevine       = false;
	unsigned char* contentMetadata = NULL;

	AAMPLOG_TRACE("[HHH]contentProt.size=%d\n", contentProt.size());
	for (unsigned iContentProt = 0; iContentProt < contentProt.size(); iContentProt++)
	{
		if (contentProt.at(iContentProt)->GetSchemeIdUri().find(COMCAST_DRM_INFO_ID) != string::npos)
		{
			logprintf("[HHH]Comcast DRM Agnostic CENC system ID found!\n");
			const vector<INode*> node = contentProt.at(iContentProt)->GetAdditionalSubNodes();
			string psshData = node.at(0)->GetText();
			data = base64_Decode(psshData.c_str(), &dataLength);

			if(gpGlobalConfig->logging.trace)
			{
				logprintf("content metadata from manifest; length %d\n", dataLength);
				for (int j = 0; j < dataLength; j++)
				{
					logprintf("0x%02x ", data[j]);
					if ((j+1)%8==0) logprintf("\n");
				}
				logprintf("\n");
			}
			if(dataLength != 0)
			{
				int contentMetadataLen = 0;
				contentMetadata = _extractWVContentMetadataFromPssh((const char*)data, dataLength, &contentMetadataLen);
				if(gpGlobalConfig->logging.trace)
				{
					logprintf("content metadata from PSSH; length %d\n", contentMetadataLen);
					for (int j = 0; j < contentMetadataLen; j++)
					{
						logprintf("0x%02x ", contentMetadata[j]);
						if ((j+1)%8==0) logprintf("\n");
					}
					logprintf("\n");
				}
			}
			if(data) free(data);
			continue;
		}

		if (contentProt.at(iContentProt)->GetSchemeIdUri().find(WIDEVINE_SYSTEM_ID) != string::npos)
		{
			logprintf("[HHH]Widevine system ID found!\n");
			const vector<INode*> node = contentProt.at(iContentProt)->GetAdditionalSubNodes();
			string psshData = node.at(0)->GetText();
			wvData = base64_Decode(psshData.c_str(), &wvDataLength);
			mContext->hasDrm = true;
			if(gpGlobalConfig->logging.trace)
			{
				logprintf("init data from manifest; length %d\n", wvDataLength);
				for (int j = 0; j < wvDataLength; j++)
				{
					logprintf("%c", wvData[j]);
				}
				logprintf("\n");
			}
			continue;
		}

		if (contentProt.at(iContentProt)->GetSchemeIdUri().find(PLAYREADY_SYSTEM_ID) != string::npos)
		{
			logprintf("[HHH]Playready system ID found!\n");
			const vector<INode*> node = contentProt.at(iContentProt)->GetAdditionalSubNodes();
			string psshData = node.at(0)->GetText();
			prData = base64_Decode(psshData.c_str(), &prDataLength);
			mContext->hasDrm = true;
			if(gpGlobalConfig->logging.trace)
			{
				logprintf("init data from manifest; length %d\n", prDataLength);
				for (int j = 0; j < prDataLength; j++)
				{
					logprintf("%c", prData[j]);
				}
				logprintf("\n");
			}
			continue;
		}
	}

	// Choose widevine if both widevine and playready contentprotectiondata sections are presenet.
	// TODO: We need to add more flexible selection logic here (using aamp.cfg etc)
	if(wvData != NULL && wvDataLength > 0 && ((DRMSystems)gpGlobalConfig->preferredDrm == eDRM_WideVine || prData == NULL))
	{
		isWidevine = true;
		data = wvData;
		dataLength = wvDataLength;

		if(prData){
			free(prData);
		}
	}else if(prData != NULL && prDataLength > 0)
	{
		isWidevine = false;
		data = prData;
		dataLength = prDataLength;
		if(wvData){
			free(wvData);
		}
	}

	if(dataLength != 0)
	{
		int keyIdLen = 0;
		unsigned char* keyId = NULL;
		aamp->licenceFromManifest = true;
		keyId = _extractKeyIdFromPssh((const char*)data, dataLength, &keyIdLen, isWidevine);


		if (!(keyIdLen == lastProcessedKeyIdLen && 0 == memcmp(lastProcessedKeyId, keyId, keyIdLen)))
		{
			struct DrmSessionParams* sessionParams = (struct DrmSessionParams*)malloc(sizeof(struct DrmSessionParams));
			sessionParams->initData = data;
			sessionParams->initDataLen = dataLength;
			sessionParams->stream_type = mediaType;
			sessionParams->aamp = aamp;
			sessionParams->isWidevine = isWidevine;
			sessionParams->contentMetadata = contentMetadata;

			if(drmSessionThreadStarted) //In the case of license rotation
			{
				void *value_ptr = NULL;
				int rc = pthread_join(createDRMSessionThreadID, &value_ptr);
				if (rc != 0)
				{
					logprintf("pthread_join returned %d for createDRMSession Thread\n", rc);
				}
				drmSessionThreadStarted = false;
			}
			/*
			* Memory allocated for data via base64_Decode() and memory for sessionParams
			* is released in CreateDRMSession.
			* Memory for keyId allocated in _extractDataFromPssh() is released
			* a. In the else block of this 'if', if it's previously processed keyID
			* b. Assigned to lastProcessedKeyId which is released before new keyID is assigned
			*     or in the distructor of PrivateStreamAbstractionMPD
			*/
			if(0 == pthread_create(&createDRMSessionThreadID,NULL,CreateDRMSession,sessionParams))
			{
				drmSessionThreadStarted = true;
				if(lastProcessedKeyId)
				{
					free(lastProcessedKeyId);
				}
				lastProcessedKeyId =  keyId;
				lastProcessedKeyIdLen = keyIdLen;
				aamp->setCurrentDrm(isWidevine?eDRM_WideVine:eDRM_PlayReady);
			}
			else
			{
				logprintf("%s %d pthread_create failed for CreateDRMSession : error code %d, %s", __FUNCTION__, __LINE__, errno, strerror(errno));
			}
		}
		else
		{
			if(keyId)
			{
				free(keyId);
			}
			free(data);
		}
	}

}

#else

/**
 * @brief
 * @param[in] adaptationSet
 * @param[in] mediaType
 */
void PrivateStreamAbstractionMPD::ProcessContentProtection(IAdaptationSet * adaptationSet,MediaType mediaType)
{
	logprintf("MPD DRM not enabled\n");
}
#endif


/**
 *   @brief  GetFirstSegment start time from period
 *   @param  period
 *   @retval start time
 */
uint64_t GetFirstSegmentStartTime(IPeriod * period)
{
	uint64_t startTime = 0;
	const std::vector<IAdaptationSet *> adaptationSets = period->GetAdaptationSets();
	if (adaptationSets.size() > 0)
	{
		IAdaptationSet * firstAdaptation = adaptationSets.at(0);
		ISegmentTemplate *segmentTemplate = firstAdaptation->GetSegmentTemplate();
		if (!segmentTemplate)
		{
			const std::vector<IRepresentation *> representations = firstAdaptation->GetRepresentation();
			if (representations.size() > 0)
			{
				segmentTemplate = representations.at(0)->GetSegmentTemplate();
			}
		}
		if (segmentTemplate)
		{
			const ISegmentTimeline *segmentTimeline = segmentTemplate->GetSegmentTimeline();
			if (segmentTimeline)
			{
				std::vector<ITimeline *>&timelines = segmentTimeline->GetTimelines();
				if(timelines.size() > 0)
				{
					startTime = timelines.at(0)->GetStartTime();
				}
			}
		}
	}
	return startTime;
}

/**
 *   @brief  Get Period Duration
 *   @param  period
 *   @retval period duration
 */
uint64_t GetPeriodDuration(IPeriod * period)
{
	uint64_t durationMs = 0;

	const std::vector<IAdaptationSet *> adaptationSets = period->GetAdaptationSets();
	if (adaptationSets.size() > 0)
	{
		IAdaptationSet * firstAdaptation = adaptationSets.at(0);
		ISegmentTemplate *segmentTemplate = firstAdaptation->GetSegmentTemplate();
		if (!segmentTemplate)
		{
			const std::vector<IRepresentation *> representations = firstAdaptation->GetRepresentation();
			if (representations.size() > 0)
			{
				segmentTemplate = representations.at(0)->GetSegmentTemplate();
			}
		}
		if (segmentTemplate)
		{
			const ISegmentTimeline *segmentTimeline = segmentTemplate->GetSegmentTimeline();
			uint32_t timeScale = segmentTemplate->GetTimescale();
			durationMs = (segmentTemplate->GetDuration() / timeScale) * 1000;
			if (0 == durationMs && segmentTimeline)
			{
				std::vector<ITimeline *>&timelines = segmentTimeline->GetTimelines();
				int timeLineIndex = 0;
				while (timeLineIndex < timelines.size())
				{
					ITimeline *timeline = timelines.at(timeLineIndex);
					uint32_t repeatCount = timeline->GetRepeatCount();
					uint32_t timelineDurationMs = timeline->GetDuration() * 1000 / timeScale;
					durationMs += ((repeatCount + 1) * timelineDurationMs);
					traceprintf("%s timeLineIndex[%d] size [%lu] updated durationMs[%" PRIu64 "]\n", __FUNCTION__, timeLineIndex, timelines.size(), durationMs);
					timeLineIndex++;
				}
			}
		}
		else
		{
			const std::vector<IRepresentation *> representations = firstAdaptation->GetRepresentation();
			if (representations.size() > 0)
			{
				ISegmentList *segmentList = representations.at(0)->GetSegmentList();
				if (segmentList)
				{
					const std::vector<ISegmentURL*> segmentURLs = segmentList->GetSegmentURLs();
					durationMs += (double) segmentList->GetDuration() * 1000 / segmentList->GetTimescale();
				}
				else
				{
					aamp_Error("not-yet-supported mpd format");
				}
			}
		}
	}
	return durationMs;
}


/**
 * @brief Get end time of current period
 *
 * @retval Current period's end time
 */
uint64_t PrivateStreamAbstractionMPD::GetPeriodEndTime()
{
	uint64_t  periodStartMs = 0;
	uint64_t periodDurationMs = 0;
	uint64_t periodEndTime = 0;
	IPeriod *period = mpd->GetPeriods().at(mCurrentPeriodIdx);
	string statTimeStr = period->GetStart();
	string durationStr = period->GetDuration();
	if(!statTimeStr.empty())
	{
		ParseISO8601Duration(statTimeStr.c_str(), periodStartMs);
	}
	if(!durationStr.empty())
	{
		ParseISO8601Duration(durationStr.c_str(), periodDurationMs);
		/*uint32_t timeScale = 1000;
		ISegmentTemplate *segmentTemplate = pMediaStreamContext->adaptationSet->GetSegmentTemplate();
		if(segmentTemplate)
		{
			timeScale = segmentTemplate->GetTimescale();
		}
		*/
		periodEndTime =  (periodDurationMs + periodStartMs) / 1000;
	}
	traceprintf("PrivateStreamAbstractionMPD::%s:%d - MPD periodEndTime %lld\n", __FUNCTION__, __LINE__, periodEndTime);
	return periodEndTime;
}




/**
 *   @brief  Initialize a newly created object.
 *
 *   @param[in]  tuneType to set type of object.
 *
 *   @retval true on success
 *   @retval false on failure
 *
 *   @note   To be implemented by sub classes
 */
AAMPStatusType PrivateStreamAbstractionMPD::Init(TuneType tuneType)
{
	AAMPStatusType retval = eAAMPSTATUS_OK;
	aamp->CurlInit(0, AAMP_TRACK_COUNT);
	aamp->mStreamSink->ClearProtectionEvent();
  #ifdef AAMP_MPD_DRM
	AampDRMSessionManager::setSessionMgrState(SessionMgrState::eSESSIONMGR_ACTIVE);
  #endif
	aamp->licenceFromManifest = false;
	bool newTune = ((eTUNETYPE_NEW_NORMAL == tuneType) || (eTUNETYPE_NEW_SEEK == tuneType));

	aamp->IsTuneTypeNew = false;

#ifdef AAMP_MPD_DRM
	mPushEncInitFragment = newTune || (eTUNETYPE_RETUNE == tuneType);
#endif
	AAMPStatusType ret = UpdateMPD(aamp->mEnableCache);
	if (ret == eAAMPSTATUS_OK)
	{
		char *manifestUrl = (char *)aamp->GetManifestUrl();
		int numTracks = (rate == AAMP_NORMAL_PLAY_RATE)?AAMP_TRACK_COUNT:1;
		double offsetFromStart = seekPosition;
		uint64_t durationMs = 0;
		mNumberOfTracks = 0;
		bool mpdDurationAvailable = false;
		std::string tempString = mpd->GetMediaPresentationDuration();
		if(!tempString.empty())
		{
			ParseISO8601Duration( tempString.c_str(), durationMs);
			mpdDurationAvailable = true;
			logprintf("PrivateStreamAbstractionMPD::%s:%d - MPD duration str %s val %" PRIu64 " seconds\n", __FUNCTION__, __LINE__, tempString.c_str(), durationMs/1000);
		}

		for (int i = 0; i < AAMP_TRACK_COUNT; i++)
		{
			aamp->SetCurlTimeout(gpGlobalConfig->fragmentDLTimeout, i);
		}

		mIsLive = !(mpd->GetType() == "static");
		map<string, string> mpdAttributes = mpd->GetRawAttributes();
		if(mpdAttributes.find("fogtsb") != mpdAttributes.end())
		{
			mIsFogTSB = true;
		}

		if(mIsLive)
		{
			std::string tempStr = mpd->GetMinimumUpdatePeriod();
			if(!tempStr.empty())
			{
				ParseISO8601Duration( tempStr.c_str(), (uint64_t&)mMinUpdateDurationMs);
			}
			else
			{
				mMinUpdateDurationMs = DEFAULT_INTERVAL_BETWEEN_MPD_UPDATES_MS;
			}
			logprintf("PrivateStreamAbstractionMPD::%s:%d - MPD minupdateduration val %" PRIu64 " seconds\n", __FUNCTION__, __LINE__,  mMinUpdateDurationMs/1000);
		}

		for (int i = 0; i < numTracks; i++)
		{
			mMediaStreamContext[i] = new MediaStreamContext((TrackType)i, mContext, aamp, mMediaTypeName[i]);
			mMediaStreamContext[i]->fragmentDescriptor.manifestUrl = manifestUrl;
			mMediaStreamContext[i]->mediaType = (MediaType)i;
			mMediaStreamContext[i]->representationIndex = -1;
		}

		unsigned int nextPeriodStart = 0;
		double currentPeriodStart = 0;
		size_t numPeriods = mpd->GetPeriods().size();
		bool seekPeriods = true;
		for (unsigned iPeriod = 0; iPeriod < numPeriods; iPeriod++)
		{//TODO -  test with streams having multiple periods.
			IPeriod *period = mpd->GetPeriods().at(iPeriod);
			std::string tempString = period->GetDuration();
			uint64_t  periodStartMs = 0;
			uint64_t periodDurationMs = 0;
			if(!tempString.empty())
			{
				ParseISO8601Duration( tempString.c_str(), periodDurationMs);
				if(!mpdDurationAvailable)
				{
					durationMs += periodDurationMs;
					logprintf("%s:%d - Updated duration %" PRIu64 " seconds\n", __FUNCTION__, __LINE__, durationMs/1000);
				}
			}
			else if (mIsFogTSB)
			{
				periodDurationMs = GetPeriodDuration(period);
				durationMs += periodDurationMs;
				logprintf("%s:%d - Updated duration %" PRIu64 " seconds\n", __FUNCTION__, __LINE__, durationMs/1000);
			}

			if(offsetFromStart >= 0 && seekPeriods)
			{
				tempString = period->GetStart();
				if(!tempString.empty() && !mIsFogTSB)
				{
					ParseISO8601Duration( tempString.c_str(), periodStartMs);
				}
				else if (periodDurationMs)
				{
					periodStartMs = nextPeriodStart;
					nextPeriodStart += periodDurationMs;
				}
				if (periodDurationMs != 0)
				{
					double periodEnd = periodStartMs + periodDurationMs;
					currentPeriodStart = (double)periodStartMs/1000;
					mCurrentPeriodIdx = iPeriod;
					if (periodDurationMs/1000 <= offsetFromStart && iPeriod < (numPeriods - 1))
					{
						logprintf("Skipping period %d seekPosition %f periodEnd %f\n", iPeriod, seekPosition, periodEnd);
						offsetFromStart -= periodDurationMs/1000;
						continue;
					}
					else
					{
						seekPeriods = false;
						logprintf("currentPeriodIdx %d/%d\n", iPeriod, (int)numPeriods);
					}
				}
				else if(periodStartMs/1000 <= offsetFromStart)
				{
					mCurrentPeriodIdx = iPeriod;
					currentPeriodStart = (double)periodStartMs/1000;
				}
			}
		}

		//Check added to update offsetFromStart for
		//Multi period assets with no period duration
		if(0 == nextPeriodStart)
		{
			offsetFromStart -= currentPeriodStart;
		}

		if (0 == durationMs)
		{
			durationMs = GetDurationFromRepresentation();
			logprintf("%s:%d - Duration after GetDurationFromRepresentation %" PRIu64 " seconds\n", __FUNCTION__, __LINE__, durationMs/1000);
		}
		/*Do live adjust on live streams on 1. eTUNETYPE_NEW_NORMAL, 2. eTUNETYPE_SEEKTOLIVE,
		 * 3. Seek to a point beyond duration*/
		bool notifyEnteringLive = false;
		if (mIsLive)
		{
			double duration = (double) durationMs / 1000;
			bool liveAdjust = (eTUNETYPE_NEW_NORMAL == tuneType) && !(aamp->IsVodOrCdvrAsset());
			if (eTUNETYPE_SEEKTOLIVE == tuneType)
			{
				logprintf("PrivateStreamAbstractionMPD::%s:%d eTUNETYPE_SEEKTOLIVE\n", __FUNCTION__, __LINE__);
				liveAdjust = true;
				notifyEnteringLive = true;
			}
			else if (((eTUNETYPE_SEEK == tuneType) || (eTUNETYPE_RETUNE == tuneType || eTUNETYPE_NEW_SEEK == tuneType)) && (rate > 0))
			{
				double seekWindowEnd = duration - aamp->mLiveOffset;
				// check if seek beyond live point
				if (seekPosition > seekWindowEnd)
				{
					logprintf( "PrivateStreamAbstractionMPD::%s:%d offSetFromStart[%f] seekWindowEnd[%f]\n",
							__FUNCTION__, __LINE__, seekPosition, seekWindowEnd);
					liveAdjust = true;
					if (eTUNETYPE_SEEK == tuneType)
					{
						notifyEnteringLive = true;
					}
				}
			}
			if (liveAdjust)
			{
				mCurrentPeriodIdx = mpd->GetPeriods().size() - 1;
				if(!aamp->IsVodOrCdvrAsset())
				{
					duration = (double)GetPeriodDuration(mpd->GetPeriods().at(mCurrentPeriodIdx)) / 1000;
					if(mCurrentPeriodIdx > 0)
					{
						currentPeriodStart = ((double)durationMs / 1000) - duration;
					}
					offsetFromStart = duration - aamp->mLiveOffset;
					logprintf("%s:%d duration %f durationMs %f mCurrentPeriodIdx %d currentPeriodStart %f offsetFromStart %f \n", __FUNCTION__, __LINE__,
                                 duration, (double)durationMs / 1000, mCurrentPeriodIdx, currentPeriodStart, offsetFromStart);
				}
				else
				{
					uint64_t  periodStartMs = 0;
					IPeriod *period = mpd->GetPeriods().at(mCurrentPeriodIdx);
					std::string tempString = period->GetStart();
					ParseISO8601Duration( tempString.c_str(), periodStartMs);
					currentPeriodStart = (double)periodStartMs/1000;
					offsetFromStart = duration - aamp->mLiveOffset - currentPeriodStart;
				}
				if (offsetFromStart < 0)
				{
					offsetFromStart = 0;
				}
				mContext->mIsAtLivePoint = true;
				logprintf( "PrivateStreamAbstractionMPD::%s:%d - liveAdjust - Updated offSetFromStart[%f] duration [%f] currentPeriodStart[%f] MaxPeriodIdx[%d]\n",
						__FUNCTION__, __LINE__, offsetFromStart, duration, currentPeriodStart,mCurrentPeriodIdx);
			}
		}
		else
		{
			// Non-live - VOD/CDVR(Completed) - DELIA-30266
			double seekWindowEnd = (double) durationMs / 1000;
			if(seekPosition > seekWindowEnd)
			{
				for (int i = 0; i < mNumberOfTracks; i++)
				{
					mMediaStreamContext[i]->eosReached=true;
				}
				logprintf("%s:%d seek target out of range, mark EOS. playTarget:%f End:%f. \n",
					__FUNCTION__,__LINE__,seekPosition, seekWindowEnd);
				return eAAMPSTATUS_SEEK_RANGE_ERROR;
			}
		}
		UpdateLanguageList();
		StreamSelection(true);

		if(mNumberOfTracks)
		{
			aamp->SendEventAsync(AAMP_EVENT_PLAYLIST_INDEXED);
			TunedEventConfig tunedEventConfig =  mIsLive ?
					gpGlobalConfig->tunedEventConfigLive : gpGlobalConfig->tunedEventConfigVOD;
			if (eTUNED_EVENT_ON_PLAYLIST_INDEXED == tunedEventConfig)
			{
				if (aamp->SendTunedEvent())
				{
					logprintf("aamp: mpd - sent tune event after indexing playlist\n");
				}
			}
			UpdateTrackInfo(!newTune, true, true);

			if(notifyEnteringLive)
			{
				aamp->NotifyOnEnteringLive();
			}
			SeekInPeriod( offsetFromStart);
			seekPosition = mMediaStreamContext[eMEDIATYPE_VIDEO]->fragmentTime;
			if(0 != mCurrentPeriodIdx)
				seekPosition += currentPeriodStart;
			AAMPLOG_INFO("%s:%d  offsetFromStart(%f) seekPosition(%f) \n",__FUNCTION__,__LINE__,offsetFromStart,seekPosition);
			if (newTune )
			{
				std::vector<long> bitrateList;
				bitrateList.reserve(GetProfileCount());

				for (int i = 0; i < GetProfileCount(); i++)
				{
					if (!mStreamInfo[i].isIframeTrack)
					{
						bitrateList.push_back(mStreamInfo[i].bandwidthBitsPerSecond);
					}
				}

				aamp->SetState(eSTATE_PREPARING);
				//For DASH, presence of iframe track depends on current period.
				aamp->SendMediaMetadataEvent(durationMs, mLangList, bitrateList, mContext->hasDrm, mIsIframeTrackPresent);

				aamp->UpdateDuration(((double)durationMs)/1000);
				aamp->UpdateRefreshPlaylistInterval((float)mMinUpdateDurationMs / 1000);
			}
		}
		else
		{
			logprintf("No adaptation sets could be selected\n");
			retval = eAAMPSTATUS_MANIFEST_CONTENT_ERROR;
		}
	}
	else
	{
		retval = eAAMPSTATUS_MANIFEST_PARSE_ERROR;
	}
	return retval;
}


/**
 * @brief Get duration though representation iteration
 *
 * @retval Duration in milliseconds
 */
uint64_t PrivateStreamAbstractionMPD::GetDurationFromRepresentation()
{
	uint64_t durationMs = 0;
	size_t numPeriods = mpd->GetPeriods().size();

	for (unsigned iPeriod = 0; iPeriod < numPeriods; iPeriod++)
	{
		IPeriod *period = mpd->GetPeriods().at(iPeriod);
		const std::vector<IAdaptationSet *> adaptationSets = period->GetAdaptationSets();
		if (adaptationSets.size() > 0)
		{
			IAdaptationSet * firstAdaptation = adaptationSets.at(0);
			ISegmentTemplate *segmentTemplate = firstAdaptation->GetSegmentTemplate();
			if (!segmentTemplate)
			{
				const std::vector<IRepresentation *> representations = firstAdaptation->GetRepresentation();
				if (representations.size() > 0)
				{
					segmentTemplate = representations.at(0)->GetSegmentTemplate();
				}
			}
			if (segmentTemplate)
			{
				std::string media = segmentTemplate->Getmedia();
				const ISegmentTimeline *segmentTimeline = segmentTemplate->GetSegmentTimeline();
				if (segmentTimeline)
				{
					std::vector<ITimeline *>&timelines = segmentTimeline->GetTimelines();
					uint32_t timeScale = segmentTemplate->GetTimescale();
					int timeLineIndex = 0;
					while (timeLineIndex < timelines.size())
					{
						ITimeline *timeline = timelines.at(timeLineIndex);
						uint32_t repeatCount = timeline->GetRepeatCount();
						uint32_t timelineDurationMs = timeline->GetDuration() * 1000 / timeScale;
						durationMs += ((repeatCount + 1) * timelineDurationMs);
						traceprintf("%s period[%d] timeLineIndex[%d] size [%lu] updated durationMs[%" PRIu64 "]\n", __FUNCTION__, iPeriod, timeLineIndex, timelines.size(), durationMs);
						timeLineIndex++;
					}
				}
				else
				{
					uint32_t timeScale = segmentTemplate->GetTimescale();
					durationMs = (segmentTemplate->GetDuration() / timeScale) * 1000;
				}
			}
			else
			{
				const std::vector<IRepresentation *> representations = firstAdaptation->GetRepresentation();
				if (representations.size() > 0)
				{
					ISegmentList *segmentList = representations.at(0)->GetSegmentList();
					if (segmentList)
					{
						const std::vector<ISegmentURL*> segmentURLs = segmentList->GetSegmentURLs();
						durationMs += (double) segmentList->GetDuration() * 1000 / segmentList->GetTimescale();
					}
					else
					{
						aamp_Error("not-yet-supported mpd format");
					}
				}
			}
		}
	}
	return durationMs;
}


/**
 * @brief Update MPD manifest
 *
 * @param[in] retrievePlaylistFromCache  True to try to get from cache
 *
 * @retval true on success
 */
AAMPStatusType PrivateStreamAbstractionMPD::UpdateMPD(bool retrievePlaylistFromCache)
{
	GrowableBuffer manifest;
	AAMPStatusType ret = AAMPStatusType::eAAMPSTATUS_OK;
	int downloadAttempt = 0;
	char *manifestUrl = aamp->GetManifestUrl();
	bool gotManifest = false;
	bool retrievedPlaylistFromCache = false;
	bool harvestManifest = (gpGlobalConfig->mpdHarvestLimit && strstr(manifestUrl, "ccr.mm-"));
	if (retrievePlaylistFromCache)
	{
		memset(&manifest, 0, sizeof(manifest));
		if (aamp->RetrieveFromPlaylistCache(manifestUrl, &manifest, manifestUrl))
		{
			logprintf("PrivateStreamAbstractionMPD::%s:%d manifest retrieved from cache\n", __FUNCTION__, __LINE__);
			retrievedPlaylistFromCache = true;
		}
	}
	while( downloadAttempt < 2)
	{
		if (!retrievedPlaylistFromCache)
		{
			long http_error = 0;
			downloadAttempt++;
			memset(&manifest, 0, sizeof(manifest));
			aamp->profiler.ProfileBegin(PROFILE_BUCKET_MANIFEST);
			gotManifest = aamp->GetFile(manifestUrl, &manifest, manifestUrl, &http_error, NULL, 0, true, eMEDIATYPE_MANIFEST);
			if (gotManifest)
			{
				aamp->profiler.ProfileEnd(PROFILE_BUCKET_MANIFEST);
				if (mContext->mNetworkDownDetected)
				{
					mContext->mNetworkDownDetected = false;
				}
			}
			else if (aamp->DownloadsAreEnabled())
			{
				if(downloadAttempt < 2 && 404 == http_error)
				{
					continue;
				}
				aamp->profiler.ProfileError(PROFILE_BUCKET_MANIFEST);
				if (this->mpd != NULL && (CURLE_OPERATION_TIMEDOUT == http_error || CURLE_COULDNT_CONNECT == http_error))
				{
					//Skip this for first ever update mpd request
					mContext->mNetworkDownDetected = true;
					logprintf("PrivateStreamAbstractionMPD::%s Ignore curl timeout\n", __FUNCTION__);
					ret = AAMPStatusType::eAAMPSTATUS_OK;
					break;
				}
				aamp->SendDownloadErrorEvent(AAMP_TUNE_MANIFEST_REQ_FAILED, http_error);
				logprintf("PrivateStreamAbstractionMPD::%s - manifest download failed\n", __FUNCTION__);
				ret = AAMPStatusType::eAAMPSTATUS_MANIFEST_DOWNLOAD_ERROR;
				break;
			}
		}
		else
		{
			gotManifest = true;
		}

		if (gotManifest)
		{
#ifdef AAMP_HARVEST_SUPPORT_ENABLED
		char fileName[1024] = {'\0'};
		strcat(fileName, HARVEST_BASE_PATH);
		strcat(fileName, "manifest.mpd");
		WriteFile( fileName, manifest.ptr, manifest.len);
#endif
			// parse xml
			xmlTextReaderPtr reader = xmlReaderForMemory(manifest.ptr, (int) manifest.len, NULL, NULL, 0);

			//Dump the DAI vod manifest to /opt/logs if mpdHarvestLimit is set
			//mpds will be save with a numeric suffix, which would be going from 1 to mpdHarvestLimit
			//old mpds will get overwritten after a cycle
			if(harvestManifest)
			{
				static unsigned int counter = 0;
				string fileSuffix = to_string((counter % gpGlobalConfig->mpdHarvestLimit) + 1);
				counter++;
#ifdef WIN32
				string fullPath = "c:/tmp";
#elif defined(__APPLE__)
				string fullPath = getenv("HOME");
#else
				string fullPath = "/opt/logs";
#endif
				fullPath = fullPath + "/HarvestedMPD.txt" + fileSuffix;
				logprintf("Saving manifest to %s\n",fullPath.c_str());
				FILE *outputFile = fopen(fullPath.c_str(), "w");
				fwrite(manifest.ptr, manifest.len, 1, outputFile);
				fclose(outputFile);
			}
			if (reader != NULL)
			{
				if (xmlTextReaderRead(reader))
				{
					Node *root = ProcessNode(&reader, manifestUrl);
					if(root != NULL)
					{
						uint32_t fetchTime = Time::GetCurrentUTCTimeInSec();
						MPD* mpd = root->ToMPD();
						if (mpd)
						{
							mpd->SetFetchTime(fetchTime);
							FindTimedMetadata(mpd, root);
							if (this->mpd)
							{
								delete this->mpd;
							}
							this->mpd = mpd;
	                        aamp->mEnableCache = (mpd->GetType() == "static");
	                        if (aamp->mEnableCache && !retrievedPlaylistFromCache)
	                        {
	                            aamp->InsertToPlaylistCache(aamp->GetManifestUrl(), &manifest, aamp->GetManifestUrl());
	                        }
						}
						else
						{
						    ret = AAMPStatusType::eAAMPSTATUS_MANIFEST_CONTENT_ERROR;
						}
						delete root;
					}
					else
					{
						logprintf("Error while processing MPD, Process Node returned NULL\n");
						if(downloadAttempt < 2)
						{
							retrievedPlaylistFromCache = false;
							continue;
						}
						ret = AAMPStatusType::eAAMPSTATUS_MANIFEST_PARSE_ERROR;
					}
				}
				xmlFreeTextReader(reader);
			}

			if (gpGlobalConfig->logging.trace)
			{
				aamp_AppendNulTerminator(&manifest); // make safe for cstring operations
				logprintf("%s\n", manifest.ptr);
			}
			aamp_Free(&manifest.ptr);
			mLastPlaylistDownloadTimeMs = aamp_GetCurrentTimeMS();
		}
		else
		{
			logprintf("aamp: error on manifest fetch\n");
			ret = AAMPStatusType::eAAMPSTATUS_MANIFEST_DOWNLOAD_ERROR;
		}
		break;
	}

	if( ret == eAAMPSTATUS_MANIFEST_PARSE_ERROR || ret == eAAMPSTATUS_MANIFEST_CONTENT_ERROR)
	{
	    if(NULL != manifest.ptr && NULL != manifestUrl)
	    {
            int tempDataLen = (MANIFEST_TEMP_DATA_LENGTH - 1);
            char temp[MANIFEST_TEMP_DATA_LENGTH];
            strncpy(temp, manifest.ptr, tempDataLen);
            temp[tempDataLen] = 0x00;
	        logprintf("ERROR: Invalid Playlist URL: %s ret:%d\n", manifestUrl,ret);
	        logprintf("ERROR: Invalid Playlist DATA: %s \n", temp);
	    }
        aamp->SendErrorEvent(AAMP_TUNE_INVALID_MANIFEST_FAILURE);
	}

	return ret;
}


/**
 * @brief Find timed metadata from mainifest
 *
 * @param[in]  mpd   MPD top level element
 * @param[in]  root  XML root node
 */
void PrivateStreamAbstractionMPD::FindTimedMetadata(MPD* mpd, Node* root)
{
	std::vector<Node*> subNodes = root->GetSubNodes();

	// Does the MPD element specify a content identifier?
	const std::string& contentID = (mpd != NULL) ? mpd->GetId() : "";
	if (!contentID.empty()) {
		std::ostringstream s;
		s << "#EXT-X-CONTENT-IDENTIFIER:" << contentID;

		std::string content = s.str();
		AAMPLOG_INFO("TimedMetadata: @%1.3f %s\n", 0.0f, content.c_str());

		for (int i = 0; i < aamp->subscribedTags.size(); i++)
		{
			const std::string& tag = aamp->subscribedTags.at(i);
			if (tag == "#EXT-X-CONTENT-IDENTIFIER") {
				aamp->ReportTimedMetadata(0, tag.c_str(), content.c_str(), content.size());
				break;
			}
		}
	}

	uint64_t periodStartMS = 0;
	uint64_t periodDurationMS = 0;

	// Iterate through each of the MPD's Period nodes, and ProgrameInformation.
	for (size_t i=0; i < subNodes.size(); i++) {
		Node* node = subNodes.at(i);
		const std::string& name = node->GetName();
		if (name == "Period") {
			std::string AdID;
			std::string AssetID;
			std::string ProviderID;

			// Calculate period start time and duration
			periodStartMS += periodDurationMS;
			if (node->HasAttribute("start")) {
				const std::string& value = node->GetAttributeValue("start");
				uint64_t valueMS = 0;
				if (!value.empty())
					ParseISO8601Duration(value.c_str(), valueMS);
				if (periodStartMS < valueMS)
					periodStartMS = valueMS;
			}
			periodDurationMS = 0;
			if (node->HasAttribute("duration")) {
				const std::string& value = node->GetAttributeValue("duration");
				uint64_t valueMS = 0;
				if (!value.empty())
					ParseISO8601Duration(value.c_str(), valueMS);
				periodDurationMS = valueMS;
			}

			// Iterate through children looking for SupplementProperty nodes
			std::vector<Node*> children = node->GetSubNodes();
			for (size_t i=0; i < children.size(); i++) {
				Node* child = children.at(i);
				const std::string& name = child->GetName();
				if (name == "SupplementalProperty") {
					ProcessPeriodSupplementalProperty(child, AdID, periodStartMS, periodDurationMS);
					continue;
				}
				if (name == "AssetIdentifier") {
					ProcessPeriodAssetIdentifier(child, periodStartMS, periodDurationMS, AssetID, ProviderID);
					continue;
				}
			}
			continue;
		}
		if (name == "ProgramInformation") {
			std::vector<Node*> infoNodes = node->GetSubNodes();
			for (size_t i=0; i < infoNodes.size(); i++) {
				Node* infoNode = infoNodes.at(i);
				std::string name;
				std::string ns;
				ParseXmlNS(infoNode->GetName(), ns, name);
				if (name == "ContentIdentifier") {
					if (infoNode->HasAttribute("value")) {
						const std::string& contentID = infoNode->GetAttributeValue("value");

						std::ostringstream s;
						s << "#EXT-X-CONTENT-IDENTIFIER:" << contentID;

						std::string content = s.str();
						AAMPLOG_INFO("TimedMetadata: @%1.3f %s\n", 0.0f, content.c_str());

						for (int i = 0; i < aamp->subscribedTags.size(); i++)
						{
							const std::string& tag = aamp->subscribedTags.at(i);
							if (tag == "#EXT-X-CONTENT-IDENTIFIER") {
								aamp->ReportTimedMetadata(0, tag.c_str(), content.c_str(), content.size());
								break;
							}
						}
					}
					continue;
				}
			}
			continue;
		}
		if (name == "SupplementalProperty" && node->HasAttribute("schemeIdUri")) {
			const std::string& schemeIdUri = node->GetAttributeValue("schemeIdUri");
			if (schemeIdUri == "urn:comcast:dai:2018" && node->HasAttribute("id")) {
				const std::string& ID = node->GetAttributeValue("id");
				if (ID == "identityADS" && node->HasAttribute("value")) {
					const std::string& identityADS = node->GetAttributeValue("value");

					std::ostringstream s;
					s << "#EXT-X-IDENTITY-ADS:" << identityADS;

					std::string content = s.str();
					AAMPLOG_INFO("TimedMetadata: @%1.3f %s\n", 0.0f, content.c_str());

					for (int i = 0; i < aamp->subscribedTags.size(); i++)
					{
						const std::string& tag = aamp->subscribedTags.at(i);
						if (tag == "#EXT-X-IDENTITY-ADS") {
							aamp->ReportTimedMetadata(0, tag.c_str(), content.c_str(), content.size());
							break;
						}
					}
					continue;
				}
				if (ID == "messageRef" && node->HasAttribute("value")) {
					const std::string& messageRef = node->GetAttributeValue("value");

					std::ostringstream s;
					s << "#EXT-X-MESSAGE-REF:" << messageRef;

					std::string content = s.str();
					AAMPLOG_INFO("TimedMetadata: @%1.3f %s\n", 0.0f, content.c_str());

					for (int i = 0; i < aamp->subscribedTags.size(); i++)
					{
						const std::string& tag = aamp->subscribedTags.at(i);
						if (tag == "#EXT-X-MESSAGE-REF") {
							aamp->ReportTimedMetadata(0, tag.c_str(), content.c_str(), content.size());
							break;
						}
					}
					continue;
				}
			}
			continue;
		}
	}
}


/**
 * @brief Process supplemental property of a period
 *
 * @param[in]  node        SupplementalProperty node
 * @param[out] AdID        AD Id
 * @param[in]  startMS     Start time in MS
 * @param[in]  durationMS  Duration in MS
 */
void PrivateStreamAbstractionMPD::ProcessPeriodSupplementalProperty(Node* node, std::string& AdID, uint64_t startMS, uint64_t durationMS)
{
	if (node->HasAttribute("schemeIdUri")) {
		const std::string& schemeIdUri = node->GetAttributeValue("schemeIdUri");
		if ((schemeIdUri == "urn:comcast:dai:2018") && node->HasAttribute("id")) {
			const std::string& ID = node->GetAttributeValue("id");
			if ((ID == "Tracking") && node->HasAttribute("value")) {
				const std::string& value = node->GetAttributeValue("value");
				if (!value.empty()) {
					AdID = value;

					// Check if we need to make AdID a quoted-string
					if (AdID.find(',') != std::string::npos) {
						AdID="\"" + AdID + "\"";
					}

					double duration = durationMS / 1000.0f;
					double start = startMS / 1000.0f;

					std::ostringstream s;
					s << "#EXT-X-CUE:ID=" << AdID;
					s << ",DURATION=" << std::fixed << std::setprecision(3) << duration;
					s << ",PSN=true";

					std::string content = s.str();
					AAMPLOG_INFO("TimedMetadata: @%1.3f %s\n", start, content.c_str());

					for (int i = 0; i < aamp->subscribedTags.size(); i++)
					{
						const std::string& tag = aamp->subscribedTags.at(i);
						if (tag == "#EXT-X-CUE") {
							aamp->ReportTimedMetadata(startMS, tag.c_str(), content.c_str(), content.size());
							break;
						}
					}
				}
			}
		}
		else if (!AdID.empty() && (schemeIdUri == "urn:scte:scte130-10:2014")) {
			std::vector<Node*> children = node->GetSubNodes();
			for (size_t i=0; i < children.size(); i++) {
				Node* child = children.at(i);
				std::string name;
				std::string ns;
				ParseXmlNS(child->GetName(), ns, name);
				if (name == "StreamRestrictionListType") {
					ProcessStreamRestrictionList(child, AdID, startMS);
					continue;
				}
				if (name == "StreamRestrictionList") {
					ProcessStreamRestrictionList(child, AdID, startMS);
					continue;
				}
				if (name == "StreamRestriction") {
					ProcessStreamRestriction(child, AdID, startMS);
					continue;
				}
			}
		}
	}
}


/**
 * @brief Process Period AssetIdentifier
 *
 * @param[in] node        AssetIdentifier node
 * @param[in] startMS     Start time MS
 * @param[in] durationMS  Duration MS
 * @param[in] AssetID     Asset Id
 * @param[in] ProviderID  Provider Id
 */
void PrivateStreamAbstractionMPD::ProcessPeriodAssetIdentifier(Node* node, uint64_t startMS, uint64_t durationMS, std::string& AssetID, std::string& ProviderID)
{
	if (node->HasAttribute("schemeIdUri")) {
		const std::string& schemeIdUri = node->GetAttributeValue("schemeIdUri");
		if ((schemeIdUri == "urn:cablelabs:md:xsd:content:3.0") && node->HasAttribute("value")) {
			const std::string& value = node->GetAttributeValue("value");
			if (!value.empty()) {
				size_t pos = value.find("/Asset/");
				if (pos != std::string::npos) {
					std::string assetID = value.substr(pos+7);
					std::string providerID = value.substr(0, pos);
					double duration = durationMS / 1000.0f;
					double start = startMS / 1000.0f;

					AssetID = assetID;
					ProviderID = providerID;

					// Check if we need to make assetID a quoted-string
					if (assetID.find(',') != std::string::npos) {
						assetID="\"" + assetID + "\"";
					}

					// Check if we need to make providerID a quoted-string
					if (providerID.find(',') != std::string::npos) {
						providerID="\"" + providerID + "\"";
					}

					std::ostringstream s;
					s << "#EXT-X-ASSET-ID:ID=" << assetID;
					s << ",PROVIDER=" << providerID;
					s << ",DURATION=" << std::fixed << std::setprecision(3) << duration;

					std::string content = s.str();
					AAMPLOG_INFO("TimedMetadata: @%1.3f %s\n", start, content.c_str());

					for (int i = 0; i < aamp->subscribedTags.size(); i++)
					{
						const std::string& tag = aamp->subscribedTags.at(i);
						if (tag == "#EXT-X-ASSET-ID") {
							aamp->ReportTimedMetadata(startMS, tag.c_str(), content.c_str(), content.size());
							break;
						}
					}
				}
			}
		}
	}
}


/**
 * @brief Process Stream restriction list
 *
 * @param[in] node     StreamRestrictionListType node
 * @param[in] AdID     Ad Id
 * @param[in] startMS  Start time MS
 */
void PrivateStreamAbstractionMPD::ProcessStreamRestrictionList(Node* node, const std::string& AdID, uint64_t startMS)
{
	std::vector<Node*> children = node->GetSubNodes();
	for (size_t i=0; i < children.size(); i++) {
		Node* child = children.at(i);
		std::string name;
		std::string ns;
		ParseXmlNS(child->GetName(), ns, name);
		if (name == "StreamRestriction") {
			ProcessStreamRestriction(child, AdID, startMS);
			continue;
		}
	}
}


/**
 * @brief Process stream restriction
 *
 * @param[in] node     StreamRestriction xml node
 * @param[in] AdID     Ad ID
 * @param[in] startMS  Start time in MS
 */
void PrivateStreamAbstractionMPD::ProcessStreamRestriction(Node* node, const std::string& AdID, uint64_t startMS)
{
	std::vector<Node*> children = node->GetSubNodes();
	for (size_t i=0; i < children.size(); i++) {
		Node* child = children.at(i);
		std::string name;
		std::string ns;
		ParseXmlNS(child->GetName(), ns, name);
		if (name == "Ext") {
			ProcessStreamRestrictionExt(child, AdID, startMS);
			continue;
		}
	}
}


/**
 * @brief Process stream restriction extension
 *
 * @param[in] node     Ext child of StreamRestriction xml node
 * @param[in] AdID     Ad ID
 * @param[in] startMS  Start time in ms
 */
void PrivateStreamAbstractionMPD::ProcessStreamRestrictionExt(Node* node, const std::string& AdID, uint64_t startMS)
{
	std::vector<Node*> children = node->GetSubNodes();
	for (size_t i=0; i < children.size(); i++) {
		Node* child = children.at(i);
		std::string name;
		std::string ns;
		ParseXmlNS(child->GetName(), ns, name);
		if (name == "TrickModeRestriction") {
			ProcessTrickModeRestriction(child, AdID, startMS);
			continue;
		}
	}
}


/**
 * @brief Process trick mode restriction
 *
 * @param[in] node     TrickModeRestriction xml node
 * @param[in] AdID     Ad ID
 * @param[in] startMS  Start time in ms
 */
void PrivateStreamAbstractionMPD::ProcessTrickModeRestriction(Node* node, const std::string& AdID, uint64_t startMS)
{
	double start = startMS / 1000.0f;

	std::string trickMode;
	if (node->HasAttribute("trickMode")) {
		trickMode = node->GetAttributeValue("trickMode");
	}

	std::string scale;
	if (node->HasAttribute("scale")) {
		scale = node->GetAttributeValue("scale");
	}

	std::string text = node->GetText();
	if (!trickMode.empty() && !text.empty()) {
		std::ostringstream s;
		s << "#EXT-X-TRICKMODE-RESTRICTION"
		  << ":ADID=" << AdID
		  << ",MODE=" << trickMode
		  << ",LIMIT=" << text;

		if (!scale.empty()) {
			s << ",SCALE=" << scale;
		}

		std::string content = s.str();
		AAMPLOG_INFO("TimedMetadata: @%1.3f %s\n", start, content.c_str());

		for (int i = 0; i < aamp->subscribedTags.size(); i++)
		{
			const std::string& tag = aamp->subscribedTags.at(i);
			if (tag == "#EXT-X-TRICKMODE-RESTRICTION") {
				aamp->ReportTimedMetadata(startMS, tag.c_str(), content.c_str(), content.size());
				break;
			}
		}
	}
}


/**
 * @brief Fragment downloader thread
 *
 * @param[in] arg HeaderFetchParams pointer
 */
void * TrackDownloader(void *arg)
{
	struct HeaderFetchParams* fetchParms = (struct HeaderFetchParams*)arg;
	if(aamp_pthread_setname(pthread_self(), "aampFetchInit"))
	{
		logprintf("%s:%d: aamp_pthread_setname failed\n", __FUNCTION__, __LINE__);
	}
	//Calling WaitForFreeFragmentAvailable timeout as 0 since waiting for one tracks
	//init header fetch can slow down fragment downloads for other track
	if(fetchParms->pMediaStreamContext->WaitForFreeFragmentAvailable(0))
	{
		fetchParms->pMediaStreamContext->profileChanged = false;
		fetchParms->context->FetchFragment(fetchParms->pMediaStreamContext,
				fetchParms->initialization,
				fetchParms->fragmentduration,
				fetchParms->isinitialization, (eMEDIATYPE_AUDIO == fetchParms->pMediaStreamContext->mediaType), //CurlContext 0=Video, 1=Audio)
				fetchParms->discontinuity);
	}
	return NULL;
}


/**
 * @brief Fragment downloader thread
 *
 * @param[in] arg Pointer to FragmentDownloadParams  object
 *
 * @retval NULL
 */
void * FragmentDownloader(void *arg)
{
	struct FragmentDownloadParams* downloadParams = (struct FragmentDownloadParams*) arg;
	if(aamp_pthread_setname(pthread_self(), "aampFragDown"))
	{
		logprintf("%s:%d: aamp_pthread_setname failed\n", __FUNCTION__, __LINE__);
	}
	if (downloadParams->pMediaStreamContext->adaptationSet)
	{
		while (downloadParams->context->aamp->DownloadsAreEnabled() && !downloadParams->pMediaStreamContext->profileChanged)
		{
			int timeoutMs = downloadParams->context->GetMinUpdateDuration() - (int)(aamp_GetCurrentTimeMS() - downloadParams->lastPlaylistUpdateMS);
			if(downloadParams->pMediaStreamContext->WaitForFreeFragmentAvailable(timeoutMs))
			{
				downloadParams->context->PushNextFragment(downloadParams->pMediaStreamContext, 1);
				if (downloadParams->pMediaStreamContext->eos)
				{
					if(!downloadParams->context->aamp->IsLive() && downloadParams->playingLastPeriod)
					{
						downloadParams->pMediaStreamContext->eosReached = true;
						downloadParams->pMediaStreamContext->AbortWaitForCachedFragment(false);
					}
					AAMPLOG_INFO("%s:%d %s EOS - Exit fetch loop\n", __FUNCTION__, __LINE__, downloadParams->pMediaStreamContext->name);
					break;
				}
				if (downloadParams->pMediaStreamContext->endTimeReached)
				{
					AAMPLOG_INFO("%s:%d - endTimeReached\n", __FUNCTION__, __LINE__);
					downloadParams->context->aamp->EndTimeReached(eMEDIATYPE_AUDIO);
					break;
				}
			}
			timeoutMs = downloadParams->context->GetMinUpdateDuration() - (int)(aamp_GetCurrentTimeMS() - downloadParams->lastPlaylistUpdateMS);
			if(timeoutMs <= 0 && downloadParams->context->aamp->IsLive())
			{
				break;
			}
		}
	}
	else
	{
		logprintf("%s:%d NULL adaptationSet\n", __FUNCTION__, __LINE__);
	}
	return NULL;
}

/**
 * @brief Fragment collector thread
 *
 * @param[in] arg Pointer to PrivateStreamAbstractionMPD object
 *
 * @retval NULL
 */
static void * FragmentCollector(void *arg)
{
	PrivateStreamAbstractionMPD *context = (PrivateStreamAbstractionMPD *)arg;
	if(aamp_pthread_setname(pthread_self(), "aampMPDFetch"))
	{
		logprintf("%s:%d: aamp_pthread_setname failed\n", __FUNCTION__, __LINE__);
	}
	context->FetcherLoop();
	return NULL;
}


/**
 * @brief Check if adaptation set is iframe track
 *
 * @param[in] adaptationSet Pointer to adaptainSet
 *
 * @retval true if iframe track
 */
static bool IsIframeTrack(IAdaptationSet *adaptationSet)
{
	const std::vector<INode *> subnodes = adaptationSet->GetAdditionalSubNodes();
	for (unsigned i = 0; i < subnodes.size(); i++)
	{
		INode *xml = subnodes[i];
		if (xml->GetName() == "EssentialProperty")
		{
			if (xml->HasAttribute("schemeIdUri"))
			{
				const std::string& schemeUri = xml->GetAttributeValue("schemeIdUri");
				if (schemeUri == "http://dashif.org/guidelines/trickmode")
				{
					return true;
				}
				else
				{
					logprintf("%s:%d - skipping schemeUri %s\n", __FUNCTION__, __LINE__, schemeUri.c_str());
				}
			}
		}
		else
		{
			logprintf("%s:%d - skipping name %s\n", __FUNCTION__, __LINE__, xml->GetName().c_str());
		}
	}
	return false;
}

/**
 * @brief Update language list state variables
 */
void PrivateStreamAbstractionMPD::UpdateLanguageList()
{
	size_t numPeriods = mpd->GetPeriods().size();
	for (unsigned iPeriod = 0; iPeriod < numPeriods; iPeriod++)
	{
		IPeriod *period = mpd->GetPeriods().at(iPeriod);
		size_t numAdaptationSets = period->GetAdaptationSets().size();
		for (int iAdaptationSet = 0; iAdaptationSet < numAdaptationSets; iAdaptationSet++)
		{
			IAdaptationSet *adaptationSet = period->GetAdaptationSets().at(iAdaptationSet);
			if (IsContentType(adaptationSet, eMEDIATYPE_AUDIO ))
			{
				mLangList.insert(adaptationSet->GetLang());
			}
		}
	}
}


/**
 * @brief Does stream selection
 *
 * @param[in] newTune True if this is a new tune
 */
void PrivateStreamAbstractionMPD::StreamSelection( bool newTune)
{
	int numTracks = (rate == AAMP_NORMAL_PLAY_RATE)?AAMP_TRACK_COUNT:1;
	mNumberOfTracks = 0;

	IPeriod *period = NULL;
	bool periodAdaptSetfound = true;
	size_t numPeriods = mpd->GetPeriods().size();

	do
	{
		period = mpd->GetPeriods().at(mCurrentPeriodIdx);
		periodAdaptSetfound = !(period->GetAdaptationSets().size() == 0);
		if(periodAdaptSetfound)
			break;
		logprintf("%s:%d Adaptation Sets not found for periodIdx: %lu\n", __FUNCTION__, __LINE__, mCurrentPeriodIdx);
	}while(++mCurrentPeriodIdx < numPeriods);

	if(!periodAdaptSetfound)
	{
		logprintf("%s:%d No Valid Adaptation Sets found, total periods: %lu\n", __FUNCTION__, __LINE__, numPeriods);
		return;
	}

	AAMPLOG_INFO("Selected Period index %d, id %s\n", mCurrentPeriodIdx, period->GetId().c_str());

	uint64_t  periodStartMs = 0;
	mPeriodEndTime = GetPeriodEndTime();
	string statTimeStr = period->GetStart();
	if(!statTimeStr.empty())
	{
		ParseISO8601Duration(statTimeStr.c_str(), periodStartMs);
	}
	mPeriodStartTime = periodStartMs / 1000;
	for (int i = 0; i < numTracks; i++)
	{
		struct MediaStreamContext *pMediaStreamContext = mMediaStreamContext[i];
		size_t numAdaptationSets = period->GetAdaptationSets().size();
		int  selAdaptationSetIndex = -1;
		int selRepresentationIndex = -1;
		AudioType selectedRepType = eAUDIO_UNKNOWN, internalSelRepType ;
		int desiredCodecIdx = -1;
		bool otherLanguageSelected = false;
		mMediaStreamContext[i]->enabled = false;
		std::string selectedLanguage;
		bool isIframeAdaptationAvailable = false;
		for (unsigned iAdaptationSet = 0; iAdaptationSet < numAdaptationSets; iAdaptationSet++)
		{
			IAdaptationSet *adaptationSet = period->GetAdaptationSets().at(iAdaptationSet);
			AAMPLOG_TRACE("PrivateStreamAbstractionMPD::%s %d > Content type [%s] AdapSet [%d] \n",
					__FUNCTION__, __LINE__, adaptationSet->GetContentType().c_str(),iAdaptationSet);
			if (IsContentType(adaptationSet, (MediaType)i ))
			{
				if (AAMP_NORMAL_PLAY_RATE == rate)
				{
					if (eMEDIATYPE_AUDIO == i)
					{
						std::string lang = adaptationSet->GetLang();
						internalSelRepType = selectedRepType;
						// found my language configured
						if(strncmp(aamp->language, lang.c_str(), MAX_LANGUAGE_TAG_LENGTH) == 0)
						{
							// check if already other lang adap is selected, if so start fresh
							if (otherLanguageSelected)
							{
								internalSelRepType = eAUDIO_UNKNOWN;
							}
							desiredCodecIdx = GetDesiredCodecIndex(adaptationSet, internalSelRepType);
							if(desiredCodecIdx != -1 )
							{
								otherLanguageSelected = false;
								selectedRepType	= internalSelRepType;
								selAdaptationSetIndex = iAdaptationSet;
								selRepresentationIndex = desiredCodecIdx;
								mAudioType = selectedRepType;
							}
							logprintf("PrivateStreamAbstractionMPD::%s %d > Got the matching lang[%s] AdapInx[%d] RepIndx[%d] AudioType[%d]\n",
								__FUNCTION__, __LINE__, lang.c_str(), selAdaptationSetIndex, selRepresentationIndex, selectedRepType);
						}
						else if(internalSelRepType == eAUDIO_UNKNOWN || otherLanguageSelected)
						{
							// Got first Adap with diff language , store it now until we find another matching lang adaptation
							desiredCodecIdx = GetDesiredCodecIndex(adaptationSet, internalSelRepType);
							if(desiredCodecIdx != -1)
							{
								otherLanguageSelected = true;
								selectedLanguage = lang;
								selectedRepType	= internalSelRepType;
								selAdaptationSetIndex = iAdaptationSet;
								selRepresentationIndex = desiredCodecIdx;
								mAudioType = selectedRepType;
							}
							logprintf("PrivateStreamAbstractionMPD::%s %d > Got a non-matching lang[%s] AdapInx[%d] RepIndx[%d] AudioType[%d]\n",
								__FUNCTION__, __LINE__, lang.c_str(), selAdaptationSetIndex, selRepresentationIndex, selectedRepType);
						}
					}
					else if (!gpGlobalConfig->bAudioOnlyPlayback)
					{
						if (!isIframeAdaptationAvailable || selAdaptationSetIndex == -1)
						{
							if (!IsIframeTrack(adaptationSet))
							{
								// Got Video , confirmed its not iframe adaptation
								selAdaptationSetIndex =	iAdaptationSet;
								if(!newTune)
								{
									if(GetProfileCount() == adaptationSet->GetRepresentation().size())
									{
										selRepresentationIndex = pMediaStreamContext->representationIndex;
									}
									else
									{
										selRepresentationIndex = -1; // this will be set based on profile selection
									}
								}
							}
							else
							{
								isIframeAdaptationAvailable = true;
							}
						}
						else
						{
							break;
						}
					}
				}
				else if ((!gpGlobalConfig->bAudioOnlyPlayback) && (eMEDIATYPE_VIDEO == i))
				{
					//iframe track
					if ( IsIframeTrack(adaptationSet) )
					{
						logprintf("PrivateStreamAbstractionMPD::%s %d > Got TrickMode track\n", __FUNCTION__, __LINE__);
						pMediaStreamContext->enabled = true;
						pMediaStreamContext->profileChanged = true;
						pMediaStreamContext->adaptationSetIdx = iAdaptationSet;
						mNumberOfTracks = 1;
						isIframeAdaptationAvailable = true;
						break;
					}
				}
			}
		}

		// end of adaptation loop
		if ((AAMP_NORMAL_PLAY_RATE == rate) && (pMediaStreamContext->enabled == false) && selAdaptationSetIndex >= 0)
		{
			pMediaStreamContext->enabled = true;
			pMediaStreamContext->adaptationSetIdx = selAdaptationSetIndex;
			pMediaStreamContext->representationIndex = selRepresentationIndex;
			pMediaStreamContext->profileChanged = true;
			//preferred audio language was not available, hence selected best audio format
			if (otherLanguageSelected)
			{
				if (mLangList.end() ==  mLangList.find(aamp->language))
				{
					logprintf("PrivateStreamAbstractionMPD::%s %d > update language [%s]->[%s]\n",
									__FUNCTION__, __LINE__, aamp->language, selectedLanguage.c_str());
					aamp->UpdateAudioLanguageSelection(selectedLanguage.c_str());
				}
				else
				{
					logprintf("PrivateStreamAbstractionMPD::%s %d > [%s] not available in period. Select [%s]\n",
									__FUNCTION__, __LINE__, aamp->language, selectedLanguage.c_str());
				}
			}

			/* To solve a no audio issue - Force configure gst audio pipeline/playbin in the case of multi period
			 * multi audio codec available for current decoding language on stream. For example, first period has AAC no EC3,
			 * so the player will choose AAC then start decoding, but in the forthcoming periods,
			 * if the stream has AAC and EC3 for the current decoding language then as per the EC3(default priority)
			 * the player will choose EC3 but the audio pipeline actually not configured in this case to affect this change.
			 */
			if ( aamp->previousAudioType != selectedRepType && eMEDIATYPE_AUDIO == i )
			{
				logprintf("PrivateStreamAbstractionMPD::%s %d > AudioType Changed %d -> %d\n",
						__FUNCTION__, __LINE__, aamp->previousAudioType, selectedRepType);
				aamp->previousAudioType = selectedRepType;
				mContext->SetESChangeStatus();
			}

			logprintf("PrivateStreamAbstractionMPD::%s %d > Media[%s] Adaptation set[%d] RepIdx[%d] TrackCnt[%d]\n",
				__FUNCTION__, __LINE__, mMediaTypeName[i],selAdaptationSetIndex,selRepresentationIndex,(mNumberOfTracks+1) );

			ProcessContentProtection(period->GetAdaptationSets().at(selAdaptationSetIndex),(MediaType)i);
			mNumberOfTracks++;
		}

		if(selAdaptationSetIndex < 0 && rate == 1)
		{
			logprintf("PrivateStreamAbstractionMPD::%s %d > No valid adaptation set found for Media[%s]\n",
				__FUNCTION__, __LINE__, mMediaTypeName[i]);
		}

		logprintf("PrivateStreamAbstractionMPD::%s %d > Media[%s] %s\n",
			__FUNCTION__, __LINE__, mMediaTypeName[i], pMediaStreamContext->enabled?"enabled":"disabled");

		//Store the iframe track status in current period if there is any change
		if (!gpGlobalConfig->bAudioOnlyPlayback && (i == eMEDIATYPE_VIDEO) && (mIsIframeTrackPresent != isIframeAdaptationAvailable))
		{
			mIsIframeTrackPresent = isIframeAdaptationAvailable;
			//Iframe tracks changed mid-stream, sent a playbackspeed changed event
			if (!newTune)
			{
				aamp->SendSupportedSpeedsChangedEvent(mIsIframeTrackPresent);
			}
		}
	}

	if(1 == mNumberOfTracks && !mMediaStreamContext[eMEDIATYPE_VIDEO]->enabled)
	{
		if(newTune)
			logprintf("PrivateStreamAbstractionMPD::%s:%d Audio only period\n", __FUNCTION__, __LINE__);
		mMediaStreamContext[eMEDIATYPE_VIDEO]->enabled = mMediaStreamContext[eMEDIATYPE_AUDIO]->enabled;
		mMediaStreamContext[eMEDIATYPE_VIDEO]->adaptationSetIdx = mMediaStreamContext[eMEDIATYPE_AUDIO]->adaptationSetIdx;
		mMediaStreamContext[eMEDIATYPE_VIDEO]->representationIndex = mMediaStreamContext[eMEDIATYPE_AUDIO]->representationIndex;
		mMediaStreamContext[eMEDIATYPE_VIDEO]->mediaType = eMEDIATYPE_VIDEO;
		mMediaStreamContext[eMEDIATYPE_VIDEO]->type = eTRACK_VIDEO;
		mMediaStreamContext[eMEDIATYPE_VIDEO]->profileChanged = true;
		mMediaStreamContext[eMEDIATYPE_AUDIO]->enabled = false;
	}
}

/**
 * @brief Extract bitrate info from custom mpd
 *
 * @param[in]  AdaptationSet     AdaptationSet from which bitrate info is to be extracted
 * @param[out] representations   Representation vector gets updated with available bit rates.
 *
 * @note Caller function should delete the vector elements after use.
 */
static void GetBitrateInfoFromCustomMpd(IAdaptationSet *adaptationSet, std::vector<Representation *>& representations )
{
	std::vector<xml::INode *> subNodes = adaptationSet->GetAdditionalSubNodes();
	for(int i = 0; i < subNodes.size(); i ++)
	{
		xml::INode * node = subNodes.at(i);
		if(node->GetName() == "AvailableBitrates")
		{
			std::vector<xml::INode *> reprNodes = node->GetNodes();
			for(int reprIter = 0; reprIter < reprNodes.size(); reprIter++)
			{
				xml::INode * reprNode = reprNodes.at(reprIter);
				if(reprNode->GetName() == "Representation")
				{
					dash::mpd::Representation * repr = new dash::mpd::Representation();
					if(reprNode->HasAttribute("bandwidth"))
					{
						repr->SetBandwidth(stol(reprNode->GetAttributeValue("bandwidth")));
					}
					if(reprNode->HasAttribute("height"))
					{
						repr->SetHeight(stol(reprNode->GetAttributeValue("height")));
					}
					if(reprNode->HasAttribute("width"))
					{
						repr->SetWidth(stol(reprNode->GetAttributeValue("width")));
					}
					representations.push_back(repr);
				}
			}
			break;
		}
	}
}


/**
 * @brief Updates track information based on current state
 */
void PrivateStreamAbstractionMPD::UpdateTrackInfo(bool modifyDefaultBW, bool periodChanged, bool resetTimeLineIndex)
{
	long defaultBitrate = gpGlobalConfig->defaultBitrate;
	long iframeBitrate = gpGlobalConfig->iframeBitrate;
	for (int i = 0; i < mNumberOfTracks; i++)
	{
		struct MediaStreamContext *pMediaStreamContext = mMediaStreamContext[i];
		if(pMediaStreamContext->enabled)
		{
			IPeriod *period = mpd->GetPeriods().at(mCurrentPeriodIdx);
			pMediaStreamContext->adaptationSet = period->GetAdaptationSets().at(pMediaStreamContext->adaptationSetIdx);
			pMediaStreamContext->adaptationSetId = pMediaStreamContext->adaptationSet->GetId();
			/*Populate StreamInfo for ABR Processing*/
			if (i == eMEDIATYPE_VIDEO)
			{
				if(mIsFogTSB && periodChanged)
				{
					vector<Representation *> representations;
					GetBitrateInfoFromCustomMpd(pMediaStreamContext->adaptationSet, representations);
					int representationCount = representations.size();
					if ((representationCount != mBitrateIndexMap.size()) && mStreamInfo)
					{
						delete[] mStreamInfo;
						mStreamInfo = NULL;
					}
					if (!mStreamInfo)
					{
						mStreamInfo = new StreamInfo[representationCount];
					}
					mBitrateIndexMap.clear();
					for (int idx = 0; idx < representationCount; idx++)
					{
						Representation* representation = representations.at(idx);
						mStreamInfo[idx].bandwidthBitsPerSecond = representation->GetBandwidth();
						mStreamInfo[idx].isIframeTrack = !(AAMP_NORMAL_PLAY_RATE == rate);
						mStreamInfo[idx].resolution.height = representation->GetHeight();
						mStreamInfo[idx].resolution.width = representation->GetWidth();
						mBitrateIndexMap[mStreamInfo[idx].bandwidthBitsPerSecond] = idx;
						delete representation;
					}
					pMediaStreamContext->representationIndex = 0; //Fog custom mpd has a single representation
					IRepresentation* representation = pMediaStreamContext->adaptationSet->GetRepresentation().at(0);
					pMediaStreamContext->fragmentDescriptor.Bandwidth = representation->GetBandwidth();
					aamp->profiler.SetBandwidthBitsPerSecondVideo(pMediaStreamContext->fragmentDescriptor.Bandwidth);
					mContext->profileIdxForBandwidthNotification = mBitrateIndexMap[pMediaStreamContext->fragmentDescriptor.Bandwidth];
				}
				else if(!mIsFogTSB)
				{
					int representationCount = pMediaStreamContext->adaptationSet->GetRepresentation().size();
					if ((representationCount != GetProfileCount()) && mStreamInfo)
					{
						delete[] mStreamInfo;
						mStreamInfo = NULL;
					}
					if (!mStreamInfo)
					{
						mStreamInfo = new StreamInfo[representationCount];
					}
					mContext->GetABRManager().clearProfiles();
					for (int idx = 0; idx < representationCount; idx++)
					{
						IRepresentation *representation = pMediaStreamContext->adaptationSet->GetRepresentation().at(idx);
						mStreamInfo[idx].bandwidthBitsPerSecond = representation->GetBandwidth();
						mStreamInfo[idx].isIframeTrack = !(AAMP_NORMAL_PLAY_RATE == rate);
						mStreamInfo[idx].resolution.height = representation->GetHeight();
						mStreamInfo[idx].resolution.width = representation->GetWidth();
						mContext->GetABRManager().addProfile({
							mStreamInfo[idx].isIframeTrack,
							mStreamInfo[idx].bandwidthBitsPerSecond,
							mStreamInfo[idx].resolution.width,
							mStreamInfo[idx].resolution.height,
						});
						if(mStreamInfo[idx].resolution.height > 1080
								|| mStreamInfo[idx].resolution.width > 1920)
						{
							defaultBitrate = gpGlobalConfig->defaultBitrate4K;
							iframeBitrate = gpGlobalConfig->iframeBitrate4K;
						}
					}

					if (modifyDefaultBW)
					{
						long persistedBandwidth = aamp->GetPersistedBandwidth();
						if(persistedBandwidth > 0 && (persistedBandwidth < defaultBitrate))
						{
							defaultBitrate = persistedBandwidth;
						}
					}
				}

				if (defaultBitrate != gpGlobalConfig->defaultBitrate)
				{
					mContext->GetABRManager().setDefaultInitBitrate(defaultBitrate);
				}
			}

			if(-1 == pMediaStreamContext->representationIndex)
			{
				if(!mIsFogTSB)
				{
					if(i == eMEDIATYPE_VIDEO)
					{
						if(mContext->trickplayMode)
						{
							if (iframeBitrate > 0)
							{
								mContext->GetABRManager().setDefaultIframeBitrate(iframeBitrate);
							}
							mContext->UpdateIframeTracks();
						}
						if (defaultBitrate != DEFAULT_INIT_BITRATE)
						{
							mContext->currentProfileIndex = mContext->GetDesiredProfile(false);
						}
						else
						{
							mContext->currentProfileIndex = mContext->GetDesiredProfile(true);
						}
						pMediaStreamContext->representationIndex = mContext->currentProfileIndex;
						IRepresentation *selectedRepresentation = pMediaStreamContext->adaptationSet->GetRepresentation().at(pMediaStreamContext->representationIndex);
						// for the profile selected ,reset the abr values with default bandwidth values
						aamp->ResetCurrentlyAvailableBandwidth(selectedRepresentation->GetBandwidth(),mContext->trickplayMode,mContext->currentProfileIndex);
						aamp->profiler.SetBandwidthBitsPerSecondVideo(selectedRepresentation->GetBandwidth());
	//					aamp->NotifyBitRateChangeEvent(selectedRepresentation->GetBandwidth(),
	//							"BitrateChanged - Network Adaptation",
	//							selectedRepresentation->GetWidth(),
	//							selectedRepresentation->GetHeight());
					}
					else
					{
						pMediaStreamContext->representationIndex = pMediaStreamContext->adaptationSet->GetRepresentation().size() / 2; //Select the medium profile on start
						if(i == eMEDIATYPE_AUDIO)
						{
							IRepresentation *selectedRepresentation = pMediaStreamContext->adaptationSet->GetRepresentation().at(pMediaStreamContext->representationIndex);
							aamp->profiler.SetBandwidthBitsPerSecondAudio(selectedRepresentation->GetBandwidth());
						}
					}
				}
				else
				{
					logprintf("%s:%d: [WARN] !! representationIndex is '-1' override with '0' since Custom MPD has single representation\n", __FUNCTION__, __LINE__);
					pMediaStreamContext->representationIndex = 0; //Fog custom mpd has single representation
				}
			}
			pMediaStreamContext->representation = pMediaStreamContext->adaptationSet->GetRepresentation().at(pMediaStreamContext->representationIndex);

			pMediaStreamContext->fragmentDescriptor.baseUrls = &pMediaStreamContext->representation->GetBaseURLs();
			if (pMediaStreamContext->fragmentDescriptor.baseUrls->size() == 0)
			{
				pMediaStreamContext->fragmentDescriptor.baseUrls = &pMediaStreamContext->adaptationSet->GetBaseURLs();
				if (pMediaStreamContext->fragmentDescriptor.baseUrls->size() == 0)
				{
					pMediaStreamContext->fragmentDescriptor.baseUrls = &mpd->GetPeriods().at(mCurrentPeriodIdx)->GetBaseURLs();
					if (pMediaStreamContext->fragmentDescriptor.baseUrls->size() == 0)
					{
						pMediaStreamContext->fragmentDescriptor.baseUrls = &mpd->GetBaseUrls();
					}
				}
			}
			pMediaStreamContext->fragmentIndex = 0;
			if(resetTimeLineIndex)
				pMediaStreamContext->timeLineIndex = 0;
			pMediaStreamContext->fragmentRepeatCount = 0;
			pMediaStreamContext->fragmentOffset = 0;
			pMediaStreamContext->fragmentTime = 0;
			pMediaStreamContext->eos = false;
			if(0 == pMediaStreamContext->fragmentDescriptor.Bandwidth || !aamp->IsTSBSupported())
			{
				pMediaStreamContext->fragmentDescriptor.Bandwidth = pMediaStreamContext->representation->GetBandwidth();
			}
			strcpy(pMediaStreamContext->fragmentDescriptor.RepresentationID, pMediaStreamContext->representation->GetId().c_str());
			pMediaStreamContext->fragmentDescriptor.Time = 0;
			ISegmentTemplate *segmentTemplate = pMediaStreamContext->adaptationSet->GetSegmentTemplate();
			if(!segmentTemplate)
			{
				segmentTemplate = pMediaStreamContext->representation->GetSegmentTemplate();
			}
			if(segmentTemplate)
			{
				pMediaStreamContext->fragmentDescriptor.Number = segmentTemplate->GetStartNumber();
				AAMPLOG_INFO("PrivateStreamAbstractionMPD::%s:%d Track %d timeLineIndex %d fragmentDescriptor.Number %lu\n", __FUNCTION__, __LINE__, i, pMediaStreamContext->timeLineIndex, pMediaStreamContext->fragmentDescriptor.Number);
			}
		}
	}
	if (mIsLive && !aamp->IsVodOrCdvrAsset() && mMediaStreamContext[eMEDIATYPE_VIDEO]->enabled)
	{
		UpdateCullingState();
	}
}



/**
 * @brief Update culling state for live manifests
 */
void PrivateStreamAbstractionMPD::UpdateCullingState()
{
	double newStartTimeSeconds = 0;
	traceprintf("PrivateStreamAbstractionMPD::%s:%d Enter\n", __FUNCTION__, __LINE__);
	MediaStreamContext *pMediaStreamContext = mMediaStreamContext[eMEDIATYPE_VIDEO];
	if (pMediaStreamContext->adaptationSet)
	{
		ISegmentTemplate *segmentTemplate = pMediaStreamContext->adaptationSet->GetSegmentTemplate();
		const ISegmentTimeline *segmentTimeline = NULL;
		if (!segmentTemplate && pMediaStreamContext->representation)
		{
			segmentTemplate = pMediaStreamContext->representation->GetSegmentTemplate();
		}

		if (segmentTemplate)
		{
			segmentTimeline = segmentTemplate->GetSegmentTimeline();
		}
		else
		{
			ISegmentList *segmentList = pMediaStreamContext->representation->GetSegmentList();
			if (segmentList)
			{
				std::map<string,string> rawAttributes = segmentList->GetRawAttributes();
				if(rawAttributes.find("customlist") == rawAttributes.end())
				{
					segmentTimeline = segmentList->GetSegmentTimeline();
				}
				else
				{
					//Updated logic for culling,
					vector<IPeriod*> periods =  mpd->GetPeriods();
					long duration = 0;
					long prevLastSegUrlOffset = 0;
					long newOffset = 0;
					bool offsetFound = false;
					std::string newMedia;
					for(int iPeriod = periods.size() - 1 ; iPeriod >= 0; iPeriod--)
					{
						IPeriod* period = periods.at(iPeriod);
						vector<IAdaptationSet *> adaptationSets = period->GetAdaptationSets();
						if (adaptationSets.empty())
						{
							continue;
						}
						IAdaptationSet * adaptationSet = adaptationSets.at(0);

						vector<IRepresentation *> representations = adaptationSet->GetRepresentation();

						if(representations.empty())
						{
							continue;
						}

						IRepresentation* representation = representations.at(0);
						ISegmentList *segmentList = representation->GetSegmentList();

						if(!segmentList)
						{
							continue;
						}

						duration += segmentList->GetDuration();
						vector<ISegmentURL*> segUrls = segmentList->GetSegmentURLs();
						if(!segUrls.empty())
						{
							for(int iSegurl = segUrls.size() - 1; iSegurl >= 0 && !offsetFound; iSegurl--)
							{
								std::string media = segUrls.at(iSegurl)->GetMediaURI();
								std::string offsetStr = segUrls.at(iSegurl)->GetRawAttributes().at("d");
								uint32_t offset = stol(offsetStr);
								if(0 == newOffset)
								{
									newOffset = offset;
									newMedia = media;
								}
								if(0 == mPrevLastSegurlOffset && !offsetFound)
								{
									offsetFound = true;
									break;
								}
								else if(mPrevLastSegurlMedia == media)
								{
									offsetFound = true;
									prevLastSegUrlOffset += offset;
									break;
								}
								else
								{
									prevLastSegUrlOffset += offset;
								}
							}//End of segurl for loop
						}
					} //End of Period for loop
					double culled = 0;
					long offsetDiff = 0;
					long currOffset = duration - prevLastSegUrlOffset;
					if(mPrevLastSegurlOffset)
					{
						long timescale = segmentList->GetTimescale();
						offsetDiff = mPrevLastSegurlOffset - currOffset;
						if(offsetDiff > 0)
						{
							culled = (double)offsetDiff / timescale;
						}
					}
					AAMPLOG_INFO("PrivateStreamAbstractionMPD::%s:%d PrevOffset %ld CurrentOffset %ld culled (%f)\n", __FUNCTION__, __LINE__, mPrevLastSegurlOffset, currOffset, culled);
					mPrevLastSegurlOffset = duration - newOffset;
					mPrevLastSegurlMedia = newMedia;
					if(culled)
					{
						aamp->UpdateCullingState(culled);
					}
					return;
				}
			}
			else
			{
				AAMPLOG_INFO("PrivateStreamAbstractionMPD::%s:%d NULL segmentTemplate and segmentList\n", __FUNCTION__, __LINE__);
			}
		}
		if (segmentTimeline)
		{
			double culled = 0;
			auto periods = mpd->GetPeriods();
			vector<PeriodInfo> currMPDPeriodDetails;
			uint32_t timescale = segmentTemplate->GetTimescale();
			for (auto period : periods)
			{
				PeriodInfo periodInfo;
				periodInfo.periodId = period->GetId();
				periodInfo.duration = (double)GetPeriodDuration(period)/ 1000;
				periodInfo.startTime = GetFirstSegmentStartTime(period);
				currMPDPeriodDetails.push_back(periodInfo);
			}

			int iter1 = 0;
			PeriodInfo currFirstPeriodInfo = currMPDPeriodDetails.at(0);
			while (iter1 < mMPDPeriodsInfo.size())
			{
				PeriodInfo prevPeriodInfo = mMPDPeriodsInfo.at(iter1);
				if(prevPeriodInfo.periodId == currFirstPeriodInfo.periodId)
				{
					if(prevPeriodInfo.startTime && currFirstPeriodInfo.startTime)
					{
						uint64_t timeDiff = currFirstPeriodInfo.startTime - prevPeriodInfo.startTime;
						culled += ((double)timeDiff / (double)timescale);
						logprintf("%s:%d PeriodId %s, prevStart %" PRIu64 " currStart %" PRIu64 " culled %f\n", __FUNCTION__, __LINE__,
											prevPeriodInfo.periodId.c_str(), prevPeriodInfo.startTime, currFirstPeriodInfo.startTime, culled);
					}
					break;
				}
				else
				{
					culled += prevPeriodInfo.duration;
					iter1++;
					logprintf("%s:%d PeriodId %s , with last known duration %f seems to have got culled\n", __FUNCTION__, __LINE__,
									prevPeriodInfo.periodId.c_str(), prevPeriodInfo.duration);
				}
			}

			mMPDPeriodsInfo = currMPDPeriodDetails;

			if(culled)
			{
				logprintf("%s:%d Culled seconds = %f\n", __FUNCTION__, __LINE__, culled);
				aamp->UpdateCullingState(culled);
			}
			return;
		}
		else
		{
			AAMPLOG_INFO("PrivateStreamAbstractionMPD::%s:%d NULL segmentTimeline. Hence modifying culling logic based on MPD availabilityStartTime, periodStartTime, fragment number and current time\n", __FUNCTION__, __LINE__);
			string startTimestr = mpd->GetAvailabilityStarttime();
			std::tm time = { 0 };
			strptime(startTimestr.c_str(), "%Y-%m-%dT%H:%M:%SZ", &time);
			double availabilityStartTime = (double)mktime(&time);
			double currentTimeSeconds = aamp_GetCurrentTimeMS() / 1000;
			double fragmentDuration = ((double)segmentTemplate->GetDuration()) / segmentTemplate->GetTimescale();
			double newStartTimeSeconds = 0;
			if (0 == pMediaStreamContext->lastSegmentNumber)
			{
				newStartTimeSeconds = availabilityStartTime;
			}

			// Recalculate the newStartTimeSeconds after periodic manifest updates
			if (mIsLive && 0 == newStartTimeSeconds)
			{
				newStartTimeSeconds = availabilityStartTime + mPeriodStartTime + ((pMediaStreamContext->lastSegmentNumber - segmentTemplate->GetStartNumber()) * fragmentDuration);
			}

			if (newStartTimeSeconds && mPrevStartTimeSeconds)
			{
				double culled = newStartTimeSeconds - mPrevStartTimeSeconds;
				traceprintf("PrivateStreamAbstractionMPD::%s:%d post-refresh %fs before %f (%f)\n\n", __FUNCTION__, __LINE__, newStartTimeSeconds, mPrevStartTimeSeconds, culled);
				aamp->UpdateCullingState(culled);
			}
			else
			{
				logprintf("PrivateStreamAbstractionMPD::%s:%d newStartTimeSeconds %f mPrevStartTimeSeconds %F\n", __FUNCTION__, __LINE__, newStartTimeSeconds, mPrevStartTimeSeconds);
			}
			mPrevStartTimeSeconds = newStartTimeSeconds;
		}
	}
	else
	{
		logprintf("PrivateStreamAbstractionMPD::%s:%d NULL adaptationset\n", __FUNCTION__, __LINE__);
	}
}

/**
 * @brief Fetch and inject initialization fragment
 *
 * @param[in]  discontinuity  True if discontinuous fragment
 */
void PrivateStreamAbstractionMPD::FetchAndInjectInitialization(bool discontinuity)
{
	pthread_t trackDownloadThreadID;
	HeaderFetchParams *fetchParams = NULL;
	bool dlThreadCreated = false;
	int numberOfTracks = mNumberOfTracks;
	for (int i = 0; i < numberOfTracks; i++)
	{
		struct MediaStreamContext *pMediaStreamContext = mMediaStreamContext[i];
		if(pMediaStreamContext->enabled && pMediaStreamContext->profileChanged)
		{
			if (pMediaStreamContext->adaptationSet)
			{
				ISegmentTemplate *segmentTemplate = pMediaStreamContext->adaptationSet->GetSegmentTemplate();

				//XRE-12249: SegmentTemplate can be a sub-node of Representation
				if(!segmentTemplate)
				{
					segmentTemplate = pMediaStreamContext->representation->GetSegmentTemplate();
				}
				//XRE-12249: End of Fix

				if (segmentTemplate)
				{
					std::string initialization = segmentTemplate->Getinitialization();
					if (!initialization.empty())
					{
						double fragmentDuration = 0.0;
						/*
						 * This block is added to download the initialization tracks in parallel
						 * to reduce the tune time, especially when using DRM.
						 * Moving the fragment download of first AAMPTRACK to separate thread
						 */
						if(!dlThreadCreated)
						{
							fetchParams = new HeaderFetchParams();
							fetchParams->context = this;
							fetchParams->fragmentduration = fragmentDuration;
							fetchParams->initialization = initialization;
							fetchParams->isinitialization = true;
							fetchParams->pMediaStreamContext = pMediaStreamContext;
							fetchParams->discontinuity = discontinuity;
							int ret = pthread_create(&trackDownloadThreadID, NULL, TrackDownloader, fetchParams);
							if(ret != 0)
							{
								logprintf("PrivateStreamAbstractionMPD::%s:%d pthread_create failed for TrackDownloader with errno = %d, %s\n", __FUNCTION__, __LINE__, errno, strerror(errno));
								delete fetchParams;
							}
							else
							{
								dlThreadCreated = true;
							}
						}
						else
						{
							if(pMediaStreamContext->WaitForFreeFragmentAvailable(0))
							{
								pMediaStreamContext->profileChanged = false;
								FetchFragment(pMediaStreamContext, initialization, fragmentDuration,true, (eMEDIATYPE_AUDIO == i), discontinuity);
							}
						}
					}
				}
				else
				{
					ISegmentBase *segmentBase = pMediaStreamContext->representation->GetSegmentBase();
					if (segmentBase)
					{
						pMediaStreamContext->fragmentOffset = 0;
						if (pMediaStreamContext->index_ptr)
						{
							aamp_Free(&pMediaStreamContext->index_ptr);
						}
						const IURLType *urlType = segmentBase->GetInitialization();
						if (urlType)
						{
							std::string range = urlType->GetRange();
							int start, fin;
							sscanf(range.c_str(), "%d-%d", &start, &fin);
#ifdef DEBUG_TIMELINE
							logprintf("init %s %d..%d\n", mMediaTypeName[pMediaStreamContext->mediaType], start, fin);
#endif
							char fragmentUrl[MAX_URI_LENGTH];
							GetFragmentUrl(fragmentUrl, &pMediaStreamContext->fragmentDescriptor, "");
							if(pMediaStreamContext->WaitForFreeFragmentAvailable(0))
							{
								pMediaStreamContext->profileChanged = false;
								if(!pMediaStreamContext->CacheFragment(fragmentUrl, 0, pMediaStreamContext->fragmentTime, 0, range.c_str(), true ))
								{
									logprintf("PrivateStreamAbstractionMPD::%s:%d failed. fragmentUrl %s fragmentTime %f\n", __FUNCTION__, __LINE__, fragmentUrl, pMediaStreamContext->fragmentTime);
								}
							}
						}
						else
						{
							std::string irange = segmentBase->GetIndexRange();
							int start,stop;
							if(sscanf(irange.c_str(), "%d-%d", &start, &stop)==2)
							{
								stop = start-1;
								start = 0;
								char range[10];
								snprintf(range, 9, "%d-%d", start, stop);
								range[9] = '\0';
								char fragmentUrl[MAX_URI_LENGTH];
								GetFragmentUrl(fragmentUrl, &pMediaStreamContext->fragmentDescriptor, "");
								if(pMediaStreamContext->WaitForFreeFragmentAvailable(0))
								{
									pMediaStreamContext->profileChanged = false;
									if(!pMediaStreamContext->CacheFragment(fragmentUrl, 0, pMediaStreamContext->fragmentTime, 0, range, true ))
									{
										logprintf("PrivateStreamAbstractionMPD::%s:%d failed. fragmentUrl %s fragmentTime %f\n", __FUNCTION__, __LINE__, fragmentUrl, pMediaStreamContext->fragmentTime);
									}
								}
							}
							else
							{
								logprintf("%s:%d sscanf failed\n", __FUNCTION__, __LINE__);
							}
						}
					}
					else
					{
						ISegmentList *segmentList = pMediaStreamContext->representation->GetSegmentList();
						if (segmentList)
						{
							std::string initialization = segmentList->GetInitialization()->GetSourceURL();
							if (!initialization.empty())
							{
								double fragmentDuration = 0.0;
								/*
								 * This block is added to download the initialization tracks in parallel
								 * to reduce the tune time, especially when using DRM.
								 * Moving the fragment download of first AAMPTRACK to separate thread
								 */
								if(!dlThreadCreated)
								{
									fetchParams = new HeaderFetchParams();
									fetchParams->context = this;
									fetchParams->fragmentduration = fragmentDuration;
									fetchParams->initialization = initialization;
									fetchParams->isinitialization = true;
									fetchParams->pMediaStreamContext = pMediaStreamContext;
									int ret = pthread_create(&trackDownloadThreadID, NULL, TrackDownloader, fetchParams);
									if(ret != 0)
									{
										logprintf("PrivateStreamAbstractionMPD::%s:%d pthread_create failed for TrackDownloader with errno = %d, %s\n", __FUNCTION__, __LINE__, errno, strerror(errno));
										delete fetchParams;
									}
									else
									{
										dlThreadCreated = true;
									}
								}
								else
								{
									if(pMediaStreamContext->WaitForFreeFragmentAvailable(0))
									{
										pMediaStreamContext->profileChanged = false;
										FetchFragment(pMediaStreamContext, initialization, fragmentDuration,true, (eMEDIATYPE_AUDIO == i));
									}
								}
							}
							else
							{
								string range;
#ifdef LIBDASH_SEGMENTLIST_GET_INIT_SUPPORT
								const ISegmentURL *segmentURL = NULL;
								segmentURL = segmentList->Getinitialization();

								if (segmentURL)
								{
									range = segmentURL->GetMediaRange();
								}
#else
								const std::vector<ISegmentURL*> segmentURLs = segmentList->GetSegmentURLs();
								if (segmentURLs.size() > 0)
								{
									ISegmentURL *firstSegmentURL = segmentURLs.at(0);
									int start, fin;
									const char *firstSegmentRange = firstSegmentURL->GetMediaRange().c_str();
									AAMPLOG_INFO("firstSegmentRange %s [%s]\n",
											mMediaTypeName[pMediaStreamContext->mediaType], firstSegmentRange);
									if (sscanf(firstSegmentRange, "%d-%d", &start, &fin) == 2)
									{
										if (start > 1)
										{
											char range_c[64];
											sprintf(range_c, "%d-%d", 0, start - 1);
											range = range_c;
										}
										else
										{
											logprintf("PrivateStreamAbstractionMPD::%s:%d segmentList - cannot determine range for Initialization - first segment start %d\n",
													__FUNCTION__, __LINE__, start);
										}
									}
								}
#endif
								if (!range.empty())
								{
									char fragmentUrl[MAX_URI_LENGTH];
									GetFragmentUrl(fragmentUrl, &pMediaStreamContext->fragmentDescriptor, "");
									AAMPLOG_INFO("%s [%s]\n", mMediaTypeName[pMediaStreamContext->mediaType],
											range.c_str());
									if(pMediaStreamContext->WaitForFreeFragmentAvailable(0))
									{
										pMediaStreamContext->profileChanged = false;
										if(!pMediaStreamContext->CacheFragment(fragmentUrl, 0, pMediaStreamContext->fragmentTime, 0.0, range.c_str(), true ))
										{
											logprintf("PrivateStreamAbstractionMPD::%s:%d failed. fragmentUrl %s fragmentTime %f\n", __FUNCTION__, __LINE__, fragmentUrl, pMediaStreamContext->fragmentTime);
										}
									}
								}
								else
								{
									logprintf("PrivateStreamAbstractionMPD::%s:%d segmentList - empty range string for Initialization\n",
											__FUNCTION__, __LINE__);
								}
							}
						}
						else
						{
							aamp_Error("not-yet-supported mpd format");
						}
					}
				}
			}
		}
	}

	if(dlThreadCreated)
	{
		AAMPLOG_TRACE("Waiting for pthread_join trackDownloadThread\n");
		pthread_join(trackDownloadThreadID, NULL);
		AAMPLOG_TRACE("Joined trackDownloadThread\n");
		delete fetchParams;
	}
}


/**
 * @brief Check if current period is clear
 *
 * @retval True if clear period
 */
bool PrivateStreamAbstractionMPD::CheckForInitalClearPeriod()
{
	bool ret = true;
	vector<IDescriptor*> contentProt;
	for(int i = 0; i < mNumberOfTracks; i++)
	{
		contentProt = mMediaStreamContext[i]->adaptationSet->GetContentProtection();
		if(0 != contentProt.size())
		{
			ret = false;
			break;
		}
	}
	if(ret)
	{
		logprintf("%s %d Initial period is clear period, trying work around\n",__FUNCTION__,__LINE__);
	}
	return ret;
}

/**
 * @brief Push encrypted headers if available
 */
void PrivateStreamAbstractionMPD::PushEncryptedHeaders()
{
	//Find the first period with contentProtection
	size_t numPeriods = mpd->GetPeriods().size();
	int headerCount = 0;
	for(int i = mNumberOfTracks - 1; i >= 0; i--)
	{
		bool encryptionFound = false;
		unsigned iPeriod = 0;
		while(iPeriod < numPeriods && !encryptionFound)
		{
			IPeriod *period = mpd->GetPeriods().at(iPeriod);
			size_t numAdaptationSets = period->GetAdaptationSets().size();
			for(unsigned iAdaptationSet = 0; iAdaptationSet < numAdaptationSets && !encryptionFound; iAdaptationSet++)
			{
				IAdaptationSet *adaptationSet = period->GetAdaptationSets().at(iAdaptationSet);
				if (IsContentType(adaptationSet, (MediaType)i ))
				{
					vector<IDescriptor*> contentProt;
					contentProt = adaptationSet->GetContentProtection();
					if(0 == contentProt.size())
					{
						continue;
					}
					else
					{
						ISegmentTemplate *segmentTemplate = adaptationSet->GetSegmentTemplate();
						if (segmentTemplate)
						{
							std::string initialization = segmentTemplate->Getinitialization();
							if (!initialization.empty())
							{
								char fragmentUrl[MAX_URI_LENGTH];
								struct FragmentDescriptor * fragmentDescriptor = (struct FragmentDescriptor *) malloc(sizeof(struct FragmentDescriptor));
								memset(fragmentDescriptor, 0, sizeof(FragmentDescriptor));
								fragmentDescriptor->manifestUrl = mMediaStreamContext[eMEDIATYPE_VIDEO]->fragmentDescriptor.manifestUrl;
								IRepresentation *representation = NULL;
								size_t representionIndex = 0;
								if(MediaType(i) == eMEDIATYPE_VIDEO)
								{
									size_t representationCount = adaptationSet->GetRepresentation().size();
									if(adaptationSet->GetRepresentation().at(representionIndex)->GetBandwidth() > adaptationSet->GetRepresentation().at(representationCount - 1)->GetBandwidth())
									{
										representionIndex = representationCount - 1;
									}
								}
								else if (mAudioType != eAUDIO_UNKNOWN)
								{
									AudioType selectedAudioType = eAUDIO_UNKNOWN;
									representionIndex = GetDesiredCodecIndex(adaptationSet, selectedAudioType);
									if(selectedAudioType != mAudioType)
									{
										continue;
									}
									logprintf("%s %d Audio type %d\n", __FUNCTION__, __LINE__, selectedAudioType);
								}
								else
								{
									logprintf("%s %d Audio type eAUDIO_UNKNOWN\n", __FUNCTION__, __LINE__);
								}
								ProcessContentProtection(adaptationSet, (MediaType)i);
								representation = adaptationSet->GetRepresentation().at(representionIndex);
								fragmentDescriptor->Bandwidth = representation->GetBandwidth();
								fragmentDescriptor->baseUrls = &representation->GetBaseURLs();
								if (fragmentDescriptor->baseUrls->size() == 0)
								{
									fragmentDescriptor->baseUrls = &adaptationSet->GetBaseURLs();
									if (fragmentDescriptor->baseUrls->size() == 0)
									{
										fragmentDescriptor->baseUrls = &period->GetBaseURLs();
										if (fragmentDescriptor->baseUrls->size() == 0)
										{
											fragmentDescriptor->baseUrls = &mpd->GetBaseUrls();
										}
									}
								}
								strcpy(fragmentDescriptor->RepresentationID, representation->GetId().c_str());
								GetFragmentUrl(fragmentUrl,fragmentDescriptor , initialization);
								if (mMediaStreamContext[i]->WaitForFreeFragmentAvailable())
								{
									logprintf("%s %d Pushing encrypted header for %s\n", __FUNCTION__, __LINE__, mMediaTypeName[i]);
									mMediaStreamContext[i]->CacheFragment(fragmentUrl, i, mMediaStreamContext[i]->fragmentTime, 0.0, NULL, true);
								}
								free(fragmentDescriptor);
								encryptionFound = true;
							}
						}
					}
				}
			}
			iPeriod++;
		}
	}
}

/**
 * @brief Fetches and caches fragments in a loop
 */
void PrivateStreamAbstractionMPD::FetcherLoop()
{
	bool exitFetchLoop = false;
	bool trickPlay = (AAMP_NORMAL_PLAY_RATE != rate);
	bool mpdChanged = false;
	double delta = 0;
	bool lastLiveFlag=false;
#ifdef AAMP_MPD_DRM
	if (mPushEncInitFragment && CheckForInitalClearPeriod())
	{
		PushEncryptedHeaders();
	}
	mPushEncInitFragment = false;
#endif

	logprintf("PrivateStreamAbstractionMPD::%s:%d - fetch initialization fragments\n", __FUNCTION__, __LINE__);
	FetchAndInjectInitialization();
	IPeriod *currPeriod = mpd->GetPeriods().at(mCurrentPeriodIdx);
	mPeriodId = currPeriod->GetId();
	mPrevAdaptationSetCount = currPeriod->GetAdaptationSets().size();
	logprintf("aamp: ready to collect fragments. mpd %p\n", mpd);
	do
	{
		bool liveMPDRefresh = false;
		if (mpd)
		{
			mIsLive = !(mpd->GetType() == "static");
			aamp->SetIsLive(mIsLive);
			size_t numPeriods = mpd->GetPeriods().size();
			unsigned iPeriod = mCurrentPeriodIdx;
			logprintf("MPD has %d periods current period index %d\n", numPeriods, mCurrentPeriodIdx);
			while(iPeriod < numPeriods && iPeriod >= 0 && !exitFetchLoop)
			{
				bool periodChanged = (iPeriod != mCurrentPeriodIdx);
				if (periodChanged || mpdChanged)
				{
					bool discontinuity = false;
					bool requireStreamSelection = false;
					uint64_t nextSegmentTime = mMediaStreamContext[eMEDIATYPE_VIDEO]->fragmentDescriptor.Time;
					mpdChanged = false;
					if (periodChanged)
					{
						mCurrentPeriodIdx = iPeriod;
						//We are moving to new period so reset the lastsegment time
						//for VOD and cDVR
						logprintf("%s:%d Period(%d/%d) IsLive(%d) IsCdvr(%d) \n",__FUNCTION__,__LINE__,
							mCurrentPeriodIdx,numPeriods,mIsLive,aamp->IsInProgressCDVR());
						for (int i = 0; i < mNumberOfTracks; i++)
						{
							mMediaStreamContext[i]->lastSegmentTime = 0;
						}
					}

					IPeriod * period = mpd->GetPeriods().at(mCurrentPeriodIdx);
					vector <IAdaptationSet*> adapatationSets = period->GetAdaptationSets();
					int adaptationSetCount = adapatationSets.size();
					if(0 == adaptationSetCount)
					{
						/*To Handle non fog scenarios where empty periods are
						* present after mpd update causing issues (DELIA-29879)
						*/
						iPeriod++;
						continue;
					}
					if( mPeriodId != period->GetId())
					{
						logprintf("Period ID changed from \'%s\' to \'%s\'\n", mPeriodId.c_str(),period->GetId().c_str());
						mPeriodId = period->GetId();
						mPrevAdaptationSetCount = adaptationSetCount;
						logprintf("playing period %d/%d\n", iPeriod, (int)numPeriods);
						requireStreamSelection = true;
						periodChanged = true;
					}
					else if(mPrevAdaptationSetCount != adaptationSetCount)
					{
						logprintf("Change in AdaptationSet count; adaptationSetCount %d  mPrevAdaptationSetCount %d,updating stream selection\n", adaptationSetCount, mPrevAdaptationSetCount);
						mPrevAdaptationSetCount = adaptationSetCount;
						requireStreamSelection = true;
					}
					else
					{
						for (int i = 0; i < mNumberOfTracks; i++)
						{
							if(mMediaStreamContext[i]->adaptationSetId != adapatationSets.at(mMediaStreamContext[i]->adaptationSetIdx)->GetId())
							{
								logprintf("AdaptationSet index changed; updating stream selection\n");
								requireStreamSelection = true;
							}
						}
					}

					if(requireStreamSelection)
					{
						StreamSelection();
					}
					// IsLive = 1 , resetTimeLineIndex = 1
					// InProgressCdvr (IsLive=1) , resetTimeLineIndex = 1
					// Vod/CDVR for PeriodChange , resetTimeLineIndex = 1
					bool resetTimeLineIndex = (mIsLive || lastLiveFlag|| periodChanged);
					UpdateTrackInfo(true, periodChanged, resetTimeLineIndex);
					if(mIsLive || lastLiveFlag)
					{
						uint64_t durationMs = GetDurationFromRepresentation();
						aamp->UpdateDuration(((double)durationMs)/1000);
					}
					lastLiveFlag = mIsLive;
					/*Discontinuity handling on period change*/
					if ( periodChanged && gpGlobalConfig->mpdDiscontinuityHandling && mMediaStreamContext[eMEDIATYPE_VIDEO]->enabled &&
							mMediaStreamContext[eMEDIATYPE_AUDIO] && mMediaStreamContext[eMEDIATYPE_AUDIO]->enabled &&
							(gpGlobalConfig->mpdDiscontinuityHandlingCdvr || (!aamp->IsInProgressCDVR())))
					{
						MediaStreamContext *pMediaStreamContext = mMediaStreamContext[eMEDIATYPE_VIDEO];
						ISegmentTemplate *segmentTemplate = pMediaStreamContext->adaptationSet->GetSegmentTemplate();
						if (!segmentTemplate)
						{
							segmentTemplate = pMediaStreamContext->representation->GetSegmentTemplate();
						}
						if (segmentTemplate)
						{
							uint64_t segmentStartTime = GetFirstSegmentStartTime(period);
							if (nextSegmentTime != segmentStartTime)
							{
								logprintf("PrivateStreamAbstractionMPD::%s:%d discontinuity detected nextSegmentTime %" PRIu64 " FirstSegmentStartTime %" PRIu64 " \n", __FUNCTION__, __LINE__, nextSegmentTime, segmentStartTime);
								discontinuity = true;
								mFirstPTS = (double)segmentStartTime/segmentTemplate->GetTimescale();
							}
							else
							{
								logprintf("PrivateStreamAbstractionMPD::%s:%d No discontinuity detected nextSegmentTime %" PRIu64 " FirstSegmentStartTime %" PRIu64 " \n", __FUNCTION__, __LINE__, nextSegmentTime, segmentStartTime);
							}
						}
						else
						{
							traceprintf("PrivateStreamAbstractionMPD::%s:%d Segment template not available\n", __FUNCTION__, __LINE__);
						}
					}
					FetchAndInjectInitialization(discontinuity);
					if(rate < 0)
					{
						SkipToEnd(mMediaStreamContext[eMEDIATYPE_VIDEO]);
					}
				}

				// playback
				while (!exitFetchLoop && !liveMPDRefresh)
				{
					bool bCacheFullState = true;
					for (int i = mNumberOfTracks-1; i >= 0; i--)
					{
						struct MediaStreamContext *pMediaStreamContext = mMediaStreamContext[i];
						if (pMediaStreamContext->adaptationSet )
						{
							if((pMediaStreamContext->numberOfFragmentsCached != gpGlobalConfig->maxCachedFragmentsPerTrack) && !(pMediaStreamContext->profileChanged))
							{	// profile not changed and Cache not full scenario
								if (!pMediaStreamContext->eos)
								{
									if(trickPlay)
									{
										if((rate > 0 && delta <= 0) || (rate < 0 && delta >= 0))
										{
											delta = rate / gpGlobalConfig->vodTrickplayFPS;
										}
										delta = SkipFragments(pMediaStreamContext, delta);
									}
									if(PushNextFragment(pMediaStreamContext,i))
									{
										if (mIsLive)
										{
											mContext->CheckForPlaybackStall(true);
										}
										if((!pMediaStreamContext->mContext->trickplayMode) && (eMEDIATYPE_VIDEO == i) && (!aamp->IsTSBSupported()))
										{
											if (aamp->CheckABREnabled())
											{
												pMediaStreamContext->mContext->CheckForProfileChange();
											}
											else
											{
												pMediaStreamContext->mContext->CheckUserProfileChangeReq();
											}
										}
									}
									else if (pMediaStreamContext->eos == true && mIsLive && i == eMEDIATYPE_VIDEO)
									{
										mContext->CheckForPlaybackStall(false);
									}
								}
							}
							// Fetch init header for both audio and video ,after mpd refresh(stream selection) , profileChanged = true for both tracks .
							// Need to reset profileChanged flag which is done inside FetchAndInjectInitialization
							// Without resetting profileChanged flag , fetch of audio was stopped causing audio drop
							// DELIA-32017
							else if(pMediaStreamContext->profileChanged)
							{	// Profile changed case
								FetchAndInjectInitialization();
							}

							if(pMediaStreamContext->numberOfFragmentsCached != gpGlobalConfig->maxCachedFragmentsPerTrack)
							{
								bCacheFullState = false;
							}

						}
						if (!aamp->DownloadsAreEnabled())
						{
							exitFetchLoop = true;
							bCacheFullState = false;
							break;
						}
					}// end of for loop
					// BCOM-2959  -- Exit from fetch loop for period to be done only after audio and video fetch
					// While playing CDVR with EAC3 audio , durations doesnt match and only video downloads are seen leaving audio behind
					// Audio cache is always full and need for data is not received for more fetch.
					// So after video downloads loop was exiting without audio fetch causing audio drop .
					// Now wait for both video and audio to reach EOS before moving to next period or exit.
					bool vEos = mMediaStreamContext[eMEDIATYPE_VIDEO]->eos;
					bool audioEnabled = (mMediaStreamContext[eMEDIATYPE_AUDIO] && mMediaStreamContext[eMEDIATYPE_AUDIO]->enabled);
					bool aEos = (audioEnabled && mMediaStreamContext[eMEDIATYPE_AUDIO]->eos);
					if (vEos || aEos)
					{
						if((!mIsLive || (rate != AAMP_NORMAL_PLAY_RATE)) && ((rate > 0 && mCurrentPeriodIdx >= (numPeriods -1)) || (rate < 0 && 0 == mCurrentPeriodIdx)))
						{
							if(vEos)
							{
								mMediaStreamContext[eMEDIATYPE_VIDEO]->eosReached = true;
								mMediaStreamContext[eMEDIATYPE_VIDEO]->AbortWaitForCachedFragment(false);
							}
							if(audioEnabled)
							{
								if(mMediaStreamContext[eMEDIATYPE_AUDIO]->eos)
								{
									mMediaStreamContext[eMEDIATYPE_AUDIO]->eosReached = true;
									mMediaStreamContext[eMEDIATYPE_AUDIO]->AbortWaitForCachedFragment(false);
								}
							}
							else
							{	// No Audio enabled , fake the flag to true
								aEos = true;
							}
						}
						else
						{
							if(!audioEnabled)
							{
								aEos = true;
							}
						}
						// If audio and video reached EOS then only break the fetch loop .
						if(vEos && aEos)
                                                {
							AAMPLOG_INFO("%s:%d EOS - Exit fetch loop \n",__FUNCTION__, __LINE__);
							break;
                                                }
					}


					if (mMediaStreamContext[eMEDIATYPE_VIDEO]->endTimeReached)
					{
						//TODO : Move this to inject loop to complete playback
						logprintf("%s:%d - endTimeReached\n", __FUNCTION__, __LINE__);
						aamp->EndTimeReached(eMEDIATYPE_VIDEO);
						break;
					}
					int timeoutMs =  MAX_DELAY_BETWEEN_MPD_UPDATE_MS - (int)(aamp_GetCurrentTimeMS() - mLastPlaylistDownloadTimeMs);
					if(timeoutMs <= 0 && mIsLive && rate > 0)
					{
						liveMPDRefresh = true;
						break;
					}
					else if(bCacheFullState)
					{
						// play cache is full , wait until cache is available to inject next, max wait of 1sec
						int timeoutMs = 200;
						AAMPLOG_TRACE("%s:%d Cache full state,no download until(%d) Time(%lld)\n",__FUNCTION__, __LINE__,timeoutMs,aamp_GetCurrentTimeMS());
						mMediaStreamContext[eMEDIATYPE_VIDEO]->WaitForFreeFragmentAvailable(timeoutMs);
					}
					else
					{	// This sleep will hit when there is no content to download and cache is not full
						// and refresh interval timeout not reached . To Avoid tight loop adding a min delay
						aamp->InterruptableMsSleep(50);
					}
				}// end of while loop (!exitFetchLoop && !liveMPDRefresh)
				if(liveMPDRefresh)
				{
					break;
				}
				if(rate > 0)
				{
					iPeriod++;
				}
				else
				{
					iPeriod--;
				}
			}//End of Period while loop

			if ((rate < AAMP_NORMAL_PLAY_RATE && iPeriod < 0) || (rate > 1 && iPeriod >= numPeriods) || mpd->GetType() == "static")
			{
				break;
			}
			else
			{
				traceprintf("PrivateStreamAbstractionMPD::%s:%d Refresh playlist\n", __FUNCTION__, __LINE__);
			}
		}
		else
		{
			logprintf("PrivateStreamAbstractionMPD::%s:%d - null mpd\n", __FUNCTION__, __LINE__);
		}

		// If it comes here , two reason a) Reached eos b) Need livempdUpdate
		// If liveMPDRefresh is true , that means it already reached 6 sec timeout .
		// 		No more further delay required for mpd update .
		// If liveMPDRefresh is false, then it hit eos . Here the timeout is calculated based
		// on the buffer availability.
		if (!liveMPDRefresh && mLastPlaylistDownloadTimeMs)
		{
			int minDelayBetweenPlaylistUpdates = (int)mMinUpdateDurationMs;
			int timeSinceLastPlaylistDownload = (int)(aamp_GetCurrentTimeMS() - mLastPlaylistDownloadTimeMs);
			//long bufferAvailable = ((long)(mMediaStreamContext[eMEDIATYPE_VIDEO]->targetDnldPosition*1000) - (long)aamp->GetPositionMs());
			long long currentPlayPosition = aamp->GetPositionMs();
			long long endPositionAvailable = (aamp->culledSeconds + aamp->durationSeconds)*1000;
			// playTarget value will vary if TSB is full and trickplay is attempted. Cant use for buffer calculation
			// So using the endposition in playlist - Current playing position to get the buffer availability
			long bufferAvailable = (endPositionAvailable - currentPlayPosition);

			// If buffer Available is > 2*mMinUpdateDurationMs
			if(bufferAvailable  > (mMinUpdateDurationMs*2) )
			{
				// may be 1.0 times also can be set ???
				minDelayBetweenPlaylistUpdates = (int)(1.5 * mMinUpdateDurationMs);
			}
			// if buffer is between 2*target & mMinUpdateDurationMs
			else if(bufferAvailable  > mMinUpdateDurationMs)
			{
				minDelayBetweenPlaylistUpdates = (int)(0.5 * mMinUpdateDurationMs);
			}
			// This is to handle the case where target duration is high value(>Max delay)  but buffer is available just above the max update inteval
			else if(bufferAvailable > (2*MAX_DELAY_BETWEEN_MPD_UPDATE_MS))
			{
				minDelayBetweenPlaylistUpdates = MAX_DELAY_BETWEEN_MPD_UPDATE_MS;
			}
			// if buffer < targetDuration && buffer < MaxDelayInterval
			else
			{
				// if bufferAvailable is less than targetDuration ,its in RED alert . Close to freeze
				// need to refresh soon ..
				if(bufferAvailable)
				{
					minDelayBetweenPlaylistUpdates = (int)(bufferAvailable / 3) ;
				}
				else
				{
					minDelayBetweenPlaylistUpdates = MIN_DELAY_BETWEEN_MPD_UPDATE_MS; // 500mSec
				}
				// limit the logs when buffer is low
				{
					static int bufferlowCnt;
					if((bufferlowCnt++ & 5) == 0)
					{
						logprintf("Buffer is running low(%ld).Refreshing playlist(%d).PlayPosition(%lld) End(%lld)\n",
							bufferAvailable,minDelayBetweenPlaylistUpdates,currentPlayPosition,endPositionAvailable);
					}
				}

			}

			// First cap max limit ..
			// remove already consumed time from last update
			// if time interval goes negative, limit to min value

			// restrict to Max delay interval
			if (minDelayBetweenPlaylistUpdates > MAX_DELAY_BETWEEN_MPD_UPDATE_MS)
			{
				minDelayBetweenPlaylistUpdates = MAX_DELAY_BETWEEN_MPD_UPDATE_MS;
			}

			// adjust with last refreshed time interval
			minDelayBetweenPlaylistUpdates -= timeSinceLastPlaylistDownload;

			if(minDelayBetweenPlaylistUpdates < MIN_DELAY_BETWEEN_MPD_UPDATE_MS)
			{
				// minimum of 500 mSec needed to avoid too frequent download.
				minDelayBetweenPlaylistUpdates = MIN_DELAY_BETWEEN_MPD_UPDATE_MS;
			}

			AAMPLOG_INFO("aamp playlist end refresh bufferMs(%ld) delay(%d) delta(%d) End(%lld) PlayPosition(%lld)\n",
				bufferAvailable,minDelayBetweenPlaylistUpdates,timeSinceLastPlaylistDownload,endPositionAvailable,currentPlayPosition);

			// sleep before next manifest update
			aamp->InterruptableMsSleep(minDelayBetweenPlaylistUpdates);
		}
		if (UpdateMPD() != eAAMPSTATUS_OK)
		{
			break;
		}

		if(mIsFogTSB)
		{
			//Periods could be added or removed, So select period based on periodID
			//If period ID not found in MPD that means got culled, in that case select
			// first period
			logprintf("Updataing period index after mpd refresh\n");
			vector<IPeriod *> periods = mpd->GetPeriods();
			int iter = periods.size() - 1;
			mCurrentPeriodIdx = 0;
			while(iter > 0)
			{
				if(mPeriodId == periods.at(iter)->GetId())
				{
					mCurrentPeriodIdx = iter;
					break;
				}
				iter--;
			}
		}
		else
		{
			// DELIA-31750 - looping of cdvr video - Issue happens with multiperiod content only
			// When playback is near live position (last period) or after eos in period
			// mCurrentPeriodIdx was resetted to 0 . This caused fetch loop to continue from Period 0/fragement 1
			// Reset of mCurrentPeriodIdx to be done to max period if Period count changes after mpd refresh
			size_t newPeriods = mpd->GetPeriods().size();
			if(mCurrentPeriodIdx > (newPeriods - 1))
			{
				logprintf("MPD Fragment Collector detected reset in Period(New Size:%d)(currentIdx:%d->%d)\n",
					newPeriods,mCurrentPeriodIdx,newPeriods - 1);
				mCurrentPeriodIdx = newPeriods - 1;
			}
		}
		mpdChanged = true;
	}
	while (!exitFetchLoop);
	logprintf("MPD fragment collector done\n");
}


/**
 * @brief StreamAbstractionAAMP_MPD Constructor
 *
 * @param[in] aamp      Pointer to PrivateInstanceAAMP object associated with player
 * @param[in] seek_pos  Seek position
 * @param[in] rate      Playback rate
 */
StreamAbstractionAAMP_MPD::StreamAbstractionAAMP_MPD(class PrivateInstanceAAMP *aamp,double seek_pos, float rate): StreamAbstractionAAMP(aamp)
{
	mPriv = new PrivateStreamAbstractionMPD( this, aamp, seek_pos, rate);
	trickplayMode = (rate != AAMP_NORMAL_PLAY_RATE);
}


/**
 * @brief StreamAbstractionAAMP_MPD Destructor
 */
StreamAbstractionAAMP_MPD::~StreamAbstractionAAMP_MPD()
{
	delete mPriv;
}


/**
 *   @brief  Set a position at which stop injection .
 *
 *   @param[in]  endPosition  Playback end position.
 */
void StreamAbstractionAAMP_MPD::SetEndPos(double endPosition)
{
	mPriv->SetEndPos(endPosition);
}


/**
 *   @brief  Set a position at which stop injection .
 *
 *   @param[in]  endPosition   Playback end position.
 */
void PrivateStreamAbstractionMPD::SetEndPos(double endPosition)
{
	mEndPosition = endPosition;
}


/**
 *   @brief  Starts streaming.
 */
void StreamAbstractionAAMP_MPD::Start(void)
{
	mPriv->Start();
}


/**
 *   @brief  Starts streaming.
 */
void PrivateStreamAbstractionMPD::Start(void)
{
#ifdef AAMP_MPD_DRM
	AampDRMSessionManager::setSessionMgrState(SessionMgrState::eSESSIONMGR_ACTIVE);
#endif
	pthread_create(&fragmentCollectorThreadID, NULL, &FragmentCollector, this);
	fragmentCollectorThreadStarted = true;
	for (int i=0; i< mNumberOfTracks; i++)
	{
		// DELIA-30608 - Right place to update targetDnldPosition.
		// Here GetPosition will give updated seek position (for live)
		mMediaStreamContext[i]->targetDnldPosition= aamp->GetPositionMs()/1000;
		mMediaStreamContext[i]->StartInjectLoop();
	}
}

/**
*   @brief  Stops streaming.
*
*   @param[in]  clearChannelData  Ignored.
*/
void StreamAbstractionAAMP_MPD::Stop(bool clearChannelData)
{
	aamp->DisableDownloads();
	ReassessAndResumeAudioTrack(true);
	mPriv->Stop();
	aamp->EnableDownloads();
}


/**
*   @brief  Stops streaming.
*/
void PrivateStreamAbstractionMPD::Stop()
{
	for (int iTrack = 0; iTrack < mNumberOfTracks; iTrack++)
	{
		MediaStreamContext *track = mMediaStreamContext[iTrack];
		if(track && track->Enabled())
		{
			track->AbortWaitForCachedFragment(true);
			track->StopInjectLoop();
		}
	}
	if(drmSessionThreadStarted)
	{
		void *value_ptr = NULL;
		AAMPLOG_INFO("Waiting to join CreateDRMSession thread\n");
		int rc = pthread_join(createDRMSessionThreadID, &value_ptr);
		if (rc != 0)
		{
			logprintf("pthread_join returned %d for createDRMSession Thread\n", rc);
		}
		AAMPLOG_INFO("Joined CreateDRMSession thread\n");
		drmSessionThreadStarted = false;
	}
	if(fragmentCollectorThreadStarted)
	{
		void *value_ptr = NULL;
		int rc = pthread_join(fragmentCollectorThreadID, &value_ptr);
		if (rc != 0)
		{
			logprintf("***pthread_join returned %d\n", rc);
		}
		fragmentCollectorThreadStarted = false;
	}
	aamp->mStreamSink->ClearProtectionEvent();
  #ifdef AAMP_MPD_DRM
	AampDRMSessionManager::setSessionMgrState(SessionMgrState::eSESSIONMGR_INACTIVE);
	AampDRMSessionManager::clearFailedKeyIds();
  #endif
}

/**
 * @brief PrivateStreamAbstractionMPD Destructor
 */
PrivateStreamAbstractionMPD::~PrivateStreamAbstractionMPD(void)
{
	for (int iTrack = 0; iTrack < mNumberOfTracks; iTrack++)
	{
		MediaStreamContext *track = mMediaStreamContext[iTrack];
		if(track )
		{
			delete track;
		}
	}

	if(lastProcessedKeyId)
	{
		free(lastProcessedKeyId);
	}

	aamp->SyncBegin();
	if (mpd)
	{
		delete mpd;
		mpd = NULL;
	}

	if(mStreamInfo)
	{
		delete[] mStreamInfo;
	}

	aamp->CurlTerm(0, AAMP_TRACK_COUNT);

	aamp->SyncEnd();
}


/**
 * @brief Stub implementation
 */
void StreamAbstractionAAMP_MPD::DumpProfiles(void)
{ // STUB
}


/**
 *   @brief Get output format of stream.
 *
 *   @param[out]  primaryOutputFormat  Format of primary track
 *   @param[out]  audioOutputFormat    Format of audio track
 */
void PrivateStreamAbstractionMPD::GetStreamFormat(StreamOutputFormat &primaryOutputFormat, StreamOutputFormat &audioOutputFormat)
{
	if(mMediaStreamContext[eMEDIATYPE_VIDEO] && mMediaStreamContext[eMEDIATYPE_VIDEO]->enabled )
	{
		primaryOutputFormat = FORMAT_ISO_BMFF;
	}
	else
	{
		primaryOutputFormat = FORMAT_NONE;
	}
	if(mMediaStreamContext[eMEDIATYPE_AUDIO] && mMediaStreamContext[eMEDIATYPE_AUDIO]->enabled )
	{
		audioOutputFormat = FORMAT_ISO_BMFF;
	}
	else
	{
		audioOutputFormat = FORMAT_NONE;
	}
}


/**
 * @brief Get output format of stream.
 *
 * @param[out]  primaryOutputFormat  Format of primary track
 * @param[out]  audioOutputFormat    Format of audio track
 */
void StreamAbstractionAAMP_MPD::GetStreamFormat(StreamOutputFormat &primaryOutputFormat, StreamOutputFormat &audioOutputFormat)
{
	mPriv->GetStreamFormat(primaryOutputFormat, audioOutputFormat);
}


/**
 *   @brief Return MediaTrack of requested type
 *
 *   @param[in]  type  Track type
 *
 *   @retval MediaTrack pointer.
 */
MediaTrack* StreamAbstractionAAMP_MPD::GetMediaTrack(TrackType type)
{
	return mPriv->GetMediaTrack(type);
}


/**
 *   @brief Return MediaTrack of requested type
 *
 *   @param[in]  type  Track type
 *
 *   @retval MediaTrack pointer.
 */
MediaTrack* PrivateStreamAbstractionMPD::GetMediaTrack(TrackType type)
{
	return mMediaStreamContext[type];
}


/**
 * @brief Get current stream position.
 *
 * @retval Current position of stream.
 */
double StreamAbstractionAAMP_MPD::GetStreamPosition()
{
	return mPriv->GetStreamPosition();
}

/**
 * @brief Gets number of profiles
 *
 * @retval number of profiles
 */
int PrivateStreamAbstractionMPD::GetProfileCount()
{
	int ret = 0;
	if(mIsFogTSB)
	{
		ret = mBitrateIndexMap.size();
	}
	else
	{
		ret = mContext->GetProfileCount();
	}
	return ret;
}

/**
 *   @brief Get stream information of a profile from subclass.
 *
 *   @param[in]  idx  Profile index.
 *
 *   @retval Stream information corresponding to index.
 */
StreamInfo* PrivateStreamAbstractionMPD::GetStreamInfo(int idx)
{
	assert(idx < GetProfileCount());
	return &mStreamInfo[idx];
}


/**
 *   @brief Get stream information of a profile from subclass.
 *
 *   @param[in]  idx  Profile index.
 *
 *   @retval Stream information corresponding to index.
 */
StreamInfo* StreamAbstractionAAMP_MPD::GetStreamInfo(int idx)
{
	return mPriv->GetStreamInfo(idx);
}


/**
 *   @brief  Get PTS of first sample.
 *
 *   @retval PTS of first sample
 */
double StreamAbstractionAAMP_MPD::GetFirstPTS()
{
	return mPriv->GetFirstPTS();
}


/**
 *   @brief  Get PTS of first sample.
 *
 *   @retval PTS of first sample
 */
double PrivateStreamAbstractionMPD::GetFirstPTS()
{
	return mFirstPTS;
}


/**
 * @brief Get index corresponding to bitrate
 *
 * @param[in] bitrate Stream's bitrate
 *
 * @retval Bandwidth index
 */
int PrivateStreamAbstractionMPD::GetBWIndex(long bitrate)
{
	int topBWIndex = 0;
	int profileCount = GetProfileCount();
	if (profileCount)
	{
		for (int i = 0; i < profileCount; i++)
		{
			StreamInfo *streamInfo = &mStreamInfo[i];
			if (!streamInfo->isIframeTrack && streamInfo->bandwidthBitsPerSecond > bitrate)
			{
				--topBWIndex;
			}
		}
	}
	return topBWIndex;
}


/**
 * @brief Get index of profile corresponds to bandwidth
 *
 * @param[in] bitrate Bitrate to lookup profile
 *
 * @retval profile index
 */
int StreamAbstractionAAMP_MPD::GetBWIndex(long bitrate)
{
	return mPriv->GetBWIndex(bitrate);
}


/**
 * @brief To get the available video bitrates.
 *
 * @return available video bitrates
 */
std::vector<long> PrivateStreamAbstractionMPD::GetVideoBitrates(void)
{
	std::vector<long> bitrates;
	int profileCount = GetProfileCount();
	bitrates.reserve(profileCount);
	if (profileCount)
	{
		for (int i = 0; i < profileCount; i++)
		{
			StreamInfo *streamInfo = &mStreamInfo[i];
			if (!streamInfo->isIframeTrack)
			{
				bitrates.push_back(streamInfo->bandwidthBitsPerSecond);
			}
		}
	}
	return bitrates;
}


/**
 * @brief To get the available audio bitrates.
 *
 * @return  Available audio bitrates
 */
std::vector<long> PrivateStreamAbstractionMPD::GetAudioBitrates(void)
{
	//TODO: Impl getter for audio bitrates
	return std::vector<long>();
}


/**
 * @brief To get the available video bitrates.
 *
 * @return  Available video bitrates
 */
std::vector<long> StreamAbstractionAAMP_MPD::GetVideoBitrates(void)
{
	return mPriv->GetVideoBitrates();
}


/**
 * @brief To get the available audio bitrates.
 * @ret available audio bitrates
 */
std::vector<long> StreamAbstractionAAMP_MPD::GetAudioBitrates(void)
{
	return mPriv->GetAudioBitrates();
}


/**
*   @brief  Stops injecting fragments to StreamSink.
*/
void StreamAbstractionAAMP_MPD::StopInjection(void)
{
	//invoked at times of discontinuity. Audio injection loop might have already exited here
	ReassessAndResumeAudioTrack(true);
	mPriv->StopInjection();
}


/**
*   @brief  Stops injection.
*/
void PrivateStreamAbstractionMPD::StopInjection(void)
{
	for (int iTrack = 0; iTrack < mNumberOfTracks; iTrack++)
	{
		MediaStreamContext *track = mMediaStreamContext[iTrack];
		if(track && track->Enabled())
		{
			track->AbortWaitForCachedFragment(true);
			track->StopInjectLoop();
		}
	}
}


/**
*   @brief  Start injecting fragments to StreamSink.
*/
void StreamAbstractionAAMP_MPD::StartInjection(void)
{
	abortWait = false;
	mPriv->StartInjection();
}


/**
*   @brief  Start injection.
*/
void PrivateStreamAbstractionMPD::StartInjection(void)
{
	for (int iTrack = 0; iTrack < mNumberOfTracks; iTrack++)
	{
		MediaStreamContext *track = mMediaStreamContext[iTrack];
		if(track && track->Enabled())
		{
			track->StartInjectLoop();
		}
	}
}

/**
 * @}
 */
