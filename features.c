#include <highgui.h>
#include "features.h"

void initFeatureDetection(void) {
  IplImage** f = feature_templates;

  /* FIXME: Dynamic from config file. Also change FEATURE_COUNT */
  f[0] = cvLoadImage("/home/iblue/motiontrack/axis.png", 1);
  f[1] = cvLoadImage("/home/iblue/motiontrack/top-center.png", 1);
  f[2] = cvLoadImage("/home/iblue/motiontrack/edge-left.png", 1);
  f[3] = cvLoadImage("/home/iblue/motiontrack/edge-right.png", 1);
}

CvPoint matchFeature(IplImage *frame, int feature_type) {
  assert(feature_type >= 0);
  assert(feature_type < FEATURE_COUNT);

  IplImage *template = feature_templates[feature_type];
  /* Initialize result space */
  /* TODO: Optimization: Only once per template. Maybe on template load? */
  //static CvMat *result = NULL;
  //if(result == NULL) {
  CvMat *result = NULL;
  int result_cols = frame->width - template->width + 1;
  int result_rows = frame->height - template->height + 1;
  result = cvCreateMat(result_rows, result_cols, CV_32FC1);
  //}

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

  return;
}
