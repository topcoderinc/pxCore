
px.import({scene: "px:scene.1.js", 
    keys: 'px:tools.keys.js'
}).then(function (imports) {
    var scene = imports.scene;
    var keys = imports.keys;
    var root = scene.root;
    var videoBaseUrl = 'https://bitmovin-a.akamaihd.net/content/MI201109210084_1/video/720_2400000/dash/';
    var audioBaseUrl = 'https://bitmovin-a.akamaihd.net/content/MI201109210084_1/audio/1_stereo_128000/dash/';
    var initVideoUrl = videoBaseUrl + 'init.mp4';
    var initAudioUrl = audioBaseUrl + 'init.mp4';
    var templateVideoUrl = videoBaseUrl + 'segment_$Number$.m4s';
    var templateAudioUrl = audioBaseUrl + 'segment_$Number$.m4s';
    var indexVideo = 0;
    var indexAudio = 0;
    var numberOfChunks = 20;
    var sourceBufferVideo;
    var sourceBufferAudio;

    var video = scene.create({ 
        t: 'video',
        x: 0,
        y: 0,
        w: root.w,
        h: root.h,
        parent: scene.root
    });

    var mediaSource = video.createMediaSource();

    mediaSource.on('sourceopen', onMediaSourceOpen);
    console.log("created ms");

    function onMediaSourceOpen() {
        console.log("onMediaSourceOpen");

        sourceBufferVideo = mediaSource.addSourceBuffer('video/mp4; codecs="avc1.42c00d"');
        sourceBufferVideo.on('updateend', nextVideoSegment);

        sourceBufferAudio = mediaSource.addSourceBuffer('audio/mp4; codecs="mp4a.40.2"');
        sourceBufferAudio.on('updateend', nextAudioSegment);

        px.getFile(initVideoUrl).then(function (buf) {
            console.log("appending init video segment buffer with length=" + buf.length);
            var arr = new Uint8Array(buf);
            sourceBufferVideo.appendBuffer(arr);
        });

        px.getFile(initAudioUrl).then(function (buf) {
            console.log("appending init audio segment buffer with length=" + buf.length);
            var arr = new Uint8Array(buf);
            sourceBufferAudio.appendBuffer(arr);
        });

        video.play();
    }

    function nextVideoSegment() {
        console.log("nextVideoSegment with index=" + indexVideo);

        var url = templateVideoUrl.replace('$Number$', indexVideo);

        px.getFile(url).then(function (buf) {
            console.log("appending data video segment buffer with length=" + buf.length);
            var arr = new Uint8Array(buf);
            sourceBufferVideo.appendBuffer(arr);
        });

        indexVideo++;
        if (indexVideo >= numberOfChunks) {
            sourceBufferVideo.delListener('updateend', nextVideoSegment);
        }
    }

    function nextAudioSegment() {
        console.log("nextAudioSegment with index=" + indexAudio);

        var url = templateAudioUrl.replace('$Number$', indexAudio);

        px.getFile(url).then(function (buf) {
            console.log("appending data Audio segment buffer with length=" + buf.length);
            var arr = new Uint8Array(buf);
            sourceBufferAudio.appendBuffer(arr);
        });

        indexAudio++;
        if (indexAudio >= numberOfChunks) {
            sourceBufferAudio.delListener('updateend', nextAudioSegment);
        }
    }
});
