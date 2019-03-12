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

//--------------------------------------------------------------
void ofApp::update() {
	cam.update();
	if (cam.isFrameNew()) {
		if (tracker.update(toCv(cam))) {
			classifier.classify(tracker);
		}
	}

	ofPolyline faceOutline = tracker.getImageFeature(ofxFaceTracker::FACE_OUTLINE);
	auto boundingBox = faceOutline.getBoundingBox();
	boundingBox.standardize();

	// Approximating the bounding box of the entire face as a being 1/3 taller than the bounding box provided 
	// by the FACE_OUTLINE image feature that enscribes the top of the eyebrows and the bottom of the chin
	float extra = boundingBox.height * 0.5f;
	float expandedHeight = boundingBox.height + extra;
	float expandedY = boundingBox.y - (extra);
	ofRectangle expandedBound = ofRectangle(boundingBox.x, expandedY, boundingBox.width, expandedHeight);

	// We want the circle to be slightly larger than the max dimension (which should be height) 
	// of the bounding box that contains the rough approximation of the face + hairline
	float maxDimension = MAX(expandedHeight, boundingBox.width);
	float circleDiameter = maxDimension * 1.5f;
	circleRadius = circleDiameter / 2.0f;
	circleCenter = expandedBound.getCenter();

	// Use nose bridge feature to approximate face tilt
	ofPolyline noseBridge = tracker.getImageFeature(ofxFaceTracker::NOSE_BRIDGE);
	// If there are no points associated with the nose bridge feature, just return
	if (noseBridge.size() == 0) return;

	ofPoint originPoint = noseBridge[0];
	mouthCenter = tracker.getImageFeature(ofxFaceTracker::INNER_MOUTH).getCentroid2D();

	lineSlope = (mouthCenter.x - originPoint.x) / (mouthCenter.y - originPoint.y);
	lineIntercept = mouthCenter.x - (lineSlope*mouthCenter.y);

	fakeMaxPoint = 380;

	ofPoint endPoint = ofPoint(lineSlope * 380 + lineIntercept, 380); // todo: get width + make this accurate
	startPoint = mouthCenter;

	float halfVertex = 45.0 / 2.0f;
	currentTriangleHeight = ofDist(startPoint.x, startPoint.y, endPoint.x, endPoint.y);
	float triangleLength = currentTriangleHeight / cos(ofDegToRad(halfVertex));

	ofVec2f directionVector = ofVec2f(endPoint.x - startPoint.x, endPoint.y - startPoint.y);
	ofVec2f currentTriangleVector = directionVector.getNormalized();
	int angle = currentTriangleVector.angle(ofVec2f(0, -1));

	double addRadians = ofDegToRad(angle + halfVertex);
	double subRadians = ofDegToRad(angle - halfVertex);

	// Calculate values for offset callout
	float offsetMouthY = mouthCenter.y - 5; // todo: figure out correct offset value
	offsetMouthCenter = ofPoint(lineSlope * offsetMouthY + lineIntercept, offsetMouthY);

	// Adapted from https://math.stackexchange.com/q/1101779
	ofPoint vertexA = ofPoint(startPoint.x - (triangleLength * sin(addRadians)), startPoint.y - (triangleLength * cos(addRadians)));
	ofPoint vertexB = ofPoint(startPoint.x - (triangleLength * sin(subRadians)), startPoint.y - (triangleLength * cos(subRadians)));
	ofPoint offsetVertexA = ofPoint(offsetMouthCenter.x - (triangleLength * sin(addRadians)), offsetMouthCenter.y - (triangleLength * cos(addRadians)));
	ofPoint offsetVertexB = ofPoint(offsetMouthCenter.x - (triangleLength * sin(subRadians)), offsetMouthCenter.y - (triangleLength * cos(subRadians)));

	lineA.clear(); // todo: draw triangle using these lines instead of ofDrawTriangle()
	lineA.addVertex(startPoint.x, startPoint.y);
	lineA.addVertex(vertexA);

	lineB.clear();
	lineB.addVertex(startPoint.x, startPoint.y);
	lineB.addVertex(vertexB);

	offsetLineA.clear();
	ofPoint circleIntersectionA = getIntersection(offsetMouthCenter, offsetVertexA, circleCenter, circleRadius);
	offsetLineA.addVertex(circleIntersectionA);
	offsetLineA.addVertex(offsetVertexA);

	offsetLineB.clear();
	ofPoint circleIntersectionB = getIntersection(offsetMouthCenter, offsetVertexB, circleCenter, circleRadius);
	offsetLineB.addVertex(circleIntersectionB);
	offsetLineB.addVertex(offsetVertexB);

	// todo: add line extensions back in

	//ofVec2f directionVectorA = ofVec2f(vertexA.x - startPoint.x, vertexA.y - startPoint.y);
	////ofVec2f normalizedVectorA = directionVector.getNormalized();
	//ofPoint extendoA = vertexA + directionVectorA * maxDimension;
	//lineA.addVertex(extendoA);

	//ofVec2f directionVectorB = ofVec2f(vertexB.x - startPoint.x, vertexB.y - startPoint.y);
	////ofVec2f normalizedVectorA = directionVector.getNormalized();
	//ofPoint extendoB = vertexB + directionVectorB * maxDimension;
	//lineB.addVertex(extendoB);
}


void ofApp::draw() {
	ofSetColor(255);
	ofPushStyle();

	ofPushStyle();	
	ofNoFill();
	
	ofEnableSmoothing();
	ofSetLineWidth(5.0f);
	ofSetColor(ofColor::white);

	ofTexture tex = cam.getTexture();
	ofPixels pix;
	tex.readToPixels(pix);

	// todo: get height and width
	float height = 480 - circleCenter.y;
	float width = 640 - (circleCenter.x + circleRadius);
	ofRectangle leftRectangle = ofRectangle(0, circleCenter.y, circleCenter.x - circleRadius, height);
	ofRectangle rightRectangle = ofRectangle(circleCenter.x + circleRadius, circleCenter.y, width, height);

	ofPoint leftVertexA = ofPoint(0, 0);
	ofPoint leftVertexB = ofPoint(leftRectangle.getTopRight().x, leftRectangle.getTopRight().y);
	ofPoint leftVertexC = ofPoint(0, 480);
	ofDrawTriangle(leftVertexA.x, leftVertexA.y, leftVertexB.x, leftVertexB.y, leftVertexC.x, leftVertexC.y);
	
	ofPoint rightVertexA = ofPoint(640, 0);
	ofPoint rightVertexB = ofPoint(rightRectangle.getTopLeft().x, rightRectangle.getTopLeft().y);
	ofPoint rightVertexC = ofPoint(640, 480);
	ofDrawTriangle(rightVertexA.x, rightVertexA.y, rightVertexB.x, rightVertexB.y, rightVertexC.x, rightVertexC.y);

	// todo: figure out if these values could/should ever be a different value than 
	// the initial values passed as parameters into cam.setup(w, h);
	int pixelWidth = pix.getWidth();
	int pixelHeight = pix.getHeight();

	for (int i = 0; i < pixelWidth; i++) {
		for (int j = 0; j < pixelHeight; j++) {
			ofColor color = pix.getColor(i, j);
			// https://en.wikipedia.org/wiki/Grayscale#Luma_coding_big_gameas
			float luminance = (0.3 * color.r) + (0.59 * color.g) + (0.11 * color.b);
			// Default color for pixels is monochromotatic orange-red gradient
			ofColor pixelColor = gradientArray[(int)luminance];
			if (tracker.getFound()) {
				// Pixels within the head-enscapsulating circle and the frames are grayscale
				bool isPixelEncircled = ofDist(i, j, circleCenter.x, circleCenter.y) < circleRadius;
				bool isInFrameHeight = (j > circleCenter.y and j < 480);
				if (isPixelEncircled) pixelColor = ofColor(luminance);
				//else if (isInFrameHeight) { // shortcut for checking y of both left and right frames
				//	if (i > leftRectangle.getLeft() and i < leftRectangle.getRight())  pixelColor = ofColor(luminance);
				//	else if (i > rightRectangle.getLeft() and i < rightRectangle.getRight()) pixelColor = ofColor(luminance);
				//}
				else if (isWithinTriangle(ofPoint(i,j), leftVertexA, leftVertexB, leftVertexC)) pixelColor = ofColor(luminance);
				else if (isWithinTriangle(ofPoint(i, j), rightVertexA, rightVertexB, rightVertexC)) pixelColor = ofColor(luminance);
			}
			pix.setColor(i, j, pixelColor);
		}
	}

	tex.loadData(pix);
	tex.draw(0,0);

	string mouthState = "mouth not detected";

	if (tracker.getFound()) { // Only draw callouts and frames if face is detected
		drawSideFrames(leftRectangle, rightRectangle);

		ofSetCircleResolution(100);
		ofDrawCircle(circleCenter, circleRadius);

		// todo: get mouth state from ofxFaceTracker jaw openness gesture
		int primary = classifier.getPrimaryExpression();
		bool mouthOpen = classifier.getDescription(primary) == "open mouth";
		string suffix = (mouthOpen) ? "open" : "closed";
		mouthState = "mouth state = " + suffix;

		if (true) 
			drawCallout(circleCenter, circleRadius);
	}
	
	ofPopStyle();
	
	ofPushMatrix();
	ofTranslate(5, 10);

	int w = 250, h = 12;

	// Display information about pose (mouth open, direction facing)
	ofSetColor(ofColor::black);
	ofDrawRectangle(0, 0, w, h);
	ofSetColor(ofColor::white);
	ofDrawBitmapString(mouthState, 5, 9);
	ofTranslate(0, h + 5);

	ofSetColor(ofColor::black);
	ofDrawRectangle(0, 0, w, h);
	ofSetColor(ofColor::white);
	ofDrawBitmapString(getDirectionString(), 5, 9);
	ofTranslate(0, h + 5);

	ofPopMatrix();
	ofPopStyle();

	//todo: remove (debugging only)
	ofSetColor(ofColor::magenta);
	ofPolyline fakeMaxPointLine;
	fakeMaxPointLine.addVertex(0, fakeMaxPoint);
	fakeMaxPointLine.addVertex(pixelWidth, fakeMaxPoint);
	fakeMaxPointLine.draw();
}

void ofApp::keyPressed(int key) {}

void ofApp::drawSideFrames(ofRectangle leftFrame, ofRectangle rightFrame) {
	ofPushStyle();

	ofNoFill();
	ofSetLineWidth(5.0f);
	ofSetColor(ofColor::white);

	//ofRectangle leftRectangle = ofRectangle(0, circleCenter.y, circleCenter.x - circleRadius, height);
	//ofRectangle rightRectangle = ofRectangle(circleCenter.x + circleRadius, circleCenter.y, width, height);
	ofDrawTriangle(0, 0, leftFrame.getTopRight().x, leftFrame.getTopRight().y, 0, 480);
	ofDrawTriangle(640, 0, rightFrame.getTopLeft().x, leftFrame.getTopLeft().y, 640, 480);

	ofPopStyle();
}

void ofApp::drawCallout(ofPoint circleCenter, float circleRadius) {
	ofFill();
	ofSetColor(orangeRed);
	// todo: figure out if there is a less hacky way than indexing into polylines
	ofDrawTriangle(mouthCenter.x, mouthCenter.y, lineA[1].x, lineA[1].y, lineB[1].x, lineB[1].y);

	ofSetLineWidth(5.0f);
	ofSetColor(ofColor::white);

	offsetLineA.draw();
	offsetLineB.draw();

	ofSetColor(ofColor::magenta);
	ofDrawCircle(offsetLineA[0], 5);
	ofDrawCircle(offsetLineB[0], 5);
	ofDrawCircle(offsetMouthCenter, 5);
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

// Adapted from https://stackoverflow.com/questions/2049582/how-to-determine-if-a-point-is-in-a-2d-triangle
bool ofApp::isWithinTriangle(ofPoint s, ofPoint a, ofPoint b, ofPoint c)
{
	int as_x = s.x - a.x;
	int as_y = s.y - a.y;

	bool s_ab = (b.x - a.x)*as_y - (b.y - a.y)*as_x > 0;

	if ((c.x - a.x)*as_y - (c.y - a.y)*as_x > 0 == s_ab) return false;

	if ((c.x - b.x)*(s.y - b.y) - (c.y - b.y)*(s.x - b.x) > 0 != s_ab) return false;

	return true;
}