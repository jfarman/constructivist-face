#pragma once

#include "ofMain.h"
#include "ofxFaceTracker.h"

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
	
	ofVideoGrabber cam;
	ofxFaceTracker tracker;
	ExpressionClassifier classifier;

protected:
	string getDirectionString();
	ofColor orangeRed;
	ofColor gradientArray[255];
	void calculateGradient();
	void drawCallout();
};
