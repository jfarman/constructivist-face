#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

ofColor orangeRed;
ofColor darkGray;
ofColor gradientArray[255];

const float cameraWidth = 640;
const float cameraHeight = 480;

void ofApp::setup() {
	ofSetVerticalSync(true);
	cam.setup(cameraWidth, cameraHeight);
	
	tracker.setup();
	tracker.setRescale(.5);

	classifier.load("expressions");

	orangeRed = ofColor(255, 70, 47);
	darkGray = ofColor(49, 48, 44);
	calculateGradient();

	font.load("kremlin.ttf", 120, true, true, true);

	displayString = "HELLO";
	updateString(displayString);

	leadingTextTimer = 0;
	trailingTextTimer = 0;

	shouldDrawLeadingText = true;
	shouldDrawTrailingText = false;
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

	ofPoint noseStartPoint = noseBridge[0];
	ofPoint noseEndPoint = noseBridge[1];
	mouthCenter = tracker.getImageFeature(ofxFaceTracker::INNER_MOUTH).getCentroid2D();

	lineSlope = (noseEndPoint.x - noseStartPoint.x) / (noseEndPoint.y - noseStartPoint.y);
	lineIntercept = mouthCenter.x - (lineSlope*mouthCenter.y);

	float maxYDimension = cameraHeight + 50; // Add some padding just to make sure text runs fully off screen

	ofPoint endPoint = ofPoint(lineSlope * maxYDimension + lineIntercept, maxYDimension);
	startPoint = mouthCenter;

	bisectingLine.clear();
	bisectingLine.addVertex(startPoint.x, startPoint.y);
	bisectingLine.addVertex(endPoint.x, endPoint.y);

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

	// todo: figure out more elegant way of extending line lengths
	ofVec2f directionVectorA = ofVec2f(vertexA.x - startPoint.x, vertexA.y - startPoint.y);
	ofPoint extendoA = vertexA + directionVectorA * maxYDimension;
	lineA.addVertex(extendoA);

	ofVec2f directionVectorB = ofVec2f(vertexB.x - startPoint.x, vertexB.y - startPoint.y);
	ofPoint extendoB = vertexB + directionVectorB * maxYDimension;
	lineB.addVertex(extendoB);
}


void ofApp::draw() {
	ofSetColor(255);
	ofPushStyle();

	ofPushStyle();	
	ofNoFill();
	
	ofEnableSmoothing();

	ofTexture tex = cam.getTexture();
	ofPixels pix;
	tex.readToPixels(pix);

	ofPoint leftVertexA = ofPoint(0, 0);
	ofPoint leftVertexB = ofPoint(circleCenter.x - circleRadius, circleCenter.y);
	ofPoint leftVertexC = ofPoint(0, 480);
	
	ofPoint rightVertexA = ofPoint(640, 0);
	ofPoint rightVertexB = ofPoint(circleCenter.x + circleRadius, circleCenter.y);
	ofPoint rightVertexC = ofPoint(640, 480);

	int pixelWidth = cameraWidth;
	int pixelHeight = cameraHeight;

	for (int i = 0; i < pixelWidth; i++) {
		for (int j = 0; j < pixelHeight; j++) {
			ofColor color = pix.getColor(i, j);
			// https://en.wikipedia.org/wiki/Grayscale#Luma_coding_big_gameas
			float luminance = (0.3 * color.r) + (0.59 * color.g) + (0.11 * color.b);
			// Default color for pixels is monochromotatic orange-red gradient
			ofColor pixelColor = orangeGradientArray[(int)luminance];
			if (tracker.getFound()) {
				// Pixels within the head-enscapsulating circle and the frames are grayscale
				bool isPixelEncircled = ofDist(i, j, circleCenter.x, circleCenter.y) < circleRadius;
				bool isInFrameHeight = (j > circleCenter.y and j < cameraWidth);
				if (isPixelEncircled) pixelColor = ofColor(luminance);
				else if (isWithinTriangle(ofPoint(i,j), leftVertexA, leftVertexB, leftVertexC)) pixelColor = ofColor(luminance);
				else if (isWithinTriangle(ofPoint(i, j), rightVertexA, rightVertexB, rightVertexC)) pixelColor = ofColor(luminance);
			}
			pix.setColor(i, j, pixelColor);
		}
	}

	tex.loadData(pix);
	tex.draw(0, 0);

	string mouthState = "mouth not detected";

	if (tracker.getFound()) { // Only draw callouts and frames if face is detected
		ofPushStyle();

		ofNoFill();
		ofSetLineWidth(5.0f);
		ofSetColor(ofColor::white);

		// Draw triangular frames on either side of the face circle
		ofDrawTriangle(leftVertexA.x, leftVertexA.y, leftVertexB.x, leftVertexB.y, leftVertexC.x, leftVertexC.y);
		ofDrawTriangle(rightVertexA.x, rightVertexA.y, rightVertexB.x, rightVertexB.y, rightVertexC.x, rightVertexC.y);

		ofSetCircleResolution(100);
		ofDrawCircle(circleCenter, circleRadius);

		ofPopStyle();

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
}

void ofApp::keyPressed(int key) {}

void ofApp::drawCallout(ofPoint circleCenter, float circleRadius) {
	ofFill();
	ofSetColor(orangeRed);
	// todo: figure out if there is a less hacky way than indexing into polylines
	ofDrawTriangle(mouthCenter.x, mouthCenter.y, lineA[1].x, lineA[1].y, lineB[1].x, lineB[1].y);

	ofSetLineWidth(5.0f);
	ofSetColor(ofColor::white);

	offsetLineA.draw();
	offsetLineB.draw();

	// Draw leading text, followed by trailing text
	if (shouldDrawLeadingText) drawText(true);
	if (shouldDrawTrailingText) drawText(false);
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
		orangeGradientArray[i] = ofColor(rA, gA, bA);
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

void ofApp::updateString(string newString) {
	charsAsFBOs.clear();
	for (char & c : newString)
	{
		string charString = string(1, c);
		ofRectangle boundingBox = font.getStringBoundingBox(charString, 0, 0, true);
		if (boundingBox.width > 0 && boundingBox.height > 0) {

			ofPushStyle();
			ofPushMatrix();
			ofFbo fbo1;
			fbo1.allocate(boundingBox.width, boundingBox.height);
			fbo1.begin();
			ofFill();
			ofSetColor(ofColor::white);
			font.drawString(charString, 0, boundingBox.height);
			fbo1.end();
			ofTexture tex1 = fbo1.getTextureReference();
			charsAsFBOs.push_back(fbo1);
			ofPopMatrix();
			ofPopStyle();
		}
	}
}

// Adapted from https://sites.google.com/site/ofauckland/examples/quad-warping
void ofApp::ofxQuadWarp(ofTexture tex, ofPoint a, ofPoint b, ofPoint c, ofPoint d, int rows, int cols)
{
	// The quad warping approach below requires top left, bottom left, etc points, but the parameters are
	// 4 arbitrary points. The sorting strategy below is not ideal but this is just for a proof of concept.
	
	vector<ofPoint> pointsByX{ a, b, c, d };
	vector<ofPoint> pointsByY{ a, b, c, d };
	ofPoint lt, rt, rb, lb;

	ofPoint leftwardPoint = pointsByX[0];
	for (int i = 0; i < 4; i++) {
		if (leftwardPoint == pointsByY[i]) {
			lt = (i < 2) ? pointsByY[i] : pointsByX[1];
			lb = (i < 2) ? pointsByX[1] : pointsByY[i];
		}
	}
	ofPoint rightwardPoint = pointsByX[2];
	for (int i = 0; i < 4; i++) {
		if (rightwardPoint == pointsByY[i]) {
			rb = (i < 2) ? pointsByY[i] : pointsByX[3];
			rt = (i < 2) ? pointsByX[3] : pointsByY[i];
		}
	}

	float tw = tex.getWidth();
	float th = tex.getHeight();

	ofMesh mesh;

	for (int x = 0; x <= cols; x++) {
		float f = float(x) / cols;
		ofPoint vTop(ofxLerp(lt, rt, f));
		ofPoint vBottom(ofxLerp(lb, rb, f));
		ofPoint tTop(ofxLerp(ofPoint(0, 0), ofPoint(tw, 0), f));
		ofPoint tBottom(ofxLerp(ofPoint(0, th), ofPoint(tw, th), f));

		for (int y = 0; y <= rows; y++) {
			float f = float(y) / rows;
			ofPoint v = ofxLerp(vTop, vBottom, f);
			mesh.addVertex(v);
			mesh.addTexCoord((ofVec2f)ofxLerp(tTop, tBottom, f));
		}
	}

	for (float y = 0; y < rows; y++) {
		for (float x = 0; x < cols; x++) {
			mesh.addTriangle(ofxIndex(x, y, cols + 1), ofxIndex(x + 1, y, cols + 1), ofxIndex(x, y + 1, cols + 1));
			mesh.addTriangle(ofxIndex(x + 1, y, cols + 1), ofxIndex(x + 1, y + 1, cols + 1), ofxIndex(x, y + 1, cols + 1));
		}
	}

	tex.bind();
	mesh.draw();
	tex.unbind();
}

void ofApp::drawText(bool isLeadingText) {

	if (isLeadingText) leadingTextTimer += ofGetLastFrameTime();
	else trailingTextTimer += ofGetLastFrameTime();
	float timer = (isLeadingText) ? leadingTextTimer : trailingTextTimer;
	float animationTime = timer / 5;

	float distance = currentTriangleHeight;
	float unit = distance / (6 * charsAsFBOs.size()); // arbitrary
	float easing = 1 + sin(PI / 2 * animationTime - PI / 2);
	float arbitraryStartLerp = 0.0f + (animationTime);
	ofPoint currentStartPosition = (ofPoint)bisectingLine.getPointAtIndexInterpolated(arbitraryStartLerp);
	float lerpUnit = 0.2;

	int numChars = MIN((int)timer + 1, charsAsFBOs.size());
	bool allCharsVisible = numChars == charsAsFBOs.size();

	for (int i = 0; i < numChars; i++)
	{
		int charIndex = charsAsFBOs.size() - (i + 1);
		float lerpIndexStart = arbitraryStartLerp - (i * lerpUnit);

		ofPoint interpolatedLineAPointStart = (ofPoint)lineA.getPointAtIndexInterpolated(lerpIndexStart);
		ofPoint interpolatedLineBPointStart = (ofPoint)lineB.getPointAtIndexInterpolated(lerpIndexStart);

		if (lerpIndexStart > 1) {
			float overhang = lerpIndexStart - 1;
			float length = (overhang)* distance;
			ofPoint vertexA = (ofPoint)lineA[1];
			float extendoDistanceA = vertexA.distance((ofPoint)lineA[2]);
			float newLerp = length / extendoDistanceA;

			interpolatedLineAPointStart = (ofPoint)lineA.getPointAtIndexInterpolated(1 + newLerp);
			interpolatedLineBPointStart = (ofPoint)lineB.getPointAtIndexInterpolated(1 + newLerp);

			lerpIndexStart = 1 + newLerp;

			if (allCharsVisible) {
				if (charIndex == 0) {
					if (isLeadingText) {
						leadingTextTimer = 0;
						shouldDrawLeadingText = false;
					}
					else {
						trailingTextTimer = 0;
						shouldDrawTrailingText = false;
					}
				}
				if (charIndex == numChars - 1) {
					if (isLeadingText) {
						shouldDrawTrailingText = true;
						//ofLog(OF_LOG_NOTICE, "shouldStartTrailingText = true");
					}
					else shouldDrawLeadingText = true;
				}
			}
		}

		float lerpIndexEnd = lerpIndexStart + (lerpUnit * 5 / 6); // arbitrary: using 5/6th of available space to space characters

		ofPoint interpolatedLineAPointEnd = (ofPoint)lineA.getPointAtIndexInterpolated(lerpIndexEnd);
		ofPoint interpolatedLineBPointEnd = (ofPoint)lineB.getPointAtIndexInterpolated(lerpIndexEnd);

		if (lerpIndexEnd > 1) {
			float overhang = lerpIndexEnd - 1;
			float length = (overhang)* distance;
			ofPoint vertexA = (ofPoint)lineA[1];
			float extendoDistanceA = vertexA.distance((ofPoint)lineA[2]);
			float newLerp = length / extendoDistanceA;

			interpolatedLineAPointEnd = (ofPoint)lineA.getPointAtIndexInterpolated(1 + newLerp);
			interpolatedLineBPointEnd = (ofPoint)lineB.getPointAtIndexInterpolated(1 + newLerp);

			lerpIndexEnd = 1 + newLerp;
		}

		ofSetColor(darkGray);

		ofTexture tex1 = charsAsFBOs[charIndex].getTextureReference();
		tex1.getTextureData().bFlipTexture = true;
		ofPoint lt = ofPoint(interpolatedLineAPointStart);
		ofPoint rt = ofPoint(interpolatedLineBPointStart);
		ofPoint rb = ofPoint(interpolatedLineAPointEnd);
		ofPoint lb = ofPoint(interpolatedLineBPointEnd);
		ofxQuadWarp(tex1, lt, rt, rb, lb, 40, 40);
	}

	ofPopStyle();
}

ofPoint ofApp::ofxLerp(ofPoint start, ofPoint end, float amt) {
	return start + amt * (end - start);
}

int ofApp::ofxIndex(float x, float y, float w) {
	return y * w + x;
}

bool ofApp::sortByX(const ofPoint &a, const ofPoint &b) {
	return a.x < b.x;
}

bool ofApp::sortByY(const ofPoint &a, const ofPoint &b) {
	return a.y < b.y;
}