/*

pxCore Copyright 2005-2018 John Robinson

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

"use strict";

var isDuk=(typeof Duktape != "undefined")?true:false;
var isV8=(typeof _isV8 != "undefined")?true:false;

var fs = require("fs");
// keep different name for http and https from js engine
var http_global = require("http");
var https_global = require("https");
var url = require("url");

var Logger = require('rcvrcore/Logger').Logger;
var log = new Logger('FileUtils');

/**
 * Load a file from either the filesystem or webservice
 * @param fileUri the full path of the file
 * @returns  a promise that will be fulfilled if the file is found, else it is rejected.
 */
function loadFile(fileUri) {
  return new Promise(function(resolve, reject) {
    var code = [];
    if (fileUri.substring(0, 4) == "http") {
      var options = url.parse(fileUri);
      var req = null;
      var httpCallback = function (res) {
        res.on('data', function (data) {
          code += data;
        });
        res.on('end', function () {
          if( res.statusCode == 200 ) {
            log.message(3, "Got file[" + fileUri + "] from web service");
            resolve(code);
          } else {
            log.error("StatusCode Bad: FAILED to read file[" + fileUri + "] from web service");
            reject(res.statusCode);
          }
        });
      };
      if (fileUri.substring(0, 5) == "https") {
        req = https_global.get(options, httpCallback);
        process._tickCallback();
      }
      else {
        req = http_global.get(options, httpCallback);
        process._tickCallback();
      }
      req.on('error', function (err) {
        log.error("Error: FAILED to read file[" + fileUri + "] from web service");
        reject(err);
      });
      
    }
    else {
      if( fileUri.substring(0,5) === 'file:' ) {
        fileUri = fileUri.substring(5);
        if( fileUri.substring(0,2) === '//') {
          fileUri = fileUri.substring(1);
        }
      }

      fs.readFile(fileUri, function (err, data) {
          if (err) {
              log.error("FAILED to read file[" + fileUri + "] from file system (error=" + err + ")");
              reject(err);
          } else {
              log.message(3, "Got file[" + fileUri + "] from file system");
              resolve(data);
          }
      });
    }
  });
}

// same as above , but do local file, spark permission and CORS check
function loadFileWithSparkPermissionsCheck( accessControl, http1, http2, fileUri) {
  console.log("in loadFileWithSparkPermissionsCheck " + fileUri);
  return new Promise(function(resolve, reject) {
    var code = [];
    if (fileUri.substring(0, 4) == "http") {
      var options = url.parse(fileUri);
      var req = null;
      var httpCallback = function (res) {
        res.on('data', function (data) {
          console.log("Received data");
          code += data;
        });
        res.on('end', function () {
          if( res.statusCode == 200 ) {
            log.message(3, "Got file[" + fileUri + "] from web service");
            resolve(code);
          } else {
            log.error("StatusCode Bad: FAILED to read file[" + fileUri + "] from web service");
            reject(res.statusCode);
          }
        });
      };
      if (fileUri.substring(0, 5) == "https") {
        req = http2.get(options, httpCallback);
      }
      else {
        req = http1.get(options, httpCallback);
      }
      // handling of request failed may be due to spark permissions check
      // CORS denial case enters below error condition handler
      if (null != req)
      {
        req.on('error', function (err) {
          log.error("Error: FAILED to read file[" + fileUri + "] from web service");
          reject(err);
        });
      }
      else
      {
        reject("Access of file not allowed");
      }
    }
    else {
      // do check from access control module for file allowed or disallowed access
      var isAllowed = accessControl.allows(fileUri);
      if (true == isAllowed)
      {
        console.log("local file access allowed for current file !!!");

        if( fileUri.substring(0,5) === 'file:' ) {
          fileUri = fileUri.substring(5);
          if( fileUri.substring(0,2) === '//') {
            fileUri = fileUri.substring(1);
          }
        }

        fs.readFile(fileUri, function (err, data) {
            if (err) {
                log.error("FAILED to read file[" + fileUri + "] from file system (error=" + err + ")");
                reject(err);
            } else {
                log.message(3, "Got file[" + fileUri + "] from file system");
                resolve(data);
            }
        });
      }
      else
      {
        //local file access not allowed
        console.log("local file access not allowed !!!");
        reject("Local file access not allowed");
      }
    }
  });
}

function hasExtension(filePath, extension) {
  var idx = filePath.lastIndexOf(extension);
  return( (idx !== -1) && ((idx + extension.length) === filePath.length) );
}

module.exports = {
  loadFile: loadFile,
  loadFileWithSparkPermissionsCheck:loadFileWithSparkPermissionsCheck,
  hasExtension: hasExtension
};
