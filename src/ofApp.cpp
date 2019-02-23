#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

void ofApp::setup() {
	ofSetVerticalSync(true);
	cam.setup(640, 480);
	
	tracker.setup();
	tracker.setRescale(.5);

	classifier.load("expressions");
}

void ofApp::update() {
	cam.update();
	if(cam.isFrameNew()) {
		if(tracker.update(toCv(cam))) {
			classifier.classify(tracker);
		}		
	}
}

void ofApp::draw() {
	ofSetColor(255);
	cam.draw(0, 0);
	
	int w = 250, h = 12;
	ofPushStyle();
	ofPushMatrix();
	ofTranslate(5, 10);

	// Display information about pose (mouth open, direction facing)
	ofSetColor(ofColor::black);
	ofDrawRectangle(0, 0, w, h);
	ofSetColor(ofColor::white);
	ofDrawBitmapString(getMouthStateString(), 5, 9);
	ofTranslate(0, h + 5);

	ofSetColor(ofColor::black);
	ofDrawRectangle(0, 0, w, h);
	ofSetColor(ofColor::white);
	ofDrawBitmapString(getDirectionString(), 5, 9);
	ofTranslate(0, h + 5);

	ofPopMatrix();
	ofPopStyle();

	ofPushStyle();	
	ofNoFill();

	ofPolyline faceOutline = tracker.getImageFeature(ofxFaceTracker::FACE_OUTLINE);
	auto boundingBox = faceOutline.getBoundingBox();
	boundingBox.standardize();
	
	// Approximating the bounding box of the entire face as a being 1/3 taller than the bounding box provided 
	// by the FACE_OUTLINE image feature that enscribes the top of the eyebrows and the bottom of the chin
	float extra = boundingBox.height * 0.5f;
	float expandedHeight = boundingBox.height + extra;
	float expandedY = boundingBox.y - (extra);
	ofRectangle expandedBound = ofRectangle(boundingBox.x, expandedY, boundingBox.width, expandedHeight);
	
	ofSetLineWidth(5.0f);
	ofSetColor(ofColor::white);
	
	// We want the circle to be slightly larger than the max dimension (which should be height) 
	// of the bounding box that contains the rough approximation of the face + hairline
	float maxDimension = MAX(expandedHeight, boundingBox.width);
	float circleDiameter = maxDimension * 1.5f;
	ofDrawCircle(expandedBound.getCenter(), circleDiameter/2.0f);
	
	ofPopStyle();
}

void ofApp::keyPressed(int key) {}

string ofApp::getDirectionString()
{
	ofxFaceTracker::Direction direction = tracker.getDirection();
	string str = "direction facing = ";

	switch (direction) {
		case ofxFaceTracker::FACING_RIGHT: str += "right";
			break;
		case ofxFaceTracker::FACING_LEFT: str += "left";
			break;
		case ofxFaceTracker::FACING_FORWARD: str += "forward";
			break;
		default: str += "none";
	}	
	return str;
}

string ofApp::getMouthStateString()
{
	int primary = classifier.getPrimaryExpression();
	bool mouthOpen = classifier.getDescription(primary) == "open mouth";
	string mouthState = mouthOpen ? "open" : "closed";
	return "mouth state = " + mouthState;
}
