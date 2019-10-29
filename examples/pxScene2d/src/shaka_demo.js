
px.import({scene: "px:scene.1.js", 
    keys: 'px:tools.keys.js'
}).then(function (imports) {
    var scene = imports.scene;
    var keys = imports.keys;
    var root = scene.root;

    var video = scene.create({ 
        t: 'video',
        x: 0,
        y: 0,
        w: root.w,
        h: root.h,
        parent: scene.root
    });

    global.shaka.polyfill.installAll();

    var timerCb = function()
      {
        video.play();
        setTimeout(timerCb, 1000);
      };

    timerCb();

    // live dash streams
    //var url = "https://irtdashreference-i.akamaihd.net/dash/live/901161/bfs/manifestARD.mpd";
    //var url = "https://irtdashreference-i.akamaihd.net/dash/live/901161/bfs/manifestBR.mpd";

    var url = "https://dash.akamaized.net/envivio/EnvivioDash3/manifest.mpd";
    //var url = "https://bitdash-a.akamaihd.net/content/MI201109210084_1/m3u8s/f08e80da-bf1d-4e3d-8899-f0f6155f6efa.m3u8";
    //var url = "https://bitdash-a.akamaihd.net/content/sintel/hls/playlist.m3u8";
    //var url = "http://www.bok.net/dash/tears_of_steel/cleartext/stream.mpd";
    //var url = "https://bitmovin-a.akamaihd.net/content/MI201109210084_1/mpds/f08e80da-bf1d-4e3d-8899-f0f6155f6efa.mpd";
    var player = new global.shaka.Player(video);
    var onErrorCb = function(e) {
        console.log('onerror');
        console.log('error_message:'+e);
    };
    player.addEventListener('error', onErrorCb);
    player.load(url).then(function () {
        console.log('video has loaded');
        video.play();
    }).catch(onErrorCb);

    console.log("END-MAIN");
}).catch( function importFailed(err){
  console.error("Import failed for shaka_demo.js: " + err);
});

