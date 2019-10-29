px.import({scene: "px:scene.1.js", keys: 'px:tools.keys.js',}).then(function (imports) {

    var scene = imports.scene;
    var keys = imports.keys;
    //var fileUrl = px.appQueryParams.url;
    var fileUrl = "https://dash.akamaized.net/envivio/EnvivioDash3/manifest.mpd";
    //var fileUrl = "http://www.bok.net/dash/tears_of_steel/cleartext/stream.mpd";
    console.log('viewVideo.js loaded, url = ' + fileUrl);
    var player = scene.create({t: 'aamp', url: fileUrl, w: scene.w, h: scene.h, x: 0, y: 0, parent: scene.root})

    var isVideoPaused = false;
    scene.root.on("onKeyDown", function (e) {
        console.log('??' + e.keyCode);
        if (e.keyCode === keys.SPACE) {
            if (!isVideoPaused) {
                player.pause();
                isVideoPaused = true;
            } else {
                player.play();
                isVideoPaused = false;
            }
        }
    });
});
