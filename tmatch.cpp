#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <stdio.h>
#include "tmatch.h"

using namespace std;
using namespace cv;

Mat img; Mat templ; Mat result;

int match_method = CV_TM_CCOEFF_NORMED;
float threshold_min = 0.007; float threshold_max = 0.90;

char* image_window = "Source Image";

bool doesMatch(float f) {
  if( match_method  == CV_TM_SQDIFF || match_method == CV_TM_SQDIFF_NORMED ) {
    return (f <= threshold_min);
  } else {
    return (f >= threshold_max);
  }
}

void readAndMatch( char* imgname, char* templname, void *out )
{
  /// Load image and template
  img = imread( imgname );
  templ = imread( templname, 1 );
  vector<Point> *vec = (vector<Point> *) out;

  /// Create windows
  namedWindow( image_window, CV_WINDOW_AUTOSIZE );

  Mat img_display;
  img.copyTo( img_display );

  /// Create the result matrix
  int result_cols =  img.cols - templ.cols + 1;
  int result_rows = img.rows - templ.rows + 1;

  /// Do the Matching and Normalize
  matchTemplate( img, templ, result, match_method );
  // normalize( result, result, 0, 1, NORM_MINMAX, -1, Mat() );

  float max = 0;
  for(int i = 0; i < result_rows; i++) {
    for(int j = 0; j < result_cols; j++) {
      float res = result.at<float>(i,j);
      if(res > max) max = res;
      if(doesMatch(res)) {
        cout << "(" << j << ", " << i << "): " << res << endl;
        rectangle( img_display,
          Point(j, i),
          Point(j + templ.cols , i + templ.rows ),
          Scalar::all(0), 2, 8, 0
        );

        vec->push_back(Point(j, i));
      }
    }
  }

  if(max >= threshold_max) {
    imshow( image_window, img_display );
    waitKey(0);
  }

  return;
}
