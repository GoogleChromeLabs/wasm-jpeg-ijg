/*
 * Copyright 2017 Google LLC. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
// This code is based on the xiph.org Daala comparison page at:
// https://people.xiph.org/~xiphmont/demo/daala/update1-tool2b.shtml
var sizekb = document.getElementById('sizekb');
var left = document.getElementById('leftcontainer');
var right = document.getElementById('rightcontainer');
var offset = {width: right.getBoundingClientRect().width,
              height: right.getBoundingClientRect().height};
var splitx = offset.width*.5;
var splity = offset.height*.5;
var splitx_target = splitx;
var splity_target = splity;
var splitx1 = 0;
var splity1 = 0;
var lefttext = document.getElementById('lefttext');
var righttext = document.getElementById('righttext');
var urlfile;
var timer;
var textheight = lefttext.offsetHeight;
var first=1;

function set_size(img,el) {
    var width = img.width;
    var height = img.height;
    el.style.width = width+"px";
    el.style.height = height+"px";
    el.style.backgroundImage='url(\"'+img.src+'\")';
    if(el==right){
        offset = {width: width, height: height};
        if(first){
            splitx = splitx_target = width*.5;
            splity = splity_target = height*.5;
            first=0;
        }
    }
    set_split();
}

function set_split(){
    if(!timer){
        timer = setInterval( function(){
            splitx1*=.5;
            splity1*=.5;
            splitx1 += (splitx_target-splitx)*.1;
            splity1 += (splity_target-splity)*.1;
            
            splitx += splitx1;
            splity += splity1;
            
            if(Math.abs(splitx-splitx_target)<.5)
                splitx=splitx_target;
            if(Math.abs(splity-splity_target)<.5)
                splity=splity_target;

            left.style.width=splitx+"px";
            lefttext.style.right = (offset.width-splitx)+"px";
            lefttext.style.bottom = (offset.height-splity)+"px";
            righttext.style.left = (splitx+1)+"px";
            righttext.style.bottom = (offset.height-splity)+"px";
            
            if(splitx==splitx_target && splity==splity_target){
                clearInterval(timer);
                timer=null;
            }
        }, 20);
    }        
}

function set_image(container){
    var image = new Image();
    container.style.background="gray";
    container.style.backgroundImage="";
    image.onload = function(){set_size(image,container)};
    image.src = urlfile;
}

function set_left(){
    var name = "original";
    set_image(left);
    lefttext.innerHTML=name+"&nbsp;&larr;";
    textheight = lefttext.offsetHeight;
}

function set_right(name){
    set_image(right);
    righttext.innerHTML=name;
}

var gQuality = 5;

function set_file(){
    urlfile = "images/js-wa-900.jpg";
    first=1;
    set_right(gQuality + "");
    set_left();
}

function set_blob(blob) {
    urlfile = blob;
    first = 1;
    set_left();
}

function set_right_array(imgAsArray, name) {
    if (wasm_loaded == false)
        return;
    var len = imgAsArray.byteLength;
    var buf = Module._malloc(len);
    Module.HEAPU8.set(new Uint8Array(imgAsArray), buf);
    var size = Module._jpg_transcode(buf, len, gQuality);
    var result = new Uint8Array(Module.HEAPU8.buffer, buf, len);
    urlfile = makeBlobUrl(result);
    set_right(name);
    sizekb.innerHTML = "" + (size / 1024.0).toFixed(2);
    Module._free(buf);
}

function set_jpeg_quality(quality) {
    gQuality = quality;
    righttext.innerHTML="&rarr;&nbsp;Q:&nbsp;"+quality;
    set_right_array(fileAsArray, "&rarr;&nbsp;Q:&nbsp;"+quality);
}

function movesplit(event){
    var offset = right.getBoundingClientRect();
    splitx_target = event.clientX-offset.left;
    splity_target = event.clientY-offset.top;
    if(splitx_target<0)splitx_target=0;
    if(splity_target<textheight)splity_target=textheight;
    if(splitx_target>=offset.width)splitx_target=offset.width-1;
    if(splity_target>=offset.height)splity_target=offset.height-1;
    set_split();
}

function sticksplit(event){
    stick = !stick;
    if(!stick){
        righttext.style.backgroundColor="rgba(0,0,0,.4)";
        lefttext.style.backgroundColor="rgba(0,0,0,.4)";
        movesplit(event);
    }else{
        righttext.style.backgroundColor="rgba(0,0,0,0)";
        lefttext.style.backgroundColor="rgba(0,0,0,0)";
    }
}

set_file();
set_split();
right.addEventListener( "mousemove", movesplit, false);
right.addEventListener( "touchstart", movesplit, false);
right.addEventListener( "touchmove", movesplit, false);
righttext.style.backgroundColor="rgba(0,0,0,.3)";
lefttext.style.backgroundColor="rgba(0,0,0,.3)";

