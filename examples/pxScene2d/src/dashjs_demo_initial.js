
px.import({scene: "px:scene.1.js", 
    keys: 'px:tools.keys.js'
}).then(function (imports) {
    var scene = imports.scene;
    var keys = imports.keys;
    var root = scene.root;

    global.fetch('https://github.com/')
    .then(res => res.text())
    .then(body => console.log("fetch-res:"+body.length));

    function reqListener () {
        //console.log(this.response);
        console.log("xhr-res:"+this.responseText.length);
        var parser = new DOMParser();
        var ret = parser.parseFromString(this.responseText, 'text/xml');
        console.log(ret.nodeType);
        console.log(this.responseText);
    }

    var oReq = new global.XMLHttpRequest();
    oReq.addEventListener("load", reqListener);
    oReq.open("GET", "https://dash.akamaized.net/envivio/EnvivioDash3/manifest.mpd");
    oReq.send();

    var video = scene.create({ 
        t: 'video',
        x: 0,
        y: 0,
        w: root.w,
        h: root.h,
        parent: scene.root
    });
    
    var player = global.dashjs.MediaPlayer().create();
    //player.initialize(video);
    console.log(player);
});
