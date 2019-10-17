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
 * @file fragmentcollector_mpd.h
 * @brief Fragment collector MPEG DASH declarations
 */

#include "StreamAbstractionAAMP.h"
#include <string>
#include <stdint.h>

/**
 * @addtogroup AAMP_COMMON_TYPES
 * @{
 */

/**
 * @class StreamAbstractionAAMP_MPD
 * @brief Fragment collector for MPEG DASH
 */
class StreamAbstractionAAMP_MPD : public StreamAbstractionAAMP
{
public:
	StreamAbstractionAAMP_MPD(class PrivateInstanceAAMP *aamp,double seekpos, float rate);
	~StreamAbstractionAAMP_MPD();
	void DumpProfiles(void);
	void SetEndPos(double endPosition);
	void Start();
	void Stop(bool clearChannelData);
	AAMPStatusType Init(TuneType tuneType);
	void GetStreamFormat(StreamOutputFormat &primaryOutputFormat, StreamOutputFormat &audioOutputFormat);
	double GetStreamPosition();
	MediaTrack* GetMediaTrack(TrackType type);
	double GetFirstPTS();
	int GetBWIndex(long bitrate);
	std::vector<long> GetVideoBitrates(void);
	std::vector<long> GetAudioBitrates(void);
	void StopInjection(void);
	void StartInjection(void);
protected:
	StreamInfo* GetStreamInfo(int idx);
private:
	class PrivateStreamAbstractionMPD* mPriv;
};

/**
 * @}
 */


