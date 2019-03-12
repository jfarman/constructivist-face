#include "ofApp.h"


int main() {
	ofGLFWWindowSettings settings;
	settings.resizable = false;
	settings.setSize(640, 480);
	ofCreateWindow(settings);
	return ofRunApp(new ofApp);
}
