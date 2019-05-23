Required Config:

1. Host UVEReferencePlayer folder in a web server or in /opt/www/ folder in device
   Folders inside /opt/www/ can be accessed using http://localhost:50050/<folder name>

2. If hosted in /opt/www folder, need the following override config files in /opt folder since
   XRE rejects Fling requests with URLs with domain localhost/127.0.0.1

a. /opt/webfilterpatterns.conf
{"webfilter":[
  {"host":"localhost", "block":"0"},
  {"host":"127.0.0.1", "block":"1"},
  {"host":"169.254.*", "block":"1"},
  {"host":"192.168.*", "block":"1"},
  {"host":"[::1]", "block":"1"},
  {"scheme":"file", "block":"1"},
  {"scheme":"ftp", "block":"1"},
  {"scheme":"*", "block":"0"},
  {"block":"1"}
]}

b. /opt/xrehtmlappswhitelist.conf
{"htmlappswhitelist":
  [
    {
      "name" : "About Blank Page",
      "url" : "about:blank"
    },
    {
      "name" : "Viper Player",
      "url" : "ccr.player-platform-stage.xcr.comcast.net/index.html"
    },
    {
      "name" : "Google",
      "url" : "www.google.com"
    },
    {
      "name" : "AAMP Reference Player",
      "url" : "localhost:50050/UVEReferencePlayer/index.html"
    }
  ]
}

3. Fling index.html to launch reference player (http://localhost:50050/UVEReferencePlayer/index.html)
   Reference Player require injectedbundle support and hence might not work with WPELauncher straightforward


Notes:
- AAMP in browser currently uses Unified Video Engine APIs
- clear/encrypted HLS supported
- clear/encrypted(Playready/Widevine) DASH supported
- Refer following URL for UVE specification details
  https://etwiki.sys.comcast.net/pages/viewpage.action?spaceKey=VIDEOARCH&title=Unified+Video+Engine+API+v0.5
- JS code have been split into -
  UVERefPlayer.js - The main entry point for JS workflow
  UVEPlayerUI.js - UI code for reference player
  UVEMediaPlayer.js - AAMP Engine impl for reference player
