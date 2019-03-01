#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

ofColor orangeRed;
ofColor gradientArray[255];

void ofApp::setup() {
	ofSetVerticalSync(true);
	cam.setup(640, 480);
	
	tracker.setup();
	tracker.setRescale(.5);

	classifier.load("expressions");

	orangeRed = ofColor(255, 70, 47);
	calculateGradient();
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
	ofPushStyle();

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
	
	ofEnableSmoothing();
	ofSetLineWidth(5.0f);
	ofSetColor(ofColor::white);

	ofTexture tex = cam.getTexture();
	ofPixels pix;
	tex.readToPixels(pix);

	// We want the circle to be slightly larger than the max dimension (which should be height) 
	// of the bounding box that contains the rough approximation of the face + hairline
	float maxDimension = MAX(expandedHeight, boundingBox.width);
	float circleDiameter = maxDimension * 1.5f;
	float circleRadius = circleDiameter / 2.0f;
	ofVec2f circleCenter = expandedBound.getCenter();

	// todo: figure out if these values could/should ever be a different value than 
	// the initial values passed as parameters into cam.setup(w, h);
	int pixelWidth = pix.getWidth();
	int pixelHeight = pix.getHeight();

	for (int i = 0; i < pixelWidth; i++) {
		for (int j = 0; j < pixelHeight; j++) {
			ofColor color = pix.getColor(i, j);
			// https://en.wikipedia.org/wiki/Grayscale#Luma_coding_big_gameas
			float luminance = (0.3 * color.r) + (0.59 * color.g) + (0.11 * color.b);
			// Pixels within the head-enscapsulating circle are grayscale, 
			// those outside of the circle are monochromotatic orange-red
			bool isPixelEncircled = ofDist(i, j, circleCenter.x, circleCenter.y) < circleRadius;
			ofColor pixelColor = (isPixelEncircled) ? ofColor(luminance) : gradientArray[(int)luminance];
			pix.setColor(i, j, pixelColor);
		}
	}

	tex.loadData(pix);
	tex.draw(0,0);

	ofSetCircleResolution(100);
	ofDrawCircle(circleCenter, circleRadius);
	
	ofPopStyle();
	
	ofPushMatrix();
	ofTranslate(5, 10);

	int w = 250, h = 12;

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

// Adapted from http://blog.72lions.com/blog/2015/7/7/duotone-in-js
void ofApp::calculateGradient()
{
	// Creates a gradient of 255 colors between orange-red and black
	for (int i = 0; i < 255; i++) {
		float ratio = (float)i / 255.0f;
		float rA = floor(orangeRed.r * ratio);
		float gA = floor(orangeRed.g * ratio);
		float bA = floor(orangeRed.b * ratio);		
		gradientArray[i] = ofColor(rA, gA, bA);
	}
}