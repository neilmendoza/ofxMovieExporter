# Usage
Declare exporter:

```cpp
Apex::ofxMovieExporter movieExporter;
```

Setup exporter, see ofxMovieExporter.h for further setup options:

```cpp
movieExporter.setup();
```

Start capturing:

```cpp
movieExporter.record();
```

Stop capturing:

```cpp
movieExporter.stop();
```

New movies will be saved to data folder each time **record()** then **stop()** are called named capture0.mp4, capture1.mp4 and so on.

# Dependencies
Addon is based on avlib version 371888c from git://git.videolan.org/ffmpeg.git

OSX binaries were compiled as LGPL.  Windows binaries were downloaded from here - http://ffmpeg.zeranoe.com/builds/ - and include x264 and hence are GPL.  If you feel in the mood for some Windows fun, compile away and I'll update.

# TODO
* Use more efficient pixel capture than glReadPixels at the end of rendering the frame, maybe pixel buffer - http://stackoverflow.com/questions/5142990/screen-capture-with-open-gl-using-glreadpixels
* Add queue mechanism that doesn't involve locking
* Add audio
* Remove unnecessary libs - probably avdevice, avfilter, avutil and postproc
* Fix timing issues
