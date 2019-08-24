
px.import({scene: "px:scene.1.js", 
    keys: 'px:tools.keys.js'
}).then(function (imports) {
    var scene = imports.scene;
    var keys = imports.keys;
    var root = scene.root;
    //var baseUrl = "mse_demo_data/";
    var baseUrl = 'https://bitdash-a.akamaihd.net/content/MI201109210084_1/video/720_2400000/dash/';
    var initUrl = baseUrl + 'init.mp4';
    var templateUrl = baseUrl + 'segment_$Number$.m4s';
    var index = 0;
    var numberOfChunks = 10;
    var sourceBuffer;

    var video = scene.create({ 
        t: 'video',
        x: 0,
        y: 0,
        w: root.w,
        h: root.h,
        parent: scene.root
    });

    var mediaSource = createMSEMediaSource();
    video.attachMediaSource(mediaSource);

    mediaSource.addEventListener('sourceopen', onMediaSourceOpen);
    console.log("created ms");

    function onMediaSourceOpen() {
        console.log("onMediaSourceOpen");

        sourceBuffer = mediaSource.addSourceBuffer('video/mp4; codecs="avc1.4d401f"');
        sourceBuffer.addEventListener('updateend', nextSegment);

        px.getFile(initUrl).then(function (buf) {
            console.log("appending init segment buffer with length=" + buf.length);
            var arr = new Uint8Array(buf);
            sourceBuffer.appendBuffer(arr);
        });

        video.play();
    }

    function nextSegment() {
        console.log("nextSegment with index=" + index);

        var url = templateUrl.replace('$Number$', index);

        px.getFile(url).then(function (buf) {
            console.log("appending data segment buffer with length=" + buf.length);
            var arr = new Uint8Array(buf);
            sourceBuffer.appendBuffer(arr);
        });

        index++;
        if (index >= numberOfChunks) {
            sourceBuffer.removeEventListener('updateend', nextSegment);
        }
    }
});
