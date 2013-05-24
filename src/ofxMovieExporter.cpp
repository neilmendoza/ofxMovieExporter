/*
 *  ofxMovieExporter.cpp
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
#include "ofxMovieExporter.h"
#include "ofThread.h"

namespace itg
{
	const string ofxMovieExporter::FILENAME_PREFIX = "capture";
	const string ofxMovieExporter::CONTAINER = "mp4";

	ofxMovieExporter::ofxMovieExporter() {
		outputFormat = NULL;
		formatCtx = NULL;
		videoStream = NULL;
		
		codec = NULL;
		codecCtx = NULL;
		convertCtx = NULL;
		
		inPixels = NULL;
		outPixels = NULL;
		encodedBuf = NULL;

		inFrame = NULL;
		outFrame = NULL;

		posX = 0;
		posY = 0;
		inW = ofGetWidth();
		inH = ofGetHeight();
		outW = ofGetWidth();
		outH = ofGetHeight();
		
		bool usePixelSource = false;
		pixelSource = NULL;
	}

	void ofxMovieExporter::setup(
		int outW,
		int outH,
		int bitRate,
		int frameRate,
		CodecID codecId,
		string container)
	{
		if (outW % 2 == 1 || outH % 2 == 1) ofLog(OF_LOG_ERROR, "ofxMovieExporter: Resolution must be a multiple of 2");

		this->outW = outW;
		this->outH = outH;
		this->frameRate = frameRate;
		this->bitRate = bitRate;
		this->codecId = codecId;
		this->container = container;

		frameInterval = 1.f / (float)frameRate;

		// HACK HACK HACK
		// Time not syncing
		// probably related to codec ticks_per_frame
		frameInterval /= 3.f;
		recording = false;
		numCaptures = 0;

		// do one time encoder set up
		av_register_all();
		convertCtx = sws_getContext(inW, inH, PIX_FMT_RGB24, outW, outH, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

		allocateMemory();
	}

	ofxMovieExporter::~ofxMovieExporter()
	{
		if (recording) finishRecord();

		stopThread();
		clearMemory();
	}

	void ofxMovieExporter::record(string filePrefix, string folderPath)
	{
		initEncoder();

		ostringstream oss;
		oss << folderPath;
		if (folderPath != "" && (folderPath[folderPath.size()-1] != '/' && folderPath[folderPath.size()-1] != '\\'))
            oss << "/";
		oss << filePrefix << numCaptures << "." << container;
		outFileName = oss.str();

		// open the output file
		if (url_fopen(&formatCtx->pb, ofToDataPath(outFileName).c_str(), URL_WRONLY) < 0)
			ofLog(OF_LOG_ERROR, "ofxMovieExporter: Could not open file %s", ofToDataPath(outFileName).c_str());

		ofAddListener(ofEvents().draw, this, &ofxMovieExporter::checkFrame);

		// write the stream header, if any
		av_write_header(formatCtx);

		lastFrameTime = 0;
		frameNum = 0;
		recording = true;
#ifdef _THREAD_CAPTURE
		startThread(true, false);
#endif
	}

	void ofxMovieExporter::stop()
	{
		ofRemoveListener(ofEvents().draw, this, &ofxMovieExporter::checkFrame);
		recording = false;
		numCaptures++;
#ifndef _THREAD_CAPTURE
		finishRecord();
#endif
	}

	void ofxMovieExporter::setRecordingArea(int x, int y, int w, int h)
	{
		posX = x;
		posY = y;
		inW = w;
		inH = h;
	}

	void ofxMovieExporter::setRecordingArea(ofRectangle& rect)
	{
		posX = rect.x;
		posY = rect.y;
		inW = rect.width;
		inH = rect.height;
	}

	void ofxMovieExporter::resetRecordingArea()
	{
		posX = 0;
		posY = 0;
		inW = ofGetViewportWidth();
		inH = ofGetViewportHeight();
	}

	ofRectangle ofxMovieExporter::getRecordingArea()
	{
		return ofRectangle(posX, posY, inW, inH);
	}

	void ofxMovieExporter::setPixelSource(unsigned char* pixels, int w, int h)
	{
		if (isRecording())
			stop();
			
		if (pixels == NULL)
		{
			ofLog(OF_LOG_ERROR, "ofxMovieExporter: Could not set NULL pixel source");
			return;
		}
		pixelSource = pixels;
		inW = w;
		inH = h;
		usePixelSource = true;
		
		// resetup encoder etc
		setup(outW, outH, bitRate, frameRate, codecId, container);
	}

	void ofxMovieExporter::resetPixelSource()
	{
		usePixelSource = false;
		pixelSource = NULL;
		inW = ofGetViewportWidth();
		inH = ofGetViewportHeight();
		
		// resetup encoder etc
		setup(outW, outH, bitRate, frameRate, codecId, container);
	}
		
	int ofxMovieExporter::getNumCaptures()
	{
		return numCaptures;
	}
	
	void ofxMovieExporter::resetNumCaptures()
	{
		numCaptures = 0;
	}
		
// PRIVATE

	void ofxMovieExporter::finishRecord()
	{
		av_write_trailer(formatCtx);

		// free the encoder
		avcodec_close(codecCtx);
		for(int i = 0; i < formatCtx->nb_streams; i++)
		{
			av_freep(&formatCtx->streams[i]->codec);
			av_freep(&formatCtx->streams[i]);
		}
		av_free(formatCtx);
		formatCtx = NULL;
		//url_fclose(formatCtx->pb);
	}

#ifdef _THREAD_CAPTURE
	void ofxMovieExporter::threadedFunction()
	{
		while (isThreadRunning())
		{
			if (!frameQueue.empty())
			{
				float start = ofGetElapsedTimef();
				frameQueueMutex.lock();
				inPixels = frameQueue.front();
				frameQueue.pop_front();
				frameQueueMutex.unlock();

				encodeFrame();

				frameMemMutex.lock();
				frameMem.push_back(inPixels);
				frameMemMutex.unlock();
				if (ofGetElapsedTimef() - start < frameInterval) ofSleepMillis(1000.f * (frameInterval - (ofGetElapsedTimef() - start)));
			}
			else if (!recording)
			{
				finishRecord();
				stopThread();
			}
		}
	}
#endif

	void ofxMovieExporter::checkFrame(ofEventArgs& args)
	{
		if (ofGetElapsedTimef() - lastFrameTime >= frameInterval)
		{
#ifdef _THREAD_CAPTURE
			unsigned char* pixels;
			if (!frameMem.empty())
			{
				frameMemMutex.lock();
				pixels = frameMem.back();
				frameMem.pop_back();
				frameMemMutex.unlock();
			}
			else
			{
				pixels = new unsigned char[inW * inH * 3];
			}
			
			if (!usePixelSource)
			{
				// this part from ofImage::saveScreen
				int screenHeight =	ofGetViewportHeight(); // if we are in a FBO or other viewport, this fails: ofGetHeight();
				int screenY = screenHeight - posY;
				screenY -= inH; // top, bottom issues
				
		 		glReadPixels(posX, screenY, inW, inH, GL_RGB, GL_UNSIGNED_BYTE, pixels);
			}
			else
			{
				memcpy(pixels, pixelSource, inW * inH * 3);
			}
			
			frameQueueMutex.lock();
			frameQueue.push_back(pixels);
			frameQueueMutex.unlock();
#else
			if (!usePixelSource) {
				// this part from ofImage::saveScreen
				int screenHeight =	ofGetViewportHeight(); // if we are in a FBO or other viewport, this fails: ofGetHeight();
				int screenY = screenHeight - posY;
				screenY -= inH; // top, bottom issues
				
				glReadPixels(posX, screenY, inW, inH, GL_RGB, GL_UNSIGNED_BYTE, inPixels);
			}
			else
			{
				memcpy(inPixels, pixelSource, inW * inH * 3);
			}
			encodeFrame();
#endif
			lastFrameTime = ofGetElapsedTimef();
		}
	}

	void ofxMovieExporter::encodeFrame()
	{
		
		avpicture_fill((AVPicture*)inFrame, inPixels, PIX_FMT_RGB24, inW, inH);
		avpicture_fill((AVPicture*)outFrame, outPixels, PIX_FMT_YUV420P, outW, outH);

		// intentionally flip the image to compensate for OF flipping if reading from the screen
		if (!usePixelSource)
		{
			inFrame->data[0] += inFrame->linesize[0] * (inH - 1);
			inFrame->linesize[0] = -inFrame->linesize[0];
		}
		
		//perform the conversion for RGB to YUV and size
		sws_scale(convertCtx, inFrame->data, inFrame->linesize, 0, inH, outFrame->data, outFrame->linesize);

		int outSize = avcodec_encode_video(codecCtx, encodedBuf, ENCODED_FRAME_BUFFER_SIZE, outFrame);
		if (outSize > 0)
		{
			AVPacket pkt;
			av_init_packet(&pkt);
			//pkt.pts = av_rescale_q(codecCtx->coded_frame->pts, codecCtx->time_base, videoStream->time_base);
			//if(codecCtx->coded_frame->key_frame) pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.pts = frameNum;//ofGetFrameNum();//codecCtx->coded_frame->pts;
			pkt.flags |= AV_PKT_FLAG_KEY;
			pkt.dts = pkt.pts;
			pkt.stream_index = videoStream->index;
			pkt.data = encodedBuf;
			pkt.size = outSize;
			av_write_frame(formatCtx, &pkt);
		}
		frameNum++;
	}

	void ofxMovieExporter::allocateMemory()
	{
		// clear if we need to reallocate
		if(inPixels)
			clearMemory();
		
		// allocate input stuff
#ifdef _THREAD_CAPTURE
		//unsigned char* initFrameMem = new unsigned char[inW * inH * 3 * INIT_QUEUE_SIZE];
		for (int i = 0; i < INIT_QUEUE_SIZE; i++)
		{
			//frameMem.push_back(initFrameMem + inW * inH * 3 * i);
			frameMem.push_back(new unsigned char[inW * inH * 3]);
		}
#else
		inPixels = new unsigned char[inW * inH * 3];
#endif
		inFrame = avcodec_alloc_frame();

		// allocate output stuff
		int outSize = avpicture_get_size(PIX_FMT_YUV420P, outW, outH);
		outPixels = (unsigned char*)av_malloc(outSize);
		outFrame = avcodec_alloc_frame();

		encodedBuf = (unsigned char*)av_malloc(ENCODED_FRAME_BUFFER_SIZE);
	}

	void ofxMovieExporter::clearMemory() {
		// clear input stuff
#ifdef _THREAD_CAPTURE
		for (int i = 0; i < frameMem.size(); i++)
		{
			delete[] frameMem[i];
		}
		for (int i = 0; i < frameQueue.size(); i++)
		{
			delete[] frameQueue[i];
		}
#else
		delete[] inPixels;
#endif
		inPixels = NULL;
		
		av_free(inFrame);
		av_free(outFrame);
		av_free(encodedBuf);
		av_free(outPixels);

		inFrame = NULL;
		outFrame = NULL;
		encodedBuf = NULL;
		outPixels = NULL;
	}

	void ofxMovieExporter::initEncoder()
	{
		/////////////////////////////////////////////////////////////
		// find codec
		codec = avcodec_find_encoder(codecId);
		if (!codec) ofLog(OF_LOG_ERROR, "ofxMovieExporter: Codec not found");

		////////////////////////////////////////////////////////////
		// auto detect the output format from the name. default is mpeg.
		ostringstream oss;
		oss << "amovie." << container;
		outputFormat = av_guess_format(NULL, oss.str().c_str(), NULL);
		if (!outputFormat) ofLog(OF_LOG_ERROR, "ofxMovieExporter: Could not guess output container for an %s file (ueuur!!)", container.c_str());
		// set the format codec (the format also has a default codec that can be read from it)
		outputFormat->video_codec = codec->id;

		/////////////////////////////////////////////////////////////
		// allocate the format context
		formatCtx = avformat_alloc_context();
		if (!formatCtx) ofLog(OF_LOG_ERROR, "ofxMovieExporter: Could not allocate format context");
		formatCtx->oformat = outputFormat;

		/////////////////////////////////////////////////////////////
		// set up the video stream
		videoStream = av_new_stream(formatCtx, 0);

		/////////////////////////////////////////////////////////////
		// init codec context
		codecCtx = videoStream->codec;
		codecCtx->bit_rate = bitRate;
		codecCtx->width = outW;
		codecCtx->height = outH;

		codecCtx->time_base.num = 1;//codecCtx->ticks_per_frame;
		codecCtx->time_base.den = frameRate;
		videoStream->time_base = codecCtx->time_base;


		codecCtx->gop_size = 10; /* emit one intra frame every ten frames */
		codecCtx->pix_fmt = PIX_FMT_YUV420P;

		if (codecCtx->codec_id == CODEC_ID_MPEG1VIDEO)
		{
			/* needed to avoid using macroblocks in which some coeffs overflow
			 this doesnt happen with normal video, it just happens here as the
			 motion of the chroma plane doesnt match the luma plane */
			codecCtx->mb_decision=2;
		}
		// some formats want stream headers to be seperate
		if(!strcmp(formatCtx->oformat->name, "mp4") || !strcmp(formatCtx->oformat->name, "mov") || !strcmp(formatCtx->oformat->name, "3gp"))
			codecCtx->flags |= CODEC_FLAG_GLOBAL_HEADER;

		// set the output parameters (must be done even if no parameters).
		if (av_set_parameters(formatCtx, NULL) < 0)	ofLog(OF_LOG_ERROR, "ofxMovieExproter: Could not set format parameters");

		// open codec
		if (avcodec_open(codecCtx, codec) < 0) ofLog(OF_LOG_ERROR, "ofxMovieExproter: Could not open codec");
	}
}
