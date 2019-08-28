px.configImport({"browser:" : "browser/"});

px.import({scene: "px:scene.1.js", 
        keys:    'px:tools.keys.js',
        ListBox: 'browser:listbox.js',
        EditBox: 'browser:editbox.js'
        }).then(function (imports) {
    var scene = imports.scene;
    var keys = imports.keys;
    var root = scene.root;
    var videoWidth = 640;
    var videoHeight = 480;
    var listBoxOffsetY = 128;

    var video;
    var player;

    //var url = "https://dash.akamaized.net/envivio/EnvivioDash3/manifest.mpd";

    function reinitPlayer(videoUrl)
    {
        if (player != undefined) {
            player.reset();
        }

        video = scene.create({ 
            t: 'video',
            x: 0,
            y: listBoxOffsetY,
            w: videoWidth, //root.w,
            h: videoHeight, //root.h,
            parent: scene.root
        });

        //videoUrl = "https://dash.akamaized.net/envivio/EnvivioDash3/manifest.mpd";
        player = global.dashjs.MediaPlayer().create();
        player.updateSettings({ 'debug': { 'logLevel': global.dashjs.Debug.LOG_LEVEL_DEBUG } });
        player.updateSettings({ 'streaming': { 'abr': { 'autoSwitchBitrate': { video: false } } } });
        player.initialize(video, videoUrl, true);
        player.on('error', function(e) {
            console.log('onerror');
            console.log('error_code:'+e.error.code);
            console.log('error_message:'+e.error.message);
            console.log('error_data:'+e.error.data);
        });

        addVideoEventListeners();
    }

    //reinitPlayer();

    // >>> progress
    var playRes = scene.create({t: 'imageResource', url: 'browser/images/icons8-play-24.png'});
    var pauseRes = scene.create({t: 'imageResource', url: 'browser/images/icons8-pause-24.png'});

    var progressRect = scene.create({t: 'rect', x: 0, y: listBoxOffsetY + videoHeight, w: root.w, h: 48, fillColor: 0xAfAfAf3f, parent: root});

    var currentTxt = scene.create({
        t: 'text',
        x: 36,
        y: 10,
        text: '00:00:00',
        parent: progressRect,
        textColor: 0x0e0e0eff
    });

    var totalText = scene.create({
        t: 'text',
        x: root.w / 2 + 128,
        y: 10,
        text: '00:00:00',
        parent: progressRect,
        textColor: 0x0e0e0eff
    });

    var playBtn = scene.create({
        t: 'image',
        resource: playRes,
        parent: progressRect,
        a: 0.8,
        x: 6,
        y: 12,
        w: 24,
        h: 24,
    });

    var sourcesConf;

    var streamsListBox1 = new imports.ListBox( { parent: root, x: 0, y: 0, w: videoWidth / 2, h: listBoxOffsetY, visible:true, numItems:5 });
    var streamsListBox2 = new imports.ListBox( { parent: root, x: videoWidth / 2, y: 0, w: videoWidth, h: listBoxOffsetY, visible:true, numItems:5 });
    streamsListBox1.visible = true;
    streamsListBox2.visible = true;

    px.getFile("dash_sources.json").then(function (buf) {
        sourcesConf = JSON.parse(buf);
        for (var i = 0; i < sourcesConf["items"].length; ++i) {
            var item = sourcesConf["items"][i];
            console.log(item);
            streamsListBox1.addItem(item["name"]);
        }
    });

    var bitrateListBox       = new imports.ListBox( {     parent: root, x: 3 * videoWidth / 2,   y: 0, w: 200, h: listBoxOffsetY, visible:true, numItems:7 });
    bitrateListBox.visible = true;

    var pBar = scene.create({t: 'rect', x: 104, y: 19, w: root.w / 2, h: 10, fillColor: 0xaaaaaa55, parent: progressRect});
    var pPos = scene.create({t: 'rect', x: 0, y: 0, h: 10, w: 1, fillColor: 0x0096d788, parent: pBar});
    var pBufPos = scene.create({t: 'rect', x: 0, y: 0, h: 10, w: 1, fillColor: 0xdadada88, parent: pBar});

    function updatePlayBtnRes() {
        playBtn.resource = player.isPaused() ? playRes : pauseRes;
    }

    function togglePauseOrPlay() {
        if (player.isPaused()) {
            player.play();
        } else {
            player.pause();
        }
        updatePlayBtnRes();
    }

    pBar.on('onMouseUp', function (e) {
        var radio = e.x / pBar.w;
        var seekTime = player.duration() * radio;
        console.log('seek to ' + seekTime);
        player.seek(seekTime);
        togglePauseOrPlay();
    });

    var prevItemIndex1 = -1;
    var prevItemIndex2 = -1;
    var prevBitrateIndex = -1;

    scene.root.on('onMouseUp', function (e) {
        var itemIndex = getItemList1Index(streamsListBox1.selectedItem());
        //streamsListBox2.clear();
        if (prevItemIndex1 != itemIndex) {
            streamsListBox2 = new imports.ListBox( { parent: root, x: videoWidth / 2, y: 0, w: videoWidth, h: listBoxOffsetY, visible:true, numItems:5 });
            streamsListBox2.visible = true;
            prevItemIndex1 = itemIndex;
        } else {
            var itemIndex2 = getItemList2Index(itemIndex, streamsListBox2.selectedItem());
            if (prevItemIndex2 != itemIndex2) {
                var item = sourcesConf["items"][itemIndex]["submenu"][itemIndex2];
                console.log("loading "  + item["url"]);
                reinitPlayer(item["url"]);
                prevItemIndex2 = itemIndex2;
            } else {
                var bitRateIndex = getBitrateItemIndex();
                if (bitRateIndex != prevBitrateIndex) {
                    player.setQualityFor('video', bitRateIndex);
                    prevBitrateIndex = bitRateIndex;
                }
            }
            return;
        }
        if (itemIndex == -1) {
            return;
        }
        for (var i = 0; i < sourcesConf["items"][itemIndex]["submenu"].length; ++i) {
            var item = sourcesConf["items"][itemIndex]["submenu"][i];
            streamsListBox2.addItem(item["name"]);
        }
    });

    playBtn.on('onMouseUp', function () {
        togglePauseOrPlay();
    });


    scene.root.on("onKeyDown", function (e) {
        if (e.keyCode === keys.SPACE) {
            togglePauseOrPlay();
        }
    });

    function getItemList1Index(item)
    {
        var ret = -1;
        for (var i = 0; i < sourcesConf["items"].length; ++i) {
            if (sourcesConf["items"][i]["name"] == item) {
                ret = i;
                break;
            }
        }
        return ret;
    }

    function getBitrateItemIndex()
    {
        var ret = -1;
        for (var i = 0; i < bitrateListBox.getItemsLength(); ++i) {
            if (bitrateListBox.selectedItem() == bitrateListBox.getItem(i)) {
                ret = i;
                break;
            }
        }
        return ret;
    }

    function getItemList2Index(idx1, item)
    {
        var ret = -1;
        for (var i = 0; i < sourcesConf["items"][idx1]["submenu"].length; ++i) {
            if (sourcesConf["items"][idx1]["submenu"][i]["name"] == item) {
                ret = i;
                break;
            }
        }
        return ret;
    }

    function getTime(t) {
        function pad(v) {
            return v < 10 ? ('0' + v) : (v + '')
        }

        t = Math.floor(t);
        var h = Math.floor(t / 3600);
        var m = Math.floor(t % 3600 / 60);
        var s = Math.floor(t % 3600 % 60);
        return pad(h) + ":" + pad(m) + ":" + pad(s)
    }

    function updatePos() {
        if (pBar.w <= 0) {
            return;
        }

        var bufradio = 0;
        var radio = 0;
        var d = player.duration();
        var c = player.time();
        var bd = video.buffered.length > 0 ? video.buffered.end(0) : 0;

        if (c >= d) {
            radio = 1;
        } else {
            radio = c / d;
        }
        if (bd >= d) {
            bufradio = 1;
        } else {
            bufradio = bd / d;
        }
        pPos.w = pBar.w * radio;
        pBufPos.w = pBar.w * bufradio;
        pBufPos.x = pPos.w;
        currentTxt.text = getTime(radio * d);
    }

    function addVideoEventListeners() {
        video.addEventListener('loadedmetadata', () => {
            totalText.text = getTime(player.duration());
            var arr = player.getBitrateInfoListFor('video');
            bitrateListBox       = new imports.ListBox( {     parent: root, x: 3 * videoWidth / 2,   y: 0, w: 200, h: listBoxOffsetY, visible:true, numItems:7 });
            bitrateListBox.visible = true;
            for (var i = 0; i < arr.length; ++i) {
                var elem = arr[i];
                var elemStr = Math.floor(elem.bitrate / 1000) + ' kbps';
                elemStr += ' (' + elem.width + 'x' + elem.height + ')';
                bitrateListBox.addItem(elemStr);
            }
        });
        video.addEventListener('timeupdate', function () {
            updatePos();
        });
        video.addEventListener('playing', function () {
            updatePlayBtnRes();
        });
        video.addEventListener('stop', function () {
            updatePlayBtnRes();
        });
        video.addEventListener('pause', function () {
            updatePlayBtnRes();
        });
    }

    function removeVideoEventListeners() {
        for (var evt in ['loadedmetadata', 'timeupdate', 'playing', 'stop', 'pause']) {
            video.removeEventListener(evt);
        }
    }
});
