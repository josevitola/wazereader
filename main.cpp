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

#define Q 163
#define WINW 1920
#define WINH 1080

using namespace std;
using namespace cv;

/*
  DLAT and DLNG values are fixed for a 1920 x 1080 screen
  running a 100%-zoom Google Chrome window, with a 16z map zoom
  in Ubuntu 16.04.

  TODO
  * display all reads in single image
  FIXME
  * false match for accidente.png
  * improve lat: error increases as match is further from centre?
  * read cropped images
*/

const int ZOOM = 17;
const int PRECISION = 7;
const int ICONSIZE = 54;
const double DLAT = 0.0108;  // y-axis, increase upwards
const double DLNG = 0.0206;  // x-axis, decrease left
const double DLOAD = 5;      // console progress bar
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
void signalHandler( int signum );

int main () {
  cout << currentDateTime() << endl;
  signal(SIGINT, signalHandler);

  bash.precision(PRECISION);
  data.precision(PRECISION);
  cout.precision(PRECISION);

  data.open("data.log", ios::app);
  data << "*** " << currentDateTime() << " ***" << endl << endl;
  data.close();

  initGrid();

  int load = 0;   // used in console progress bar
  for(int i = 0; i < Q; i++) {
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

    // readAndMatch( (char*) IMGNAME, (char*) "icons/accidente.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/detenido.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/embotellamiento_grave.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/embotellamiento_moderado.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/embotellamiento_alto_total.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/obra.png", &points );
    // readAndMatch( (char*) IMGNAME, (char*) "icons/via_cerrada.png", &points );

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

    // print progress bar
    if(((i+1)*100/Q) >= load) {
      cout << lng << "," << lat << endl;
      cout << load << "% " << (load/10 == 0 ? " " : "") << "[";
      for(int j = 0; j < load; j+=DLOAD) cout << "==";
      for(int j = load; j < 100; j+=DLOAD)  cout << "  ";
      cout << "]" << endl;
      load+=DLOAD;
    }
  }

  cout << currentDateTime() << endl;
  return 0;
}

void getCoordinates(int pixelLng, int pixelLat, double lat, double lng, void *res) {
  // image's origin coordinates
  double oLat = lat + DLAT/2;
  double oLng = lng - DLNG/2;

  double resLat = oLat - DLAT*((double) (pixelLat+ICONSIZE/2)/WINH);
  double resLng = oLng + DLNG*((double) (pixelLng+ICONSIZE/2)/WINW);

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
    grid[j].lat = grid[j-1].lat - DLAT;
    grid[j].lng = iniLng;
  }
}

// TODO change lngs accordingly with new DLNG
void initGrid() {
  fillCol(0,  4.81968, -74.02588, 19);
  fillCol(9,  4.78594, -74.04648, 28);
  fillCol(22, 4.75220, -74.06708, 36);
  fillCol(39, 4.73533, -74.08768, 34);
  fillCol(55, 4.73533, -74.10828, 28);
  fillCol(68, 4.63410, -74.12888, 11);
  fillCol(73, 4.63410, -74.14948, 9);
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
