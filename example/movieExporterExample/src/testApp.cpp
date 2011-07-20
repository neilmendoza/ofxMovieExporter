#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup()
{
	ofSetFrameRate(60);
	ofBackgroundHex(0x000000);
	testImage.loadImage("kate-1024-768.jpg");
	movieExporter.setup();
}

//--------------------------------------------------------------
void testApp::update()
{
}

//--------------------------------------------------------------
void testApp::draw()
{
	testImage.draw(0, 0, ofGetWidth(), ofGetHeight());
	ofEllipse(.5f * ofGetWidth() + 200.f * cosf(ofGetElapsedTimef()), .5f * ofGetHeight() + 200.f * sinf(ofGetElapsedTimef()), 100, 100);
	int size = max(1000 - ofGetFrameNum(), 0);
	ofEllipse(.5f * ofGetWidth(), .5f * ofGetHeight(), size, size);
	ofDrawBitmapString(ofToString(ofGetFrameRate()), 10, 10);
	ofDrawBitmapString(ofToString(ofGetElapsedTimef()), 10, 30);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key)
{
	switch (key)
	{
		case ' ':
			if (movieExporter.isRecording()) movieExporter.stop();
			else movieExporter.record();
			break;
	}
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}