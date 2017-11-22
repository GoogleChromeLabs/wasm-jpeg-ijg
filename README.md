# JPEG codec in WASM demo

In-browser JPEG Codec demo for experimenting with different quality settings.

## Prerequisites:
* Installed emscripten SDK, available at https://github.com/juj/emsdk
* some version of 'make' installed

## To build:
```
make
```

This will generate 'jpegsquash.js' and 'jpegsquash.wasm'. Load 'index.html' from a local web server and enjoy!

## To build test harness:
```
make transcode
```

This builds a small test harness to transcode from the command line.

Licensed under the Apache License, version 2.0.

This is not an official Google product.
