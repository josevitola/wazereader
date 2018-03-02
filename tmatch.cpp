#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include "tmatch.h"

using namespace std;
using namespace cv;

Mat img; Mat templ; Mat result;

int match_method = CV_TM_CCOEFF_NORMED;
float threshold_min = 0.007; float threshold_max = 0.88;

char* image_window = "Source Image";

bool doesMatch(float f) {
  if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED ) {
    return (f <= threshold_min);
  } else {
    return (f >= threshold_max);
  }
}

void fetchMatches( char* imgname, char* templname, void *out, bool graphic, bool debug )
{
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

  return;
}
