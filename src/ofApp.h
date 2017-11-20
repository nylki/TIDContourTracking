#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxNetwork.h"
#include "ofxJSON.h"
// #include "ofxIO.h"
#include <sstream>



class ofApp : public ofBaseApp{

public:
	void setup();
	void update();
	void draw();
	void drawContours();
	bool loadConfiguration();
	bool saveConfiguration();

	void keyPressed  (int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void exit();
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);

	bool connectTCP();
	bool parseAndReadInJSONConfig(std::string json, bool isLocalConfig);

	// ofxIO::DirectoryWatcherManager watcher;

	// void onDirectoryWatcherItemModified(const ofxIO::DirectoryWatcherManager::DirectoryEvent& evt);
  //
	// void onDirectoryWatcherItemAdded(const ofxIO::DirectoryWatcherManager::DirectoryEvent& evt){};
  // void onDirectoryWatcherItemRemoved(const ofxIO::DirectoryWatcherManager::DirectoryEvent& evt){};
  // void onDirectoryWatcherItemMovedFrom(const ofxIO::DirectoryWatcherManager::DirectoryEvent& evt){};
  // void onDirectoryWatcherItemMovedTo(const ofxIO::DirectoryWatcherManager::DirectoryEvent& evt){};
  // void onDirectoryWatcherError(const Poco::Exception& exc){};

	ofxTCPClient client;
	ofxJSONElement json;
	Json::FastWriter jsonWriter;
  Json::Reader jsonReader;


	std::string address;
	int port;


	float threshold;
	float minAreaRadius;
	float maxAreaRadius;
	ofImage inverted;
	ofVideoGrabber vidGrabber;
	ofPixels videoInverted;
	ofTexture videoTexture;
	int camWidth;
	int camHeight;
	ofxCv::ContourFinder contourFinder;
	bool showLabels;
	int frameSequence;
	ofFile configFile;

	const int CONNECT_ATTEMPT_INTERVAL = 5000;
	uint64_t lastConnectAttempt = 0;

	bool resetConnection = false;
	bool updateResolution = false;
	bool doVisualize = false;

	string msgRx;

	// // websocket methods
	// void onConnect( ofxLibwebsockets::Event& args );
	// void onOpen( ofxLibwebsockets::Event& args );
	// void onClose( ofxLibwebsockets::Event& args );
	// void onIdle( ofxLibwebsockets::Event& args );
	// void onMessage( ofxLibwebsockets::Event& args );
	// void onBroadcast( ofxLibwebsockets::Event& args );
};
