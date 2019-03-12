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
	ofColor orangeGradientArray[255];

	void calculateGradient();
	void drawCallout(ofPoint circleCenter, float circleRadius);
	ofPoint getIntersection(ofPoint pointA, ofPoint pointB, ofPoint circleCenter, float circleRadius);
	bool isWithinTriangle(ofPoint s, ofPoint a, ofPoint b, ofPoint c);

	static ofPoint ofxLerp(ofPoint start, ofPoint end, float amt);
	static int ofxIndex(float x, float y, float w);
	static bool sortByX(const ofPoint &a, const ofPoint &b);
	static bool sortByY(const ofPoint &a, const ofPoint &b);

	void ofxQuadWarp(ofTexture tex, ofPoint lt, ofPoint rt, ofPoint rb, ofPoint lb, int rows, int cols);
	void drawText(bool isLeadingText);

	ofPoint mouthCenter;

	ofPolyline lineA;
	ofPolyline lineB;
	ofPolyline offsetLineA;
	ofPolyline offsetLineB;
	ofPolyline bisectingLine;

	ofPoint startPoint;
	ofPoint endPoint;
	float lineSlope;
	float lineIntercept;
	float currentTriangleHeight;

	float circleRadius;
	ofPoint circleCenter;

	float fakeMaxPoint;
	ofPoint offsetMouthCenter;

	ofTrueTypeFont font;
	vector <ofFbo> charsAsFBOs;
	string displayString;
	void updateString(string newString);

	float leadingTextTimer;
	float trailingTextTimer;
	
	bool shouldDrawLeadingText;
	bool shouldDrawTrailingText;
};
