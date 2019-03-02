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
	ofPoint circleCenter = expandedBound.getCenter();

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

	// todo: get mouth state from ofxFaceTracker jaw openness gesture
	int primary = classifier.getPrimaryExpression();
	bool mouthOpen = classifier.getDescription(primary) == "open mouth";
	
	if (mouthOpen) drawCallout(circleCenter, circleRadius);
	
	ofPopStyle();
	
	ofPushMatrix();
	ofTranslate(5, 10);

	int w = 250, h = 12;

	// Display information about pose (mouth open, direction facing)
	ofSetColor(ofColor::black);
	ofDrawRectangle(0, 0, w, h);
	ofSetColor(ofColor::white);
	string suffix = (mouthOpen) ? "open" : "closed";
	ofDrawBitmapString("mouth state = " + suffix, 5, 9);
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

void ofApp::drawCallout(ofPoint circleCenter, float circleRadius) {
	// Use nose bridge feature to approximate face tilt
	ofPolyline noseBridge = tracker.getImageFeature(ofxFaceTracker::NOSE_BRIDGE);
	// If there are no points associated with the nose bridge feature, just return
	if (noseBridge.size() == 0) return;
		
	ofPoint firstPoint = noseBridge[0];
	ofPoint mouthCenter = tracker.getImageFeature(ofxFaceTracker::INNER_MOUTH).getCentroid2D();
	ofPoint lastPoint = mouthCenter;

	float slope = (lastPoint.x - firstPoint.x) / (lastPoint.y - firstPoint.y);
	float intercept = lastPoint.x - (slope*lastPoint.y);

	ofPoint endPoint = ofPoint(slope * 640 + intercept, 640); // todo: get width

	float halfVertex = 45.0 / 2.0f;
	float triangleHeight = ofDist(mouthCenter.x, mouthCenter.y, endPoint.x, endPoint.y);
	float triangleLength = triangleHeight / cos(ofDegToRad(halfVertex));

	ofVec2f directionVector = ofVec2f(lastPoint.x - firstPoint.x, lastPoint.y - firstPoint.y);
	ofVec2f normalized = directionVector.getNormalized();
	int angle = normalized.angle(ofVec2f(0, -1));

	double addRadians = ofDegToRad(angle + halfVertex);
	double subRadians = ofDegToRad(angle - halfVertex);

	float offsetMouthY = mouthCenter.y - 5; // todo: figure out correct offset value
	ofPoint offsetMouthCenter = ofPoint(slope * offsetMouthY + intercept, offsetMouthY);

	// Adapted from https://math.stackexchange.com/q/1101779
	ofPoint vertexA = ofPoint(mouthCenter.x - (triangleLength * sin(addRadians)), mouthCenter.y - (triangleLength * cos(addRadians)));
	ofPoint vertexB = ofPoint(mouthCenter.x - (triangleLength * sin(subRadians)), mouthCenter.y - (triangleLength * cos(subRadians)));
	ofPoint offsetVertexA = ofPoint(offsetMouthCenter.x - (triangleLength * sin(addRadians)), offsetMouthCenter.y - (triangleLength * cos(addRadians)));
	ofPoint offsetVertexB = ofPoint(offsetMouthCenter.x - (triangleLength * sin(subRadians)), offsetMouthCenter.y - (triangleLength * cos(subRadians)));

	ofFill();
	ofSetColor(orangeRed);
	ofDrawTriangle(mouthCenter.x, mouthCenter.y, vertexA.x, vertexA.y, vertexB.x, vertexB.y);

	ofSetLineWidth(5.0f);
	ofSetColor(ofColor::white);

	ofPolyline offsetCalloutLineA = ofPolyline();
	ofPoint circleIntersectionA = getIntersection(offsetMouthCenter, offsetVertexA, circleCenter, circleRadius);
	offsetCalloutLineA.addVertex(circleIntersectionA);
	offsetCalloutLineA.addVertex(offsetVertexA);
	offsetCalloutLineA.draw();

	ofPolyline offsetCalloutLineB = ofPolyline();
	ofPoint circleIntersectionB = getIntersection(offsetMouthCenter, offsetVertexB, circleCenter, circleRadius);
	offsetCalloutLineB.addVertex(circleIntersectionB);
	offsetCalloutLineB.addVertex(offsetVertexB);
	offsetCalloutLineB.draw();
}

// Adapted from https://stackoverflow.com/a/1088058
ofPoint ofApp::getIntersection(ofPoint pointA, ofPoint pointB, ofPoint circleCenter, float circleRadius) {
	// compute the euclidean distance between A and B
	float distance = ofDist(pointA.x, pointA.y, pointB.x, pointB.y);

	// compute the direction vector from A to B
	ofVec2f directionVector = ofVec2f((pointB.x - pointA.x) / distance, (pointB.y - pointA.y) / distance);

	// compute the distance between the points A and E, where
	// E is the point of AB closest the circle center (Cx, Cy)
	float intersectionDistance = directionVector.x * (circleCenter.x - pointA.x) + directionVector.y * (circleCenter.y - pointA.y);

	// compute the coordinates of the point E
	ofPoint closestToCenterPoint = ofVec2f(intersectionDistance * directionVector.x + pointA.x, intersectionDistance * directionVector.y + pointA.y);

	// compute the euclidean distance between E and C
	float euclideanDistance = ofDist(circleCenter.x, circleCenter.y, closestToCenterPoint.x, closestToCenterPoint.y);

	// test if the line intersects the circle
	if (euclideanDistance < circleRadius)
	{
		// compute distance from t to circle intersection point
		double radiusSquared = (double)circleRadius*circleRadius;
		float dt = sqrt(radiusSquared - euclideanDistance*euclideanDistance);

		// compute intersection point (don't need the point at intersectionDistance - dt
		// because we don't need the intersection at the top of the circle)	
		float Gx = (intersectionDistance + dt)*directionVector.x + pointA.x;
		float Gy = (intersectionDistance + dt)*directionVector.y + pointA.y;
		
		return ofPoint(Gx, Gy);
	}
}

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