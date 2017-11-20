#include "ofApp.h"
using namespace ofxCv;
using namespace cv;

// TODO: Read default threshold etc. from Json
// TODO: write new threshold etc. to Json when receiving via WebSockets
// see below:
//
bool ofApp::loadConfiguration() {
  configFile.open("config.json", ofFile::ReadOnly);
  if(configFile.isFile() && configFile.canRead()) {
    ofBuffer buff = configFile.readToBuffer();
    parseAndReadInJSONConfig(buff.getText(), true);
    return true;
  } else {
    return false;
  }
}

bool ofApp::saveConfiguration() {

  configFile.open("config.json", ofFile::WriteOnly);

  if(configFile.isFile() && configFile.canWrite()) {

    Json::Value configRoot;
    configRoot["address"] = address;
    configRoot["port"] = port;
    configRoot["threshold"] = threshold;
    configRoot["minAreaRadius"] = minAreaRadius;
    configRoot["maxAreaRadius"] = maxAreaRadius;
    configRoot["resolution"]["width"] = camWidth;
    configRoot["resolution"]["height"] = camHeight;
    configFile << jsonWriter.write(configRoot) << endl;

    return true;

  } else {

    return false;

  }


}

bool ofApp::connectWebsocket() {

  try {
    // basic connection:
    client.connect(address, port);
    // OR optionally use SSL
    //     client.connect("echo.websocket.org", true);

    // 1 - get default options
    //    ofxLibwebsockets::ClientOptions options = ofxLibwebsockets::defaultClientOptions();

    // 2 - set basic params
    //    options.host = "echo.websocket.org";

    // advanced: set keep-alive timeouts for events like
    // loss of internet

    // 3 - set keep alive params
    // BIG GOTCHA: on BSD systems, e.g. Mac OS X, these time params are system-wide
    // ...so ka_time just says "check if alive when you want" instead of "check if
    // alive after X seconds"
    //    options.ka_time     = 1;
    //    options.ka_probes   = 1;
    //    options.ka_interval = 1;=

    // 4 - connect
    //    client.connect(options);
    //
  } catch (const std::exception& e) { // reference to the base of a polymorphic object
    std::cout << e.what(); // information from length_error printed
  }


}

//--------------------------------------------------------------
void ofApp::setup(){

  ofBackground(0);
  ofSetLogLevel(OF_LOG_NOTICE);

  lastConnectAttempt = ofGetElapsedTimeMillis();
  client.addListener(this);



  // Define some default values. If there is a config.json in the data folder
  // the values from there will be used instead. See use of loadConfiguration()
  address = "localhost";
  port = 8080;
  camWidth = 1280;  // try to grab at this size.
  camHeight = 960;
  frameSequence = 0;
  threshold = 150;
  minAreaRadius = 15;
  maxAreaRadius = 200;

  if(loadConfiguration() == false) {
    cout << "No config file yet, save default values to config.json" << endl;
    saveConfiguration();
  } else {
    cout << "Loaded previous config from config.json\n" << "threshold: " << threshold << endl;
  }

  connectWebsocket();

  // watcher.registerAllEvents(this);
  //
  // std::string folderToWatch = ofToDataPath("", true);
  // bool listExistingItemsOnStart = false;
  //
  // watcher.addPath(folderToWatch, listExistingItemsOnStart);



  //we can now get back a list of devices.
  vector<ofVideoDevice> devices = vidGrabber.listDevices();

  for(int i = 0; i < devices.size(); i++){
    if(devices[i].bAvailable){
      ofLogNotice() << devices[i].id << ": " << devices[i].deviceName;
    }else{
      ofLogNotice() << devices[i].id << ": " << devices[i].deviceName << " - unavailable ";
    }
  }

  vidGrabber.setDeviceID(0);
  vidGrabber.setDesiredFrameRate(30);
  vidGrabber.initGrabber(camWidth, camHeight);

  inverted.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);

  // videoInverted.allocate(camWidth, camHeight, OF_PIXELS_RGB);
  // videoTexture.allocate(videoInverted);
  ofSetVerticalSync(true);


  contourFinder.setMinAreaRadius(minAreaRadius);
  contourFinder.setMaxAreaRadius(maxAreaRadius);
  contourFinder.setThreshold(threshold);
  // wait for half a frame before forgetting something
  contourFinder.getTracker().setPersistence(15);
  // an object can move up to 32 pixels per frame
  contourFinder.getTracker().setMaximumDistance(32);

  showLabels = true;

  ofSetFrameRate(30);
}

//--------------------------------------------------------------
void ofApp::update(){

  // If we are not connected to WebSocket Server, try to reconnect
  // if CONNECT_ATTEMPT_INTERVAL time has passed since last try.
  // Also try to reconnect if address config has changed!
  uint64_t ellapsedMillis = ofGetElapsedTimeMillis();
  resetConnection = resetConnection || (!client.isConnected() && ellapsedMillis - lastConnectAttempt > CONNECT_ATTEMPT_INTERVAL);

  if(resetConnection) {
    lastConnectAttempt = ellapsedMillis;
    resetConnection = false;
    cout << "attempt connection" << endl;
    connectWebsocket();
  }

  if(updateResolution) {
    vidGrabber.setDeviceID(0);
    vidGrabber.setDesiredFrameRate(30);
    vidGrabber.initGrabber(camWidth, camHeight);
    inverted.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    cout << "new resolution: " << camWidth << "x" << camHeight << endl;
    updateResolution = false;
  }

  vidGrabber.update();
  if(vidGrabber.isFrameNew()) {
    frameSequence++;
    convertColor(vidGrabber, inverted, CV_RGB2GRAY);
    blur(inverted, 5);
    invert(inverted);
    inverted.update();
    contourFinder.findContours(inverted);

    // Dont send anything when no contours found.
    if (contourFinder.size() > 0) {
      Json::Value json;
      json["frameSequence"] = frameSequence;
      json["contours"] = Json::arrayValue;
      for(int i = 0; i < contourFinder.size(); i++) {
        Json::Value contour;
        ofPoint center = toOf(contourFinder.getCenter(i));
        cv::RotatedRect areaRect = contourFinder.getMinAreaRect(i);
        cv::Vec2f velocity = contourFinder.getVelocity(i);

        contour["id"] = contourFinder.getLabel(i);
        contour["pos"] = Json::arrayValue;
        contour["pos"].append(center.x);
        contour["pos"].append(center.y);
        contour["angle"] = areaRect.angle;
        contour["width"] = areaRect.size.width;
        contour["height"] = areaRect.size.height;
        contour["area"] = areaRect.size.width * areaRect.size.height;
        contour["velocity"] = Json::arrayValue;
        contour["velocity"].append(velocity[0]);
        contour["velocity"].append(velocity[1]);
        contour["points"] = Json::arrayValue;

        // Add all points of contour to json array "points"
        vector<cv::Point> points = contourFinder.getContour(i);
        for (size_t j = 0; j < points.size(); j++) {
          Json::Value point = Json::Value(Json::arrayValue);
          point.append(points[j].x);
          point.append(points[j].y);
          contour["points"].append(point);
        }

        // add contour to the list
        json["contours"].append(contour);
      }

      client.send(jsonWriter.write(json));
    }
  }
}

//--------------------------------------------------------------
void ofApp::draw(){

  if(doVisualize) {
    drawContours();
  }

  ofDrawBitmapString("Theshold: " + ofToString(threshold), 10,10);
  ofDrawBitmapString(client.isConnected() ? "Client is connected" : "Client disconnected :(", 10,50);
}

void ofApp::drawContours() {

  ofSetBackgroundAuto(showLabels);
  RectTracker& tracker = contourFinder.getTracker();

  if(showLabels) {
    ofSetColor(255);
    inverted.draw(0, 0);
    contourFinder.draw();
    for(int i = 0; i < contourFinder.size(); i++) {
      ofPoint center = toOf(contourFinder.getCenter(i));
      ofPushMatrix();
      ofTranslate(center.x, center.y);
      int label = contourFinder.getLabel(i);
      string msg = ofToString(label) + ":" + ofToString(tracker.getAge(label));
      ofDrawBitmapString(msg, 0, 0);
      ofVec2f velocity = toOf(contourFinder.getVelocity(i));
      ofScale(5, 5);
      ofDrawLine(0, 0, velocity.x, velocity.y);
      ofPopMatrix();
    }
  } else {
    for(int i = 0; i < contourFinder.size(); i++) {
      unsigned int label = contourFinder.getLabel(i);
      // only draw a line if this is not a new label
      if(tracker.existsPrevious(label)) {
        // use the label to pick a random color
        ofSeedRandom(label << 24);
        ofSetColor(ofColor::fromHsb(ofRandom(255), 255, 255));
        // get the tracked object (cv::Rect) at current and previous position
        const cv::Rect& previous = tracker.getPrevious(label);
        const cv::Rect& current = tracker.getCurrent(label);
        // get the centers of the rectangles
        ofVec2f previousPosition(previous.x + previous.width / 2, previous.y + previous.height / 2);
        ofVec2f currentPosition(current.x + current.width / 2, current.y + current.height / 2);
        ofDrawLine(previousPosition, currentPosition);
      }
    }
  }

  // this chunk of code visualizes the creation and destruction of labels
  const vector<unsigned int>& currentLabels = tracker.getCurrentLabels();
  const vector<unsigned int>& previousLabels = tracker.getPreviousLabels();
  const vector<unsigned int>& newLabels = tracker.getNewLabels();
  const vector<unsigned int>& deadLabels = tracker.getDeadLabels();
  ofSetColor(cyanPrint);
  for(int i = 0; i < currentLabels.size(); i++) {
    int j = currentLabels[i];
    ofDrawLine(j, 0, j, 4);
  }
  ofSetColor(magentaPrint);
  for(int i = 0; i < previousLabels.size(); i++) {
    int j = previousLabels[i];
    ofDrawLine(j, 4, j, 8);
  }
  ofSetColor(yellowPrint);
  for(int i = 0; i < newLabels.size(); i++) {
    int j = newLabels[i];
    ofDrawLine(j, 8, j, 12);
  }
  ofSetColor(ofColor::white);
  for(int i = 0; i < deadLabels.size(); i++) {
    int j = deadLabels[i];
    ofDrawLine(j, 12, j, 16);
  }
}

//--------------------------------------------------------------
void ofApp::onConnect( ofxLibwebsockets::Event& args ){
  cout<<"on connected"<<endl;
}

//--------------------------------------------------------------
void ofApp::onOpen( ofxLibwebsockets::Event& args ){
  cout<<"on open"<<endl;
}

//--------------------------------------------------------------
void ofApp::onClose( ofxLibwebsockets::Event& args ){
  cout<<"on close"<<endl;
}

//--------------------------------------------------------------
void ofApp::onIdle( ofxLibwebsockets::Event& args ){
  // cout<<"on idle"<<endl;
}

// void ofApp::onDirectoryWatcherItemModified(const ofxIO::DirectoryWatcherManager::DirectoryEvent& evt){
//   cout << "Modified "  + evt.item.path() << ". Reload config." << endl;
//   loadConfiguration();
// }




/**
* Parses a JSON configuration and replace existing values with onMessage
* from the JSON.
* For example, such a JSON can look like:
*
* {
*   threshold: 128,
*   minAreaRadius: 12,
*   maxAreaRadius 300,
*   resolution: {
*     width: 1920,
*     height: 1080
*   }
* }
*
* Not all parameters have to be present. Only the ones inside the JSON,
* will be changed. Other values will stay unaffected.
*
* @param std::string json
*/
bool ofApp::parseAndReadInJSONConfig( std::string json, bool isLocalConfig = false ) {
  Json::Value messageRoot;
  jsonReader.parse(json, messageRoot);
  bool configChanged = false;
  cout << messageRoot << endl;

  if(isLocalConfig && messageRoot["address"].isString()) {
    std::string newAddress = messageRoot["address"].asString();
    if(newAddress != address) {
      address = newAddress;
      resetConnection = true;
      configChanged = true;
      cout << "new address: " << address << endl;
    }
  }

  if(isLocalConfig && messageRoot["port"].isInt()) {
    int newPort = messageRoot["port"].asInt();
    if(newPort != port) {
      port = newPort;
      resetConnection = true;
      configChanged = true;
      cout << "new port: " << port << endl;
    }
  }

  if(messageRoot["threshold"].isNumeric()) {
    int newThreshold = messageRoot["threshold"].asInt();
    if(newThreshold != threshold) {
      threshold = newThreshold;
      contourFinder.setThreshold(threshold);
      configChanged = true;
      cout << "new threshold: " << threshold << endl;
    }
  }

  if(messageRoot["minAreaRadius"].isNumeric()) {
    int newMinAreaRadius = messageRoot["minAreaRadius"].asInt();
    if(newMinAreaRadius != minAreaRadius) {
      minAreaRadius = newMinAreaRadius;
      contourFinder.setMinAreaRadius(minAreaRadius);
      configChanged = true;
      cout << "new minAreaRadius: " << minAreaRadius << endl;
    }

  }

  if(messageRoot["maxAreaRadius"].isNumeric()) {
    int newMaxAreaRadius = messageRoot["maxAreaRadius"].asInt();
    if(newMaxAreaRadius != maxAreaRadius) {
      maxAreaRadius = newMaxAreaRadius;
      contourFinder.setMaxAreaRadius(maxAreaRadius);
      configChanged = true;
      cout << "new maxAreaRadius: " << maxAreaRadius << endl;
    }

  }

  if(messageRoot["resolution"]["width"].isNumeric() && messageRoot["resolution"]["height"].isNumeric()) {
    // update resolution
    int newCamWidth = messageRoot["resolution"]["width"].asInt();
    int newCamHeight = messageRoot["resolution"]["height"].asInt();
    if(newCamWidth != camWidth || newCamHeight != camHeight) {
      camWidth = newCamWidth;
      camHeight = newCamHeight;
      updateResolution = true;
      configChanged = true;
    }
  }

  return configChanged;
}

//--------------------------------------------------------------
void ofApp::onMessage( ofxLibwebsockets::Event& args ){
  cout << "Got new message: " << args.message << endl;
  bool configChanged = parseAndReadInJSONConfig(args.message);
  if(configChanged) saveConfiguration();
}

//--------------------------------------------------------------
void ofApp::onBroadcast( ofxLibwebsockets::Event& args ){
  cout<<"Got new broadcast message: " << args.message << endl;
  bool configChanged = parseAndReadInJSONConfig(args.message);
  if(configChanged) saveConfiguration();
}

//--------------------------------------------------------------
void ofApp::exit(){
  saveConfiguration();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if(key == 'v') {
    doVisualize = !doVisualize;
  }

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
