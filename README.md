# AV1 video decoder in WASM demo

In-browser AV1 video decoder demo for experimenting with codecs.

## Prerequisites:
* Installed emscripten SDK, available at https://github.com/juj/emsdk
* some version of 'make' installed

## To build:
```
make
```

This will generate 'decode-av1.js' and 'decode-av1.wasm'. Load 'index.html' from a local web server and enjoy!

## To build test harness:
```
make testavx
```

This builds a small test harness to decode from the command line, dumping a few frames as YUV files.

Licensed under the Apache License, version 2.0.

This is not an official Google product.
