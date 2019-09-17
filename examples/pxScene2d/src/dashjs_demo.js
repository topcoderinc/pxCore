
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

    var url = "https://dash.akamaized.net/envivio/EnvivioDash3/manifest.mpd";
    //var url = "http://www.bok.net/dash/tears_of_steel/cleartext/stream.mpd";
    //var url = "https://bitmovin-a.akamaihd.net/content/MI201109210084_1/mpds/f08e80da-bf1d-4e3d-8899-f0f6155f6efa.mpd";
    var player = global.dashjs.MediaPlayer().create();
    player.updateSettings({ 'debug': { 'logLevel': global.dashjs.Debug.LOG_LEVEL_INFO } });
    player.initialize(video, url, true);
    player.on('error', function(e) {
        console.log('onerror');
        console.log('error_code:'+e.error.code);
        console.log('error_message:'+e.error.message);
        console.log('error_data:'+e.error.data);
    });

    console.log("END-MAIN");
});
