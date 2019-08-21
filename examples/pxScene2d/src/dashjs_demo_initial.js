
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
        console.log("xhr-res:"+this.responseText.length);
    }

    var oReq = new global.XMLHttpRequest();
    oReq.addEventListener("load", reqListener);
    oReq.open("GET", "http://www.google.com");
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
