#include <iostream>
#include <ctime>
#include <csignal>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <errno.h>

#include "tmatch.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#define Q 143
#define BARH 62
#define WINW 1920
#define WINH 1080

using namespace std;
using namespace cv;

/*
  ## REQUIREMENTS ##

  DLAT and DLNG values are fixed for a 1920 x 1080 screen
  running a 100%-zoom Google Chrome window, with a 16z map zoom
  in Ubuntu 16.04.

  TODO:
  * check system requirements:
  *** opencv
  *** python3 (and its dependencies)

  * display all reads in single image
  * check when flag in args is not recognized
  (http://www.cplusplus.com/articles/DEN36Up4/)

  FIXME:
  * false match for accidente.png
  * improve lat: error increases as match is farther from centre?
  * read cropped images
  * emb_moderado is also read as emb_grave and emb_total
*/

const int ZOOM      = 17;
const int PRECISION = 7;
const int ICON      = 54;
const int DELAY     = 3;      // TODO: allow to choose between various internet speeds
const double DLAT   = 0.0108;  // y-axis, increase upwards
const double DLNG   = 0.0206;  // x-axis, decrease left
const double DLOAD  = 5;      // console progress bar
const char* IMGNAME = "screenshot.png";
const char* LOGNAME = "data-wr.log";

struct Location {
  double lat;
  double lng;
};

int gidx = 0;
bool graphicMode, debugMode, allMode;
ofstream bash, data;
struct Location grid[Q];

string currentDateTime();
void chksyscall(char* line);
void clean();
void fillCol(int init, double iniLat, double iniLng, int n);
void getCoordinates(int pixelLng, int pixelLat, double lat, double lng, void *res);
void initGrid();
void printProgress(int i, int load);
void signalHandler( int signum );
void writeMatches(char* templname, char* label, double lat, double lng);

int main (int argc, char *argv[]) {
  // get execution flags
  if(argc > 1) {
    graphicMode = false;    
    debugMode = false;
    allMode = false;

    for(int i = 1; i < argc; i++) {
      graphicMode = graphicMode || strcmp(argv[i], (char *)"--graphic") == 0;
      debugMode = debugMode || strcmp(argv[i], (char *)"--debug") == 0;
      allMode = allMode || strcmp(argv[i], (char *)"-A") == 0 || strcmp(argv[i], (char *)"--all") == 0;
    }
  }

  chksyscall("./setup.sh");

  signal(SIGINT, signalHandler);

  bash.precision(PRECISION);
  data.precision(PRECISION);
  cout.precision(PRECISION);

  cout << currentDateTime() << endl;
  data.open(LOGNAME, ios::app);
  data << "*** " << currentDateTime() << " ***" << endl;
  data.close();

  initGrid();

  int load = 0;   // used in console progress bar
  for(int i = 0; i < Q; i++) {
    // print progress bar
    printProgress(i, load);
    load+=DLOAD;

    double lat = grid[i].lat;
    double lng = grid[i].lng;

    bash.open ("script.sh");
    bash << "#!/bin/bash\n";
    bash << "google-chrome --headless --disable-gpu --screenshot --window-size="
        << WINW << "," << WINH
        << " \'https://www.waze.com/es-419/livemap?zoom=" << ZOOM
        << "&lat=" << lat << "&lon=" << lng << "\' &> /dev/null\n";
    bash.close();

    chksyscall( (char*)"chmod +x script.sh" );
    chksyscall( (char*)"./script.sh" );

    writeMatches((char *)"icons/accidente.png", (char *)"accidente", lat, lng);
  }

  cout << currentDateTime() << endl;
  cout << "map reading done. fetching coordinates from file..." << endl;

  chksyscall("python3 locate.py &");
  cout << "fetch done. cleaning and exiting..." << endl;

  chksyscall("rm coordinate.log");
  clean();

  return 0;
}

void printProgress(int i, int load) {
  if(((i+1)*100/Q) >= load) {
    cout << load << "% " << (load/10 == 0 ? " " : "") << "[";
    for(int j = 0; j < load; j+=DLOAD) cout << "==";
    for(int j = load; j < 100; j+=DLOAD)  cout << "  ";
    cout << "]" << endl;
  }
}

void writeMatches(char *templname, char* label, double lat, double lng) {
  struct Location loc;
  vector<Point> points;

  fetchMatches( (char*) IMGNAME, templname, &points, graphicMode, debugMode );

  if(points.size() >= 1) {
    data.open(LOGNAME, ios::app);
    for(vector<Point>::const_iterator pos = points.begin(); pos != points.end(); ++pos) {
      getCoordinates(pos->x+ICON/2, pos->y+ICON/2, lat, lng, &loc);
      cout << label << ": " << loc.lng << "," << loc.lat << endl;
      data << currentDateTime() << "," << label << "," << loc.lat << "," << loc.lng << endl;
    }
    

    data.close();
  }
}

void getCoordinates(int pixelLng, int pixelLat, double lat, double lng, void *res) {
  // image's origin coordinates
  double oLat = lat + DLAT/2 ;
  double oLng = lng - DLNG/2;

  double resLat = oLat - DLAT*((double) pixelLat/(WINH-BARH));
  double resLng = oLng + DLNG*((double) pixelLng/WINW);

  struct Location * locRes = (struct Location *) res;
  locRes->lat = resLat;
  locRes->lng = resLng;
}

void chksyscall(char* line) {
  int status = system(line);
  if (status < 0) {
    cout << "Error: " << strerror(errno) << '\n';
    clean();
    exit(-1);
  }
  else
  {
    if (!WIFEXITED(status)) {
      clean();
      cout << "Call to " << line << "was interrupted. Exiting...\n";
      exit(-1);
    }
  }
}

void fillCol(double iniLat, double iniLng, int n) {
  grid[gidx].lat = iniLat;
  grid[gidx].lng = iniLng;

  for(int j = 0; j < n; j++) {
    grid[++gidx].lat = grid[gidx-1].lat - DLAT;
    grid[gidx].lng = iniLng;
  }
}

// TODO change lngs accordingly with new DLNG
void initGrid() {
  gidx = 0;
  fillCol(4.8196, -74.02588, 11);
  fillCol(4.7980, -74.04648, 13);
  fillCol(4.7635, -74.06708, 18);
  fillCol(4.7569, -74.08768, 23);
  fillCol(4.7569, -74.10828, 25);
  fillCol(4.7421, -74.12888, 19);
  fillCol(4.6989, -74.14948, 15);
  fillCol(4.6532, -74.17008, 8);
  fillCol(4.6424, -74.19068, 6);
  fillCol(4.6316, -74.21128, 5);
}

string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

void signalHandler( int signum ) {
   cout << "Interrupt signal (" << signum << ") received.\n";
   clean();
   exit(signum);

}

void clean() {
  cout << "closing files..." << endl;
  data.open(LOGNAME, ios::app);
  data << endl;
  data.close();
  bash.close();

  cout << "deleting auxiliary files..." << endl;
  chksyscall("rm screenshot.png");
  chksyscall("rm script.sh");

  cout << "done." << endl;
}
