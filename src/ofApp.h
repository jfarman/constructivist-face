#pragma once

#include "ofMain.h"
#include "ofxFaceTracker.h"
#include <math.h>.

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
	void drawCallout(ofPoint circleCenter, float circleRadius);
	void drawSideFrames(ofRectangle leftFrame, ofRectangle rightFrame);
	ofPoint getIntersection(ofPoint pointA, ofPoint pointB, ofPoint circleCenter, float circleRadius);
	bool isWithinTriangle(ofPoint s, ofPoint a, ofPoint b, ofPoint c);
	
	//void ofxQuadWarp(ofTexture tex, ofPoint lt, ofPoint rt, ofPoint rb, ofPoint lb, int rows, int cols);
	//void drawText();

	ofPoint mouthCenter;

	ofPolyline lineA;
	ofPolyline lineB;
	ofPolyline offsetLineA;
	ofPolyline offsetLineB;

	ofPoint startPoint;
	ofPoint endPoint;
	float lineSlope;
	float lineIntercept;
	float currentTriangleHeight;

	float circleRadius;
	ofPoint circleCenter;

	float fakeMaxPoint;
	ofPoint offsetMouthCenter;
};
