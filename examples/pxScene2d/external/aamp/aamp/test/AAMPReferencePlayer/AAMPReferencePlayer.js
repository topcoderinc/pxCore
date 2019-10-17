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

window.onload = function() {

    // Video
    var video = document.getElementById("video");
    video.src = "aamps://tungsten.aaplimg.com/VOD/bipbop_adv_example_v2/master.m3u8";
    
    // Buttons
    var playButton = document.getElementById("playOrPauseButton");
    var muteButton = document.getElementById("muteVideoButton");
    var closedCaptionButton = document.getElementById("ccButton");
    var rwdButton = document.getElementById("rewindButton");
    var skipBwdButton = document.getElementById("skipBackwardButton");
    var skipFwdButton = document.getElementById("skipForwardButton");
    var fwdButton = document.getElementById("fastForwardButton");
    var helpContentButton = document.getElementById('helpButton');

    // Sliders
    var seekBar = document.getElementById("seekBar");

    var loadFile = document.getElementById("loadButton");
    var deleteFile = document.getElementById("deleteButton");
    var loadFileContent = document.getElementById("userURLPlayButton");

    var ccFlag = true;
    video.playbackRate = 1;

    //to refreash URL select field on every reload
    var storedURLs = JSON.parse(sessionStorage.getItem("URLItems"));
    if(storedURLs) {
        //to remove all current options of videoURLs
        while (videoURLs.firstChild) {
            videoURLs.removeChild(videoURLs.firstChild);
        }
        //iteratively adding all the options in session storage to videoURLs
        var option = document.createElement("option");
        option.disabled = true;
        option.value = "";
        option.text = "Select video URL:";
        videoURLs.add(option);
	
        for (var iter = 0; iter < storedURLs.length; iter++) {
            var option = document.createElement("option");
            option.value = storedURLs[iter];
            option.text = storedURLs[iter];
            videoURLs.add(option);
        }
    }

    //to show the navBar initially
    var navBar = document.getElementById('controlDiv');
    navBar.style.display = "block";
    var navBarNext = document.getElementById('controlDivNext');
    navBarNext.style.display = "block";

    //to set the toggle bar value on reload
     if(sessionStorage.autoPlayFlag=="true") {
         document.getElementById("autoPlayCheck").checked = true;
     } else {
         document.getElementById("autoPlayCheck").checked = false;
     }
     if(sessionStorage.replaceDateFlag=="true") {
         document.getElementById("replaceDateCheck").checked = true;
     } else {
         document.getElementById("replaceDateCheck").checked = false;
     }


    // play videoloadVideo
    function vidplay(evt) {
        if (loadFile.src == "") { // inital source load
            getVideo();
        }
        button = evt.target; //  get the button id to swap the text based on the state
        if (loadFile.paused) { // play the file, and display pause symbol
            loadFile.play();
            button.textContent = "||";
        } else { // pause and display play symbol
            loadFile.pause();
            button.textContent = ">";
        }
    }

    function playPause() {
        console.log("playPause");
        // If it is a trick play operation
        if(video.playbackRate!=1) {
            // Start playback with normal speed
            video.playbackRate = 1;
            video.play();
        } else {
            if (video.paused == true) {
                // Play the video
                video.playbackRate = 1;
                video.play();
                document.getElementById("playOrPauseIcon").src = "icons/pause.png"; 
                document.getElementById("playState").innerHTML = "PLAYING";
            } else { // Pause the video
                video.pause();
                document.getElementById("playOrPauseIcon").src = "icons/play.png";
                document.getElementById("playState").innerHTML = "PAUSE";
            }
        }
        document.getElementById("speedX").innerHTML = video.playbackRate;
    };

    video.muted = false;
    function mutePlayer() {
        if (video.muted == false) {
            // Mute the video
            video.muted = true;
            document.getElementById("muteIcon").src = "icons/mute.png";
        } else {
            // Unmute the video
            video.muted = false;
            document.getElementById("muteIcon").src = "icons/unMute.png";
        }
    };

    function closedCaptionPlayer() {
        if(window.AAMP_JSController!== undefined) {
            if (!window.AAMP_JSController.closedCaptionEnabled) {
                document.getElementById("ccIcon").src = "icons/closedCaptioning.png";
                document.getElementById("testCC").innerHTML =  "ON";
            } else {
                document.getElementById("ccIcon").src = "icons/closedCaptioningDisabled.png";
                document.getElementById("testCC").innerHTML =  "OFF";
            }
            window.AAMP_JSController.closedCaptionEnabled = !window.AAMP_JSController.closedCaptionEnabled;
        }
    };


    function setTime(tValue) {
        //  if no video is loaded, this throws an exception
        try {
            if (tValue == 0) {
                video.currentTime = tValue;
            } else {
                    video.currentTime += tValue;
            }

        } catch (err) {
            // errMessage(err) // show exception
            errMessage("Video content might not be loaded");
        }
    }

    // Event listener for the play/pause button
    playButton.addEventListener("click", function() {
        playPause();
    });
    // show listener for the
    helpContentButton.addEventListener("click", function() {
        showhide();
    });
    // Event listener for the mute button
    muteButton.addEventListener("click", function() {
        mutePlayer();
    });

    // Event listener for the cc button
    closedCaptionButton.addEventListener("click", function() {
        closedCaptionPlayer();
    });

    function showhide() {
        // Get the modal
        var modal = document.getElementById('myModal');
        modal.style.display = "block";
        setTimeout(function(){ 
            modal.style.display = "none";
        }, 5000);
    };

    var rwdButton = document.getElementById("rewindButton");
    var skipBwdButton = document.getElementById("skipBackwardButton");
    var skipFwdButton = document.getElementById("skipForwardButton");
    var fwdButton = document.getElementById("fastForwardButton");


    function skipBackward() {
        setTime(-3);
    };

    function skipForward() {
        setTime(3);
    };

    function fastrwd() {
        switch(video.playbackRate) {
            case -32:
                video.playbackRate *= 2;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case -16:
                video.playbackRate *= 2;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case -4:
                video.playbackRate *= 4;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case -1:
                video.playbackRate *= 4;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case 1:
                video.playbackRate *= -1;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case 4:
                video.playbackRate /= 4;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case 16:
                video.playbackRate /= 4;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case 32:
                video.playbackRate /= 2;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case 64:
                video.playbackRate /= 2;
                document.getElementById("speedX").innerHTML = video.playbackRate;
        }
        document.getElementById("playOrPauseIcon").src = "icons/play.png";
        document.getElementById("playState").innerHTML = "FAST REWIND";
    };

    function fastfwd() {
        switch(video.playbackRate) {
            case -64:
                video.playbackRate /= 2;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case -32:
                video.playbackRate /= 2;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case -16:
                video.playbackRate /= 4;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case -4:
                video.playbackRate /= 4;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case -1:
                video.playbackRate /= -1;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case 1:
                video.playbackRate *= 4;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case 4:
                video.playbackRate *= 4;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case 16:
                video.playbackRate *= 2;
                document.getElementById("speedX").innerHTML = video.playbackRate;
                break;
            case 32:
                video.playbackRate *= 2;
                document.getElementById("speedX").innerHTML = video.playbackRate;
        }
        document.getElementById("playOrPauseIcon").src = "icons/play.png";
        document.getElementById("playState").innerHTML = "FAST FORWARD";
    };

    function overlayDisplay() {
        // Get the modal
        var modal = document.getElementById('overlayModal');
        //modal.style.display = "block";
        if(modal.style.display == "block") {
            modal.style.display = "none";
        } else {
            modal.style.display = "block";
        }
    };

    function overlayController() {
        // Get the modal
        if(navBar.style.display == "block") {
            navBar.style.display = "none";
        } else {
            navBar.style.display = "block";
        }
        if(navBarNext.style.display == "block") {
            navBarNext.style.display = "none";
        } else {
            navBarNext.style.display = "block";
        }        
    };

    // Event listener for the rewind button
    rwdButton.addEventListener("click", function() {
        fastrwd();
    });

    // Event listener for the skip Backward button
    skipBwdButton.addEventListener("click", function() {
        skipBackward();
    });

    // Event listener for the skip Forward button
    skipFwdButton.addEventListener("click", function() {
        skipForward();
    });

    // Event listener for the fast Forward button
    fwdButton.addEventListener("click", function() {
        fastfwd();
    });

    // Event listener for the seek bar
    seekBar.addEventListener("change", function() {
        // Calculate the new time
        var time = video.duration * (seekBar.value / 100);
        console.log("seek cursor time");
        // Update the video time
        video.currentTime = time;
    });

    // Update the seek bar as the video plays
    video.addEventListener("timeupdate", function() {
        // Calculate the slider value
        var value = (100 / video.duration) * video.currentTime;
        //console.log("seek cursor");
        // Update the slider value
        if(isFinite(value)) {
            seekBar.value = value;
            seekBar.style.width = value+"%";
            seekBar.style.backgroundColor = "red";
            seekBar.innerHTML = Math.ceil(value)+"%";
            document.getElementById("testPosition").innerHTML =  value.toFixed(2);

            // To display closedCaptioning at the initial loading
            if(ccFlag && (window.AAMP_JSController!== undefined)) {
                if (window.AAMP_JSController.closedCaptionEnabled) {
                    document.getElementById("ccIcon").src = "icons/closedCaptioning.png";
                    document.getElementById("testCC").innerHTML =  "ON";
                } else {
                    document.getElementById("ccIcon").src = "icons/closedCaptioningDisabled.png";
                    document.getElementById("testCC").innerHTML =  "OFF";
                }
                ccFlag = false;
            }
        }
    });

    // Pause the video when the seek handle is being dragged
    seekBar.addEventListener("keydown", function() {
        video.pause();
    });

    // Play the video when the seek handle is dropped
    seekBar.addEventListener("keyup", function() {
        video.play();
    });


   function onTunedEvent(event) {
        console.log("onTuned event received:" + JSON.stringify(event));
    }
    
    function onTuneFailedEvent(event) {
        console.log("onTuneFailed event received: " + JSON.stringify(event));
    }
    
    function onStatusChangedEvent(event) {
        console.log("statusChanged event received: " + JSON.stringify(event) );
    }
    
    if (typeof(window.AAMP_JSController) !== "undefined") {
        console.log("Registering event listeners to AAMP_JSController");
        window.AAMP_JSController.addEventListener("playbackStarted", onTunedEvent);
        window.AAMP_JSController.addEventListener("playbackFailed", onTuneFailedEvent);
        window.AAMP_JSController.addEventListener("playbackStateChanged", onStatusChangedEvent);
    }

    //  load video file from input field
    function getVideotxt() {
        var fileURLContent = document.getElementById("userEnteredVideoURL").value; // get input field
        if (fileURLContent != "") {
            var newFileURLContent = fileURLContent;
            video.src = newFileURLContent;
            // if replaceDateCheck is checked
            if(sessionStorage.replaceDateFlag=="true") {
                // construct date string based on current date (avoids Sling url expiry)
                var date = new Date();
                var dateEnd = new Date();
                dateEnd.setHours(date.getHours() + 4);
                //fileURLContent = "aamp://q-cdn3-1-cg17-linear-7dd67fc2.movetv.com/clipslist/FOODHD/2017-05-19T11:14:19.981Z/2017-05-19T09:23:45.333Z/master_7_5_none.m3u8"; 
                // search for ISOdate formatted string inside the URL
                var dateOccurPos = fileURLContent.search(/(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d:[0-5]\d\.\d+([+-][0-2]\d:[0-5]\d|Z))|(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d:[0-5]\d([+-][0-2]\d:[0-5]\d|Z))|(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d([+-][0-2]\d:[0-5]\d|Z))/);
                // replace the Date stamp in the URL and save it as the new src
                if(dateOccurPos!=-1) {
                    var newFileURLContent = fileURLContent.slice(dateOccurPos+25,fileURLContent.length);
                    // search for the second ISOdate formatted string inside the URL
                    var dateNextOccurPos = newFileURLContent.search(/(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d:[0-5]\d\.\d+([+-][0-2]\d:[0-5]\d|Z))|(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d:[0-5]\d([+-][0-2]\d:[0-5]\d|Z))|(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d([+-][0-2]\d:[0-5]\d|Z))/);
                    if(dateNextOccurPos!=-1) {
                        //if there is a second date stamp in the url
                        newFileURLContent = fileURLContent.slice(0,dateOccurPos+1)+date.toISOString()+'/'+dateEnd.toISOString()+fileURLContent.slice(dateOccurPos+50,fileURLContent.length);
                        video.src = newFileURLContent;
                    } else {
                        newFileURLContent = fileURLContent.slice(0,dateOccurPos+1)+date.toISOString()+fileURLContent.slice(dateOccurPos+25,fileURLContent.length);
                        video.src = newFileURLContent;
                    }
                }
            }
            console.log("Loading:" + newFileURLContent);
            video.load(); // if HTML source element is used
            //document.getElementById("play").click();  // start play
            console.log("Playing:" + newFileURLContent);

            //using Web Storge to save user entered URLs
            if(typeof(Storage) !== "undefined") {
                if (sessionStorage.URLItems) {
                    var storedURLs = JSON.parse(sessionStorage.getItem("URLItems"));
                    storedURLs.push(newFileURLContent);        //push user entered URL
                    sessionStorage.setItem("URLItems", JSON.stringify(storedURLs));
                    //to remove all current options of videoURLs
                    while (videoURLs.firstChild) {
                        videoURLs.removeChild(videoURLs.firstChild);
                    }
                    //iteratively adding all the options in session storage to videoURLs
                    for (var iter = 0; iter < storedURLs.length; iter++) {
                        var option = document.createElement("option");
                        option.value = storedURLs[iter];
                        option.text = storedURLs[iter];
                        videoURLs.add(option);
                    }
                } else {    //the first time user entering the URL
                    initialURLs= [];
                    initialURLs.push(newFileURLContent);
                    sessionStorage.setItem("URLItems", JSON.stringify(initialURLs));
                    //add the first URL to videoURLs 
                    var option = document.createElement("option");
                    option.value = newFileURLContent;
                    option.text = newFileURLContent;
                    videoURLs.add(option);
                }
            }

            video.play();
            document.getElementById("playOrPauseIcon").src = "icons/pause.png";
            console.log("After play");
        } else {
            errMessage("Enter a valid video URL"); // fail silently
        }
    }

    //  set src == latest video file URL
    document.getElementById("loadButton").addEventListener("click", getVideo, false);

    loadFile.addEventListener("click", function() {
        video.play();
    });

    //  load video file from select field
    function getVideo() {
        var fileURLContent = document.getElementById("videoURLs").value; // get select field
        if (fileURLContent != "") {
            var newFileURLContent = fileURLContent;
            video.src = newFileURLContent;
            //get the selected index of the URL List
            var selectedURL = document.getElementById("videoURLs");
            var optionIndex = selectedURL.selectedIndex;
            // if replaceDateCheck is checked
            if(sessionStorage.replaceDateFlag=="true") {
                // construct date string based on current date (avoids Sling url expiry)
                var date = new Date();
                var dateEnd = new Date();
                dateEnd.setHours(date.getHours() + 4);
                //fileURLContent = "aamp://q-cdn3-1-cg17-linear-7dd67fc2.movetv.com/clipslist/FOODHD/2017-05-19T11:14:19.981Z/2017-05-19T09:23:45.333Z/master_7_5_none.m3u8"; 
                // search for ISOdate formatted string inside the URL
                var dateOccurPos = fileURLContent.search(/(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d:[0-5]\d\.\d+([+-][0-2]\d:[0-5]\d|Z))|(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d:[0-5]\d([+-][0-2]\d:[0-5]\d|Z))|(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d([+-][0-2]\d:[0-5]\d|Z))/);
                // replace the Date stamp in the URL and save it as the new src
                if(dateOccurPos!=-1) {
                    var newFileURLContent = fileURLContent.slice(dateOccurPos+25,fileURLContent.length);
                    // search for the second ISOdate formatted string inside the URL
                    var dateNextOccurPos = newFileURLContent.search(/(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d:[0-5]\d\.\d+([+-][0-2]\d:[0-5]\d|Z))|(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d:[0-5]\d([+-][0-2]\d:[0-5]\d|Z))|(\/\d{4}-[01]\d-[0-3]\dT[0-2]\d:[0-5]\d([+-][0-2]\d:[0-5]\d|Z))/);
                    if(dateNextOccurPos!=-1) {
                        //if there is a second date stamp in the url
                        newFileURLContent = fileURLContent.slice(0,dateOccurPos+1)+date.toISOString()+'/'+dateEnd.toISOString()+fileURLContent.slice(dateOccurPos+50,fileURLContent.length);
                        video.src = newFileURLContent;
                        //remove the current URL with old date stamp
                        selectedURL.remove(selectedURL.selectedIndex);
                        var option = document.createElement("option");
                        option.text = newFileURLContent;
                        //add the URL with the current date stamp into its place
                        selectedURL.add(option,optionIndex);
                        //create a list from the videoURLs and update it in the web storage 
                        var modifiedURLSet = [];
                        var listIter =  1; //ignore the first "Select Video URL" option
                        for(;listIter<document.getElementById("videoURLs").options.length; listIter++) {
                            modifiedURLSet.push(document.getElementById("videoURLs").options[listIter].text);
                        }
                        sessionStorage.setItem("URLItems", JSON.stringify(modifiedURLSet));
                    } else {
                        newFileURLContent = fileURLContent.slice(0,dateOccurPos+1)+date.toISOString()+fileURLContent.slice(dateOccurPos+25,fileURLContent.length);
                        video.src = newFileURLContent;
                        //remove the current URL with old date stamp
                        selectedURL.remove(selectedURL.selectedIndex);
                        var option = document.createElement("option");
                        option.text = newFileURLContent;
                        //add the URL with the current date stamp into its place
                        selectedURL.add(option,optionIndex);
                        //create a list from the videoURLs and update it in the web storage 
                        var modifiedURLSet = [];
                        var listIter =  1;//ignore the first "Select Video URL" option
                        for(;listIter<document.getElementById("videoURLs").options.length; listIter++) {
                            modifiedURLSet.push(document.getElementById("videoURLs").options[listIter].text);
                        }
                        sessionStorage.setItem("URLItems", JSON.stringify(modifiedURLSet));
                    }
                }
            }
            //set the index to the selected field
            document.getElementById("videoURLs").selectedIndex = optionIndex;
            console.log("Loading:" + newFileURLContent);
            video.load(); // if HTML source element is used
            //document.getElementById("play").click();  // start play
            console.log("Playing:" + newFileURLContent);
            if(sessionStorage.autoPlayFlag=="true") {
            video.play();
            document.getElementById("playOrPauseIcon").src = "icons/pause.png";
            } else {
            document.getElementById("playOrPauseIcon").src = "icons/play.png";
            }
            console.log("After Play:" + newFileURLContent);
        } else {
            errMessage("Enter a valid video URL"); // fail silently
        }
    }

    function deleteVideo() {
        var fileURL = document.getElementById("videoURLs").value; // get select field
        if (fileURL != "") {
          var delURL = document.getElementById("videoURLs");
          var selectPos = delURL.selectedIndex;
          var selectOption = delURL.options;
          //"Index: " + selectOption[selectPos].index + " is " + selectOption[selectPos].text
          var storedURLs = JSON.parse(sessionStorage.getItem("URLItems"));
          storedURLs.splice(selectPos,1);
          sessionStorage.setItem("URLItems", JSON.stringify(storedURLs));
          delURL.remove(delURL.selectedIndex);
          //Display Delete modal
          var delModal = document.getElementById('deleteModal');
          document.getElementById("deleteContent").innerHTML = fileURL;
          delModal.style.display = "block";
          setTimeout(function(){ 
            delModal.style.display = "none";
          }, 3000); 
        } 
    }

    //function to toggle autoPlay Button
    function autoPlayVideo() {
        document.getElementById("autoPlayCheck").checked = !document.getElementById("autoPlayCheck").checked;
        if(typeof(Storage) !== "undefined") {
            //Store the toogle state in a session variable
            sessionStorage.autoPlayFlag = document.getElementById("autoPlayCheck").checked;
        }
    }

    //function to toggle date updation Button
    function autoUpdateDate() {
        document.getElementById("replaceDateCheck").checked = !document.getElementById("replaceDateCheck").checked;
        if(typeof(Storage) !== "undefined") {
            //Store the toogle state in a session variable
            sessionStorage.replaceDateFlag = document.getElementById("replaceDateCheck").checked;
        }
    }

    function edittext(event) {
        var x = document.getElementById("userEnteredVideoURL");
    }
    var HTML5Player = function() {
        var that = this;
        this.init = function() {
            this.video = document.getElementById("video");

            // Buttons
            this.playButton = document.getElementById("playOrPauseButton");
            this.rwdButton = document.getElementById("rewindButton");
            this.skipBwdButton = document.getElementById("skipBackwardButton"); 
            this.skipFwdButton = document.getElementById("skipForwardButton");
            this.fwdButton = document.getElementById("fastForwardButton");
            this.muteButton = document.getElementById("muteVideoButton");
            this.closedCaptionButton = document.getElementById("ccButton");

            // Sliders
            this.seekBar = document.getElementById("seekBar");
            this.videoFileList = document.getElementById("videoURLs");
            this.loadVideoButton = document.getElementById("loadButton");
            this.deleteVideoButton = document.getElementById("deleteButton");
            this.autoPlayVideoButton = document.getElementById("autoPlayButton");
            this.replaceDateVideoButton = document.getElementById("replaceDateButton");
            this.userEnteredVideoURLContent = document.getElementById("userEnteredVideoURL");
            this.userURLPlayButtonContent = document.getElementById("userURLPlayButton");
            this.helpContentButton = document.getElementById('helpButton');

            this.currentObj = this.playButton;
            this.components = [this.playButton, this.rwdButton, this.skipBwdButton, this.skipFwdButton, this.fwdButton, this.muteButton, this.closedCaptionButton, this.videoFileList, this.loadVideoButton, this.deleteVideoButton, this.autoPlayVideoButton, this.replaceDateVideoButton, this.userEnteredVideoURLContent, this.userURLPlayButtonContent, this.helpContentButton];
            this.currentPos = 0;
            this.dropDownListVisible = false;
            this.selectListIndex = 0;
            this.prevObj = null;
            this.addFocus();
        };

        this.reset = function() {
        };

        this.keyLeft = function() {
            this.gotoPrevious();
        };

        this.keyRight = function() {
            this.gotoNext();
        };

        this.keyUp = function() {
            if (this.dropDownListVisible) {
                this.prevVideoSelect();
            } else if ((this.components[this.currentPos] == this.playButton) || (this.components[this.currentPos] == this.rwdButton) || (this.components[this.currentPos] == this.skipBwdButton) || (this.components[this.currentPos] == this.skipFwdButton) || (this.components[this.currentPos] == this.fwdButton) || (this.components[this.currentPos] == this.muteButton) || (this.components[this.currentPos] == this.closedCaptionButton)) {
                //when a keyUp is received from the buttons in the bottom navigation bar
                this.removeFocus();
                this.currentObj = this.videoFileList;
                //move focus to the first element in the top navigation bar
                this.currentPos = this.components.indexOf(this.videoFileList);
                this.addFocus();
            }
        };

        this.keyDown = function() {
            if (this.dropDownListVisible) {
                this.nextVideoSelect();
            } else if ((this.components[this.currentPos] == this.videoFileList) || (this.components[this.currentPos] == this.loadVideoButton) || (this.components[this.currentPos] == this.deleteVideoButton) || (this.components[this.currentPos] == this.autoPlayVideoButton) || (this.components[this.currentPos] == this.replaceDateVideoButton) || (this.components[this.currentPos] == this.userEnteredVideoURLContent) || (this.components[this.currentPos] == this.userURLPlayButtonContent) || (this.components[this.currentPos] == this.helpContentButton)) {
                //when a keyDown is received from the buttons in the top navigation bar
                this.removeFocus();
                this.currentObj = this.playButton;
                //move focus to the first element in the bottom navigation bar
                this.currentPos = 0;
                this.addFocus();
            }
        };

        this.prevVideoSelect = function() {
            if (this.selectListIndex > 0) {
                this.selectListIndex--;
            } else {
                this.selectListIndex = this.videoFileList.options.length - 1;
            }
            this.videoFileList.options[this.selectListIndex].selected = true;
        };

        this.nextVideoSelect = function() {
            if (this.selectListIndex < this.videoFileList.options.length - 1) {
                this.selectListIndex++;
            } else {
                this.selectListIndex = 0;
            }
            this.videoFileList.options[this.selectListIndex].selected = true;
        };

        this.showDropDown = function() {
            this.dropDownListVisible = true;
            var n = this.videoFileList.options.length;
            this.videoFileList.size = n;
        };

        this.hideDropDown = function() {
            this.dropDownListVisible = false;
            this.videoFileList.size = 1;
        };

        this.ok = function() {
            switch (this.currentPos) {
                case 0:
                        playPause();
                        break;
                case 1:
                        fastrwd();
                        break;
                case 2:
                        skipBackward();
                        break;
                case 3:
                        skipForward();
                        break;
                case 4:
                        fastfwd();
                        break;
                case 5:
                        mutePlayer();
                        break;
                case 6:
                        closedCaptionPlayer();
                        break;
                case 7:
                        if (this.dropDownListVisible == false) {
                            this.showDropDown();
                        } else {
                            this.hideDropDown();
                        }
                        break;
                case 8:
                        getVideo();
                        break;
                case 9:
                        deleteVideo();
                        break;
                case 10:
                        autoPlayVideo();
                        break;
                case 11:
                        autoUpdateDate();
                        break;
                case 13:
                        getVideotxt();
                        break;
                case 14:
                        showhide();
                        break;
            };
        };

        this.gotoNext = function() {
            this.removeFocus();
            if (this.currentPos < this.components.length - 1) {
                this.currentPos++;
            } else {
                this.currentPos = 0;
            }
            this.currentObj = this.components[this.currentPos];
            this.addFocus();
        };

        this.gotoPrevious = function() {
            this.removeFocus();
            if (this.currentPos > 0) {
                this.currentPos--;
            } else {
                this.currentPos = this.components.length - 1;
            }
            this.currentObj = this.components[this.currentPos];
            this.addFocus();
        };

        this.addFocus = function() {
            if (this.currentObj && this.components[this.currentPos] != this.userEnteredVideoURLContent) {
                this.currentObj.classList.add("focus");
            } else {
                this.currentObj.focus();
            }
        };

        this.removeFocus = function() {
            if (this.currentObj && this.components[this.currentPos] != this.userEnteredVideoURLContent) {
                this.currentObj.classList.remove("focus");
            } else {
                this.currentObj.blur();
            }
        };

        this.keyEventHandler = function(e, type) {
            var keyCode = e.which || e.keyCode;
            console.log(keyCode);
            var keyCodeList = [37, 38, 39, 40, 13, 32, 112, 113, 114, 115, 116, 117, 118, 119,120];
            if (this.components[this.currentPos] === this.userEnteredVideoURLContent && keyCodeList.indexOf(keyCode) === -1) {
                this.userEnteredVideoURLContent.focus();
                return true;
            } else {
                this.userEnteredVideoURLContent.blur();
            }
            e.preventDefault();
            if (type == "keydown") {
                switch (keyCode) {
                    case 37: // Left Arrow
                            this.keyLeft();
                            break;
                    case 38: // Up Arrow
                            this.keyUp();
                            break;
                    case 39: // Right Arrow
                            this.keyRight();
                            break;
                    case 40: // Down Arrow
                            this.keyDown();
                            break;
                    case 13: // Enter
                            this.ok();
                            break;
                    case 34: // Page Down
                            skipBackward();
                            break;
                    case 33: // Page Up
                            skipForward();
                            break;
                    case 32:
                            this.ok();
                            break;
                    case 112: // F1
                            playPause();
                            break;
                    case 113: // F2
                            mutePlayer();
                            break;
                    case 114: // F3
                            showhide();
                            break;
                    case 115: // F4
                            fastrwd();
                            break;
                    case 116: // F5
                            fastfwd();
                            break;
                    case 117: // F6
                            closedCaptionPlayer();
                            break;
                    case 118: // F7
                           overlayDisplay();
                            break;
                    case 119: // F8
                            overlayController();
                    default:
                            break;
                }
            }
            return false;
        }
    };

    var playerObj = new HTML5Player();
    playerObj.init();
    if (document.addEventListener) {
        document.addEventListener("keydown", function(e) {
            return playerObj.keyEventHandler(e, "keydown");
        });
    }
    if(sessionStorage.autoPlayFlag=="true") { //To autoPlay The initial video stream
        video.play();   //Start video playback
        document.getElementById("playOrPauseIcon").src = "icons/pause.png";
    } else {
        document.getElementById("playOrPauseIcon").src = "icons/play.png";
    }
}
