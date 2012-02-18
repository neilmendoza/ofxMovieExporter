/*
 *  ofxMovieExporter.h
 *
 *  Copyright (c) 2011, Neil Mendoza, http://www.neilmendoza.com
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of 16b.it nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 */
#pragma once

#define _THREAD_CAPTURE

#include "ofMain.h"

// needed for gcc on win
#ifdef TARGET_WIN32
    #ifndef INT64_C
        #define INT64_C(c) (c ## LL)
        #define UINT64_C(c) (c ## ULL)
    #endif
#endif

extern "C"
{
    #include <avcodec.h>
    #include <avformat.h>
    #include <swscale.h>
	#include <mathematics.h>
}

namespace itg
{
	class ofxMovieExporter
#ifdef _THREAD_CAPTURE
		: public ofThread
#endif
	{
	public:
		static const int ENCODED_FRAME_BUFFER_SIZE = 500000;
		// defaults
		static const int BIT_RATE = 4000000;
		static const int FRAME_RATE = 25;
		static const int OUT_W = 640;
		static const int OUT_H = 480;
		static const int INIT_QUEUE_SIZE = 50;
		static const CodecID CODEC_ID = CODEC_ID_MPEG4;
		static const string FILENAME_PREFIX;
		static const string CONTAINER;

		ofxMovieExporter();
		~ofxMovieExporter();
		// tested so far with...
		// codecId = CODEC_ID_MPEG4, container = "mp4"
		// codecId = CODEC_ID_MPEG2VIDEO, container = "mov"
		void setup(int outW = OUT_W, int outH = OUT_H, int bitRate = BIT_RATE, int frameRate = FRAME_RATE, CodecID codecId = CODEC_ID, string container = CONTAINER);
		void record(string filePrefix=FILENAME_PREFIX, string folderPath="");
		void stop();
		bool isRecording() const;

        // set the recording area
        // x, y is the upper left corner of the recording area, default: 0, 0
        // w x h is the area size, default: viewport width x height
        void setRecordingArea(int x, int y, int w, int h);
        void setRecordingArea(ofRectangle& rect);
        
        // reset the recording area to the size of the current viewport (screen, FBO, etc)
        void resetRecordingArea();
        
        // get the recording area as a rectangle
		ofRectangle getRecordingArea();
		
		// set an external pixel source, assumes 3 Byte RGB
		// also sets the recording size but does not crop to the recording area
		void setPixelSource(unsigned char* pixels, int w, int h);
		
		// reset the pixel source and record from the screen
		// also resets the recording size to the viewport width
		void resetPixelSource();
		
		// get the number files that have been captured so far
		int getNumCaptures();
		
		// reset the filename counter back to 0
		void resetNumCaptures();
		
		// get the recording size
		inline int getRecordingWidth() 	{return outW;}
		inline int getRecordingHeight() {return outH;}

	private:
#ifdef _THREAD_CAPTURE
		void threadedFunction();
		deque<unsigned char*> frameQueue;
		deque<unsigned char*> frameMem;
		ofMutex frameQueueMutex;
		ofMutex frameMemMutex;
#endif
		void initEncoder();
		void allocateMemory();
		void clearMemory();

		void checkFrame(ofEventArgs& args);
		void encodeFrame();
		void finishRecord();

		string container;
		CodecID codecId;

		bool recording;
		int numCaptures;
		int frameRate;
		int bitRate;
		float frameInterval;
		float lastFrameTime;
		int frameNum;
		string outFileName;

		AVOutputFormat* outputFormat;
		AVFormatContext* formatCtx;
		AVStream* videoStream;

		AVCodec* codec;
		AVCodecContext* codecCtx;

		SwsContext* convertCtx;

		unsigned char* inPixels;
		unsigned char* outPixels;
		unsigned char* encodedBuf;

		AVFrame* inFrame;
		AVFrame* outFrame;

		int posX, posY;
		int inW, inH;
		int outW, outH;
		
		bool usePixelSource;
		unsigned char* pixelSource;
	};

	inline bool ofxMovieExporter::isRecording() const { return recording; }
}

namespace Apex = itg;
