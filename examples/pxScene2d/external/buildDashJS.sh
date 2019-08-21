#!/bin/bash

wget https://github.com/Dash-Industry-Forum/dash.js/archive/v3.0.0.tar.gz
tar xzvf v3.0.0.tar.gz
cd dash.js-3.0.0
npm install
npm install node-fetch
npm install xmlhttprequest
npm install -g grunt-cli
grunt dist

perl -e 'print "var gmodule = module;"' > dist/dash.all.debug.pxcore.js
cat dist/dash.all.debug.js >> dist/dash.all.debug.pxcore.js
sed -i -e 's/exports.Debug/gmodule.exports = exports; exports.Debug/' dist/dash.all.debug.pxcore.js
