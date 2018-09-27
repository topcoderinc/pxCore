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

var isDuk = (typeof timers != "undefined")?true:false;
var isV8 = (typeof _isV8 != "undefined")?true:false;

// FIXME !!!!!!!!!! duktape merge hack
if (!isDuk && !isV8) {
  var JSZip = require("jszip");
}

var Logger = require('rcvrcore/Logger').Logger;
var log = new Logger('FileUtils');

function FileArchive(filePath, nativeFileArchive) {
  this.filePath = filePath;
  this.baseFilePath = filePath.substring(0, filePath.lastIndexOf('/'));
  this.numEntries = 0;
  this.directory = {};
  this.jar = null;
  this.nativeFileArchive = nativeFileArchive;
}

FileArchive.prototype.removeFile = function(filename) {
  if( this.directory.hasOwnProperty(filename) ) {
    this.directory[filename] = null;
    delete this.directory[filename];
    --this.numEntries;
    return true;
  } else {
    return false;
  }
};

FileArchive.prototype.getFileContents = function(filename) {
  log.message(10, "FileArchive::getFileContents<" + filename + ">");
  if( this.directory.hasOwnProperty(filename) ) {
    return this.directory[filename];
  } else {
    var fileContents = null;
    if( this.nativeFileArchive !== undefined ) {
      log.message(10, "Looking for file in native archive: " + filename);
      fileContents = this.nativeFileArchive.getFileAsString(filename);
    }
    if( fileContents === undefined || fileContents === null ) {
      console.error("No file contents in [" + this.baseFilePath + "] for " + filename);
      return null;
    }

    log.message(10, "Found file <" + filename + ">: " + "ok");

    return fileContents;
  }
};

FileArchive.prototype.addFile = function(filename, contents) {
  var wasNewFile = true;
  if( this.directory.hasOwnProperty(filename) ) {
    wasNewFile = false;
  }

  this.directory[filename] = contents;
  ++this.numEntries;
  return wasNewFile;
};

FileArchive.prototype.loadFromJarData = function(dataBuf) {
  var jar = new JSZip(dataBuf);
  this.processJar(jar);
  this.loadedJarFile = true;
};

FileArchive.prototype.processJar = function(jar) {
  for(var file in jar.files) {
    var fileEntry = jar.files[file];
    if( fileEntry.options.dir === true ) {
      continue;
    }
    this.addArchiveEntry(file, fileEntry.asText());
  }
};

FileArchive.prototype.addArchiveEntry = function(filename, data) {
  ++this.numEntries;
  this.directory[filename] = data;
};

FileArchive.prototype.hasFileContents = function(filename) {
  var hasFile = this.directory.hasOwnProperty(filename);
  if( hasFile === false ) {
    if (this.nativeFileArchive !== undefined) {
      hasFile = isFileInList(filename, this.nativeFileArchive.fileNames);
    }
  }

  log.message(10, "hasFileContents(" + filename + ") = " + hasFile);

  return hasFile;
};

function isFileInList(fileName, list) {
  for(var k = 0; k < list.length; ++k) {
    if( list[k] === fileName ) {
      return true;
    }
  }

  return false;
}

module.exports = FileArchive;
