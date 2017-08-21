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

#define Q 77
#define WINW 1920
#define WINH 1080

using namespace std;
using namespace cv;

/*
 * DELTALAT and DELTALNG values are fixed for a 1920 x 1080 screen
 * running a 100%-zoom Google Chrome window, with a 16z map zoom
 * in Ubuntu 16.04.
*/
const int ZOOM = 17;
const int PRECISION = 9;
const double DELTALAT = 0.020891;  // y-axis, increase upwards
const double DELTALNG = 0.040962;  // x-axis, decrease left
const char* IMGNAME = "screenshot.png";

struct Location {
  double lat;
  double lng;
};

ofstream bash, data;
struct Location grid[Q];

string currentDateTime();
void chksyscall(char* line);
void clean();
void fillCol(int init, double iniLat, double iniLng, int n);
void getCoordinates(int pixelLng, int pixelLat, double lat, double lng, void *res);
void initGrid();
void printPoint(ofstream *ofs, float x, float y);
void signalHandler( int signum );

int main () {
  signal(SIGINT, signalHandler);

  bash.precision(PRECISION);
  data.precision(PRECISION);
  cout.precision(PRECISION);

  data.open("data.log", ios::app);
  data << "*** " << currentDateTime() << " ***" << endl << endl;
  data.close();

  initGrid();

  for(int i = 1; i <= Q; i++) {
    double lat = grid[i].lat, lng = grid[i].lng;
    struct Location loc;
    vector<Point> points;

    bash.open ("script.sh");
    bash << "#!/bin/bash\n";
    bash << "google-chrome --headless --disable-gpu --screenshot --window-size="
        << WINW << "," << WINH
        << " \'https://www.waze.com/es-419/livemap?zoom=" << ZOOM
        << "&lat=" << lat << "&lon=" << lng << "\' &> /dev/null\n";
    bash.close();

    chksyscall( (char*)"chmod +x script.sh" );
    chksyscall( (char*)"./script.sh" );

    readAndMatch( (char*) IMGNAME, (char*) "icons/accidente.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/obra.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/via_cerrada.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/embotellamiento_grave.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/detenido.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/embotellamiento_alto_total.png", &points );

    if(points.size() >= 1) {
      data.open("data.log", ios::app);
      data << "(" << lat << "," << lng << ")" << endl;

      for(vector<Point>::const_iterator pos = points.begin(); pos != points.end(); ++pos) {
        getCoordinates(pos->x, pos->y, lat, lng, &loc);
        cout << " |-- " << loc.lat << "," << loc.lng << endl;
        data << " |-- " << loc.lat << "," << loc.lng << endl;
        if((pos+1) == points.end())  data << endl;
      }

      data.close();
      cout << endl;
    }
  }

  return 0;
}

void getCoordinates(int pixelLng, int pixelLat, double lat, double lng, void *res) {
  // image's origin coordinates
  double oLat = lat + DELTALAT/2;
  double oLng = lng - DELTALNG/2;

  double resLat = oLat - DELTALAT*((double) pixelLat/WINH);
  double resLng = oLng + DELTALNG*((double) pixelLng/WINW);

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
    if (WIFEXITED(status)) {
      // cout << "Program returned normally, exit code " << WEXITSTATUS(status) << '\n';
    } else {
      clean();
      cout << "Program exited abnormally\n";
      exit(-1);
    }
  }
}

void fillCol(int init, double iniLat, double iniLng, int n) {
  grid[init].lat = iniLat;
  grid[init].lng = iniLng;

  ++init;
  for(int j = init; j < init+n-1; j++) {
    grid[j].lat = grid[j-1].lat - DELTALAT;
    grid[j].lng = iniLng;
  }
}

void initGrid() {
  fillCol(0, 4.8196776, -74.0258801, 9);
  fillCol(9, 4.7859368, -74.0587103, 13);
  fillCol(22, 4.7521960, -74.0915405, 17);
  fillCol(39, 4.7353256, -74.1243707, 16);
  fillCol(55, 4.7353256, -74.1572009, 13);
  fillCol(68, 4.6341032, -74.1900311, 5);
  fillCol(73, 4.6341032, -74.2228613, 4);
}

string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}

void signalHandler( int signum ) {
   cout << "Interrupt signal (" << signum << ") received.\n";
   clean();
   exit(signum);

}

void clean() {
  data.open("data.log", ios::app);
  data << endl;
  data.close();

  bash.close();
}

void printPoint(ofstream *ofs, float x, float y) {
  *ofs << x << y;
}
