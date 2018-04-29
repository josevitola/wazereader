#include <iostream>
#include <ctime>
#include <csignal>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <errno.h>

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#define Q 143
#define BARH 62
#define WINW 1920
#define WINH 1080
#define NMODS 5

using namespace std;
using namespace cv;

const int ZOOM      = 17;
const int PRECISION = 7;
const int ICON      = 54;
const int DELAY     = 3;      // TODO: allow to choose between various internet speeds
const double DLAT   = 0.0108;  // y-axis, increase upwards
const double DLNG   = 0.0206;  // x-axis, decrease left
const double DLOAD  = 5;      // console progress bar
const char* IMGNAME = "screenshot.png";
char *DIR = "icons/";
char* image_window = "Source Image";

struct Location {
  double lat;
  double lng;
};

Mat img; Mat templ; Mat result;
int match_method = CV_TM_CCOEFF_NORMED;
float threshold_min = 0.007; float threshold_max = 0.88;
int gidx = 0;
bool graphicMode, debugMode;
ofstream bash, data;
struct Location grid[Q];

bool READFILES[NMODS];
char *FILENAMES[NMODS] = {
  "accidente.png", "detenido.png", "embotellamiento.png",
  "obra.png", "via_cerrada.png"
};
char *FILELABELS[NMODS] = {
  "accidente", "detenido", "embotellamiento",
  "obra", "via-cerrada"
};
char *FLAGNAMES[NMODS] = {
  "--acc", "--det", "--emb", "--obra", "--via",
};

string currentDateTime();
bool doesMatch(float f);
void chksyscall(char* line);
void clean();
void fetchMatches(char* imgname, char* templname, void *out, bool graphic, bool debug);
void fillCol(int init, double iniLat, double iniLng, int n);
void getCoordinates(int pixelLng, int pixelLat, double lat, double lng, void *res);
void initGrid();
void printProgressBar(int i, int load);
void signalHandler( int signum );
void writeMatch(char* templname, char* label, double lat, double lng);
void writeMatches(double lat, double lng);

int main (int argc, char *argv[]) {
  // initialize all flags in false
  bool all = false;  
  graphicMode = false;    
  debugMode = false;
  READFILES[0] = true;
  for(int i=1; i<NMODS; i++) {
    READFILES[i] = false; 
  }

  if(argc > 1) {
    for(int i = 1; i < argc; i++) {
      char arg[10];
      strcpy(arg, argv[i]);
      
      graphicMode = graphicMode || strcmp(arg, (char *)"--graphic") == 0;
      debugMode = debugMode || strcmp(arg, (char *)"--debug") == 0;

      if(all) continue;
      if(strcmp(arg, (char *)"-A") == 0 || strcmp(arg, (char *)"--all") == 0) {
        all = true;
        cout << "fetching all icons...\n";
        for(int j=1; j<NMODS; j++) {
          READFILES[j] = true;
        }
        continue;
      }

      bool found = false;
      for(int j=0; j<NMODS; j++)  {// TODO: can't add same flag more than once
        if(strcmp(arg, (char *)FLAGNAMES[j]) == 0) {
          READFILES[j] = true; 
          cout << "fetching " << FILELABELS[j] << '\n';
          found = true;
          break;
        }}
      
      if(!found) {
        cout << arg << " is not a valid flag.\nexiting...\n";
        return 0;
      }
    }
  }

  // chksyscall("./setup.sh");

  signal(SIGINT, signalHandler);

  bash.precision(PRECISION);
  data.precision(PRECISION);
  cout.precision(PRECISION);

  cout << currentDateTime() << endl;

  initGrid();

  int load = 0;   // used in console progress bar
  for(int i = 0; i < Q; i++) {
    printProgressBar(i, load);
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

    writeMatches(lat, lng);
  }

  cout << currentDateTime() << endl;
  cout << "map reading done. fetching coordinates from file..." << endl;

  chksyscall("python3 locate.py &");
  cout << "fetch done. cleaning and exiting..." << endl;

  chksyscall("rm coordinate.log");
  clean();

  return 0;
}

void printProgressBar(int i, int load) {
  if(((i+1)*100/Q) >= load) {
    cout << load << "% " << (load/10 == 0 ? " " : "") << "[";
    for(int j = 0; j < load; j+=DLOAD) cout << "==";
    for(int j = load; j < 100; j+=DLOAD)  cout << "  ";
    cout << "]" << endl;
  }
}

void writeMatch(char *templname, char* label, double lat, double lng) {
  struct Location loc;
  vector<Point> points;

  fetchMatches( (char*) IMGNAME, templname, &points, graphicMode, debugMode );

  if(points.size() >= 1) {
    char filename[50];
    strcpy(filename, label);
    strcat(filename, "-wr.log");
    cout << filename << '\n';
    data.open(filename, ios::app);
    for(vector<Point>::const_iterator pos = points.begin(); pos != points.end(); ++pos) {
      getCoordinates(pos->x+ICON/2, pos->y+ICON/2, lat, lng, &loc);
      //cout << label << ": " << loc.lng << "," << loc.lat << endl;
      data << currentDateTime() << "," << loc.lat << "," << loc.lng << endl;
    }
    
    data.close();
  }
}

void writeMatches(double lat, double lng) {
  for(int i=0; i<NMODS; i++)
    if(READFILES[i]) {
      char filename[50];
      strcpy(filename, DIR);
      strcat(filename, FILENAMES[i]);
      writeMatch(filename, FILELABELS[i], lat, lng);
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
      cerr << "call to " << line << " was interrupted. exiting...\n";
      exit(EXIT_FAILURE);
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
  cout << "closing file streams..." << endl;
  bash.close();

  cout << "deleting auxiliary files..." << endl;
  chksyscall("if [ -f ./screenshot.png ]; then \nrm screenshot.png \nfi");
  chksyscall("if [ -f ./script.sh ]; then \nrm script.sh \nfi");

  cout << "done." << endl;
}

bool doesMatch(float f) {
  if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED ) {
    return (f <= threshold_min);
  } else {
    return (f >= threshold_max);
  }
}

void fetchMatches( char* imgname, char* templname, void *out, bool graphic, bool debug ) {
  /// Load image and template

  Mat img_display;

  vector<Point> *vec = (vector<Point> *) out;
  img = imread( imgname );
  templ = imread( templname, 1 );

  if( img.empty() ) {
    cout << "Could not open or find the image " << imgname << endl;
    vec->push_back(Point(-1,-1));
    return ;
  }

  /// Create windows
  if(graphic || debug) {
    namedWindow( image_window, CV_WINDOW_AUTOSIZE );
    img.copyTo( img_display );
  }

  /// Create the result matrix
  int result_cols =  img.cols - templ.cols + 1;
  int result_rows = img.rows - templ.rows + 1;

  matchTemplate( img, templ, result, match_method );

  float max = 0;
  for(int i = 0; i < result_rows; i++) {
    for(int j = 0; j < result_cols; j++) {
      float res = result.at<float>(i,j);
      if(res > max) max = res;
      if(doesMatch(res)) {
        if(graphic || debug) {
          cout << "(" << j << ", " << i << "): " << res << endl;
          rectangle( img_display,
            Point(j, i),
            Point(j + templ.cols , i + templ.rows ),
            Scalar::all(0), 2, 8, 0
          );
        }

        vec->push_back(Point(j, i));
      }
    }
  }

  cout << max << endl;

  if((graphic && max >= threshold_max) || debug) {
    imshow( image_window, img_display );
    waitKey(0);
    destroyAllWindows();
  }
}