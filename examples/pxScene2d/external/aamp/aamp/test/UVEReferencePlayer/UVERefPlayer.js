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

var playerStatesEnum = { "idle":0, "initializing":1, "playing":8, "paused":6, "seeking":7 };
Object.freeze(playerStatesEnum);
var playbackSpeeds = [-64, -32, -16, -4, 1, 4, 16, 32, 64];

//Default DRM config for AAMP
var defaultDrmConfig = {'com.microsoft.playready':'mds.ccp.xcal.tv', 'com.widevine.alpha':'mds.ccp.xcal.tv', 'preferredKeysystem':'com.widevine.alpha'};

//AAMP initConfig is used to pass certain predefined config params to AAMP
//Commented out values are not implemented for now
//Values assigned are default values of each config param
//All properties are optional.
var defaultInitConfig = {
    /**
     * max initial bitrate (kbps)
     */
    initialBitrate: 2500000,

    /**
     * min amount of buffer needed before playback (seconds)
     */
    //initialBuffer: number;

    /**
     * max amount of buffer during playback (seconds)
     */
    //playbackBuffer: number;

    /**
     * start position for playback (ms)
     */
    offset: 0,

    /**
     * network request timeout (ms)
     */
    networkTimeout: 10,

    /**
     * max amount of time to download ahead of playhead (seconds)
     * e.x:
     *   with a downloadBuffer of 10s there will be 10 seconds of
     *   video or audio stored in javascript memory and not in a
     *   playback buffer
     */
    //downloadBuffer: number;

    /**
     * min amount of bitrate (kbps)
     */
    //minBitrate: number;

    /**
     * max amount of bitrate (kbps)
     */
    //maxBitrate: number;

    /**
     * preferred audio language
     */
    preferredAudioLanguage: "en",

    /**
     * TSB length in seconds, value of 0 means it is disabled
     */
    //timeShiftBufferLength: number;

    /**
     * offset from live point for live assets (in secs)
     */
    liveOffset: 15,

    /**
     * drmConfig for the playback
     */

    drmConfig: defaultDrmConfig //For sample structure defaultDrmConfig

    /**
     *  network proxy to use
     */
    //networkProxy: string;

    /**
     *  network proxy to use for license requests
     */
    //licenseProxy: string;
};

var urls = [
     { url: "http://ccr.col-jitp2.xcr.comcast.net/omg01/323/429/EST_ReadyPlayerOneHD_E9180038_mezz_LVLH04.mpd", resolveUrl: "", useDefaultDrmConfig: true },
     { url: "https://s3.dualstack.us-east-1.amazonaws.com/expo-first-vpc-vod-fragments/552d2/LG114_COMCAST_12858_(12858_ISMUSP).ism/LG114_COMCAST_12858_(12858_ISMUSP).mpd", resolveUrl: "https://vimond-rest-api.ha.expo-first.vimondtv.com/api/web/asset/12858/play?protocol=ISM", useDefaultDrmConfig: false },
    { url: "http://amssamples.streaming.mediaservices.windows.net/683f7e47-bd83-4427-b0a3-26a6c4547782/BigBuckBunny.ism/manifest(format=mpd-time-csf)", resolveUrl: "", useDefaultDrmConfig: false }
];


var playerState = playerStatesEnum.idle;
var playbackRateIndex = playbackSpeeds.indexOf(1);
var urlIndex = 0;
var mutedStatus = false;
var ccStatus = false;
var playerObj = null;

window.onload = function() {
    initPlayerControls();
    resetPlayer();
    resetUIOnNewAsset();

    loadUrl(urls[urlIndex]);
}

function playbackStateChanged(event) {
    console.log("Playback state changed event: " + JSON.stringify(event));
    switch (event.state) {
        case playerStatesEnum.idle:
            playerState = playerStatesEnum.idle;
            break;
        case playerStatesEnum.initializing:
            playerState = playerStatesEnum.initializing;
            break;
        case playerStatesEnum.playing:
            playerState = playerStatesEnum.playing;
            break;
        case playerStatesEnum.paused:
            playerState = playerStatesEnum.paused;
            break;
        case playerStatesEnum.seeking:
            playerState = playerStatesEnum.seeking;
            break;
        default:
            console.log("State not expected");
            break;
    }
    console.log("Player state is: " + playerState);
}

function mediaEndReached() {
    console.log("Media end reached event!");
    loadNextAsset();
}

function mediaSpeedChanged(event) {
    console.log("Media speed changed event: " + JSON.stringify(event));
    var currentRate = event.speed;
    console.log("Speed Changed [" + playbackSpeeds[playbackRateIndex] + "] -> [" + currentRate + "]");
    if (currentRate != undefined) {
        if (currentRate != 0) {
            playbackRateIndex = playbackSpeeds.indexOf(currentRate);
        } else {
                playbackRateIndex = playbackSpeeds.indexOf(1);
        }
        if (currentRate != 0 && currentRate != 1){
                showTrickmodeOverlay(currentRate);
        }

        if (currentRate === 1) {
            document.getElementById("playOrPauseIcon").src = "icons/pause.png";
            document.getElementById("playState").innerHTML = "PLAYING";
        } else {
            document.getElementById("playOrPauseIcon").src = "icons/play.png";
            if (currentRate === 0) {
                document.getElementById("playState").innerHTML = "PAUSED";
            } else if (currentRate > 1) {
                document.getElementById("playState").innerHTML = "FAST FORWARD";
            } else if (currentRate < 0) {
                document.getElementById("playState").innerHTML = "FAST REWIND";
            }
        }
        document.getElementById("speedX").innerHTML = currentRate;
    }
}

function mediaPlaybackFailed(event) {
    console.log("Media failed event: " + JSON.stringify(event));
    loadNextAsset();
}

function mediaMetadataParsed(event) {
    console.log("Media metadata event: " + JSON.stringify(event));
}

function subscribedTagNotifier(event) {
    console.log("Subscribed tag notifier event: " + JSON.stringify(event));
}

function mediaProgressUpdate(event) {
    //console.log("Media progress update event: " + JSON.stringify(event));
    var duration = event.durationMiliseconds;
    var position = event.positionMiliseconds;
    var value = ( position / duration ) * 100;
	var seekBar = document.getElementById("seekBar");
    document.getElementById("totalDuration").innerHTML = convertSStoHr(duration / 1000.0);
    document.getElementById("currentDuration").innerHTML = convertSStoHr(position / 1000.0);
    console.log("Media progress update event: value=" + value);
    // Update the slider value
    if(isFinite(value)) {
        seekBar.value = value;
        seekBar.style.width = value+"%";
        seekBar.style.backgroundColor = "red";
    }
}

function mediaPlaybackStarted() {
    bufferingDisplay(false);
    document.getElementById("playOrPauseIcon").src = "icons/pause.png";
    document.getElementById("playState").innerHTML = "PLAYING";

    var availableVBitrates = playerObj.getVideoBitrates();
    if (availableVBitrates !== undefined) {
        createBitrateList(availableVBitrates);
    }
}

function mediaPlaybackBuffering(event) {
    if (event.buffering === true){
        bufferingDisplay(true);
    } else {
        bufferingDisplay(false);
    }
}

function mediaDurationChanged(event) {
    console.log("Duration changed!");
}

function decoderHandleAvailable(event) {
    console.log("decoderHandleAvailable " + event.decoderHandle);
    XREReceiver.onEvent("onDecoderAvailable", { decoderHandle: event.decoderHandle });
}

// helper functions
function resetPlayer() {
    if (playerObj !== null) {
        playerObj.destroy();
        playerObj = null;
    }

    playerObj = new AAMPPlayer();
    playerObj.addEventListener("playbackStateChanged", playbackStateChanged);
    playerObj.addEventListener("playbackCompleted", mediaEndReached);
    playerObj.addEventListener("playbackSpeedChanged", mediaSpeedChanged);
    playerObj.addEventListener("playbackFailed", mediaPlaybackFailed);
    playerObj.addEventListener("mediaMetadata", mediaMetadataParsed);
    playerObj.addEventListener("timedMetadata", subscribedTagNotifier);
    playerObj.addEventListener("playbackProgressUpdate", mediaProgressUpdate);
    playerObj.addEventListener("playbackStarted", mediaPlaybackStarted);
    playerObj.addEventListener("bufferingChanged", mediaPlaybackBuffering);
    playerObj.addEventListener("durationChanged", mediaDurationChanged);
    playerObj.addEventListener("decoderAvailable", decoderHandleAvailable);
    playerState = playerStatesEnum.idle;
    mutedStatus = false;
}

function loadUrl(urlObject) {
    if (urlObject.resolveUrl.length !== 0) {
        httpRequest = new XMLHttpRequest();
        httpRequest.open('GET', urlObject.resolveUrl);
        httpRequest.setRequestHeader('Accept', 'application/json');
        httpRequest.setRequestHeader('x-vimond-tenant', 'NAB2017');
        httpRequest.send();
        httpRequest.onreadystatechange = function() {
            if (httpRequest.readyState === 4 && httpRequest.status === 200) {
                console.log(httpRequest.responseText);
                var obj = JSON.parse(httpRequest.responseText);
                var drmConfigName = null;
                var drmName = obj.playback.items.item.license['@name'];
                if (drmName.indexOf('playready') !== -1) {
                    drmConfigName = "com.microsoft.playready";
                }
                var licenseUrl = obj.playback.items.item.license['@uri'];
                console.log("licenseUrl: " + licenseUrl);
                var drmConfiguration = '{' + '"' + drmConfigName + '"' + ':"' + licenseUrl + '",' + '"preferredKeysystem":"' + drmConfigName + '"' +'}';
                console.log("drmConfig: " + drmConfiguration);
                var initConfiguration = defaultInitConfig;
                initConfiguration.drmConfig = JSON.parse(drmConfiguration);
                playerObj.initConfig(initConfiguration);
                playerObj.load(urlObject.url);
            }
        }
    } else if (urlObject.useDefaultDrmConfig === true) {
        playerObj.initConfig(defaultInitConfig);
        playerObj.load(urlObject.url);
    } else {
        playerObj.load(urlObject.url);
    }
}
