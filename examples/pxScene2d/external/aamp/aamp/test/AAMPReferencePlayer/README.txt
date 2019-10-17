Required Config:
export GST_AAMP_EXPOSE_HLS_CAPS

systemctl stop xre-receiver
export XDG_RUNTIME_DIR=/run/user/0
export LD_PRELOAD=/usr/lib/libwayland-client.so.0:/usr/lib/libwayland-egl.so.0
export EGL_PLATFORM=wayland
export QT_WAYLAND_USE_XDG_SHELL=1
export QT_WAYLAND_CLIENT_BUFFER_INTEGRATION=wayland-egl
export WAYLAND_DISPLAY=xre-nested-display
westeros --renderer /usr/lib/libwesteros_render_nexus.so.0 --framerate 60 --display $WAYLAND_DISPLAY &

Fling index.html to launch sample player (or use WPELauncher.sh http://<ipaddr>:8000/index.html)

Notes:
- AAMP in browser currently uses standard html5 video tag syntax, not PlayerPlatform APIs
- AAMP supports a JS Controller object for access to inband closed captions
- URLs must be prefixed with aamp:// (instead of http)
- clear HLS supported
- limited support for DASH (clear and PlayReady) content



