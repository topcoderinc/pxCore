
px.import({scene: "px:scene.1.js", 
    keys: 'px:tools.keys.js'
}).then(function (imports) {

    var scene = imports.scene;
    var keys = imports.keys;
    var root = scene.root;

    var ms = scene.create({ t: 'mediaSource' });
    ms.mediaSource.on('sourceopen', onMediaSourceOpen);
    console.log("created ms");

    function onMediaSourceOpen() {
        //sourceBuffer = ms.addSourceBuffer('video/mp4; codecs="avc1.4d401f"');
        //sourceBuffer.addEventListener('updateend', nextSegment);

        //GET(initUrl, appendToBuffer);

        //video.play();
        console.log("onMediaSourceOpen");
    }

    /*
    var promise = px.getFile("mse_demo_data/init.mp4").then(function (buf) {
        console.log(buf.length);
        var arr = new Uint8Array(buf);
        console.log(arr.length);
    });
    */

    /*
    Promise.all([promise]);

    var TOTAL_SEGMENTS = 52;
    for (var i = 0; i < TOTAL_SEGMENTS; ++i) {
        var filePath = "mse_demo_data/segment_" + i + ".m4s";
        promise = px.getFile(filePath).then(function (buf) {
            console.log(filePath + ": " + buf.length);
        });
        Promise.all([promise]);
    }
    */

});
