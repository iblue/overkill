#include <cv.h>
#include <highgui.h>
#include "features.h"

void initFeatures(void) {
  IplImage** f = feature_templates;

  /* FIXME: Dynamic from config file. Also change FEATURE_COUNT */
  #define PATH "/home/iblue/motiontrack/"

  f[FEATURE_AXIS]        = cvLoadImage(PATH "axis.png",       1);
  f[FEATURE_TOP_CENTER]  = cvLoadImage(PATH "top-center.png", 1);
  f[FEATURE_EDGE_LEFT]   = cvLoadImage(PATH "edge-left.png",  1);
  f[FEATURE_EDGE_RIGHT]  = cvLoadImage(PATH "edge-right.png", 1);

  /* Initialize result matrices */
  for(int i=0;i<FEATURE_COUNT;i++) {
    feature_results[i] = NULL;
  }
}

CvPoint matchFeature(IplImage *frame, int feature_type) {
  assert(feature_type >= 0);
  assert(feature_type < FEATURE_COUNT);

  IplImage *template = feature_templates[feature_type];

  /* Lazy initialization for results, because we don't know frame size on init */
  CvMat *result = feature_results[feature_type];
  if(result == NULL) {
    int result_cols = frame->width - template->width + 1;
    int result_rows = frame->height - template->height + 1;
    result = cvCreateMat(result_rows, result_cols, CV_32FC1);
  }

  cvMatchTemplate(frame, template, result, 1);

  double minVal; double maxVal; CvPoint minLoc; CvPoint maxLoc;
  CvPoint matchLoc;

  cvMinMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, NULL);
  matchLoc = minLoc;

  /* Marker is at top left, move to center */
  matchLoc.x += template->width/2;
  matchLoc.y += template->height/2;

  /* Draw circle, FIXME: Remove */
  cvCircle(frame, matchLoc, 2, CV_RGB(0,255,0), 1, CV_AA, 0);

  return matchLoc;
}
