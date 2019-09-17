px.import({scene: "px:scene.1.js", keys: 'px:tools.keys.js',}).then(function (imports) {
    var scene = imports.scene;
    var keys = imports.keys;
    var fileUrl = px.appQueryParams.url;
    console.log('start load video, url = ' + fileUrl);
    var root = scene.root;
    var isLoaded = false;

    var player = scene.create({
        t: 'video',
        src: fileUrl,
        x: 0,
        y: 0,
        w: root.w,
        h: root.h,
        autoPlay: true,
        loop: true,
        parent: scene.root,
    });


    // >>> progress
    var playRes = scene.create({t: 'imageResource', url: 'browser/images/icons8-play-24.png'});
    var pauseRes = scene.create({t: 'imageResource', url: 'browser/images/icons8-pause-24.png'});

    var progressRect = scene.create({t: 'rect', x: 0, y: 0, w: root.w, h: 48, fillColor: 0xAfAfAf3f, parent: root});

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
        x: 36,
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
        stretchX: scene.stretch.STRETCH, stretchY: scene.stretch.STRETCH
    });

    var pBar = scene.create({t: 'rect', x: 104, y: 19, h: 10, fillColor: 0xaaaaaa55, parent: progressRect});
    var pPos = scene.create({t: 'rect', x: 0, y: -6, h: 24, w: 4, fillColor: 0x0e0e0e88, parent: pBar});


    function updatePlayBtnRes() {
        playBtn.resource = player.paused ? playRes : pauseRes;
    }

    function togglePauseOrPlay() {
        if (!isLoaded) {
            return;
        }
        if (player.paused) {
            player.play();
        } else {
            player.pause();
        }
        updatePlayBtnRes();
    }

    pBar.on('onMouseUp', function (e) {
        if (!isLoaded) {
            return;
        }
        var radio = e.x / pBar.w;
        var seekTime = player.duration * radio;
        console.log('seek to ' + seekTime);
        player.position = seekTime;
    });

    playBtn.on('onMouseUp', function () {
        togglePauseOrPlay();
    });


    scene.root.on("onKeyDown", function (e) {
        if (e.keyCode === keys.SPACE) {
            togglePauseOrPlay();
        }
    });

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
        if (pBar.w <= 0 || !isLoaded) {
            return;
        }

        var radio = 0;
        var d = player.duration;
        var c = player.position;

        if (c >= d) {
            radio = 1;
        } else {
            radio = c / d;
        }
        pPos.x = pBar.w * radio;
        currentTxt.text = getTime(radio * d);
    }

    const evts = [
        'abort',
        'canplay',
        'canplaythrough',
        'durationchange',
        'ended',
        'error',
        'loadeddata',
        'loadedmetadata',
        'loadstart',
        'pause',
        'play',
        'playing',
        'seeked',
        'seeking'
    ];
    for (var i = 0; i < evts.length; i++) {
        var evt = evts[i];
        player.on(evt, function (name) {
            console.log('>>>>>>> got evt from player, event name = ' + name);
            if (name === 'error') {
                console.log(player.error);
            }
        })
    }

    player.mediaSource.on('sourceclose', () => {
        console.log('>>>>>>> got evt from mediaSource, sourceclose');
    });

    player.mediaSource.on('sourceended', () => {
        console.log('>>>>>>> got evt from mediaSource, sourceended');
    });

    player.mediaSource.on('sourceopen', () => {
        console.log('>>>>>>> got evt from mediaSource, sourceopen..');
        totalText.text = getTime(player.mediaSource.duration);
        isLoaded = true;
    });

    player.on('timeupdate', function () {
        updatePos();
    });
    player.on('playing', function () {
        updatePlayBtnRes();
    });

    player.on('stop', function () {
        updatePlayBtnRes();
    });
    player.on('pause', function () {
        updatePlayBtnRes();
    });

    function updateProgressBox() {
        var margin = 12;
        progressRect.x = margin;
        progressRect.y = root.h - progressRect.h - margin;
        progressRect.w = root.w - margin * 2;
        pBar.w = progressRect.w - pBar.x - totalText.w - 12;
        totalText.x = pBar.x + pBar.w + 6;
    }

    updateProgressBox();

    scene.on("onResize", function (e) {
        player.w = root.w;
        player.h = root.h;
        updateProgressBox();
    });
});