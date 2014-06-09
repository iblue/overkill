#include <cv.h>
#include <highgui.h>
#include <stdio.h> /* snprintf, f* */
#include "features.h"
#include "inline.h"

/* Loads feature templates and opens cache file */
void initFeatures(const char* cache_file) {
  IplImage** f = feature_templates;

  /* FIXME: Dynamic from config file. Also change FEATURE_COUNT */
  #define PATH "/home/iblue/motiontrack/"

  f[FEATURE_AXIS]         = cvLoadImage(PATH "axis.png",         1);
  f[FEATURE_TOP_CENTER]   = cvLoadImage(PATH "top-center.png",   1);
  f[FEATURE_EDGE_LEFT]    = cvLoadImage(PATH "edge-left.png",    1);
  f[FEATURE_EDGE_RIGHT]   = cvLoadImage(PATH "edge-right.png",   1);
  f[FEATURE_BOTTOM_LEFT]  = cvLoadImage(PATH "bottom-left.png",  1);
  f[FEATURE_BOTTOM_RIGHT] = cvLoadImage(PATH "bottom-right.png", 1);

  /* Initialize result matrices */
  for(int i=0;i<FEATURE_COUNT;i++) {
    feature_results[i] = NULL;
  }

  /* Cache file for holding feature positions, because feature dection is slow
   * as fuck */
  feature_cache_fh = fopen(cache_file, "r+");
  if(feature_cache_fh == NULL) {
    feature_cache_fh = fopen(cache_file, "w");
  }
  assert(feature_cache_fh != NULL);
}

/* Estimates the position of the given feature using overlapping correlation */
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
    feature_results[feature_type] = result;
  }

  cvMatchTemplate(frame, template, result, 1);

  double minVal; double maxVal; CvPoint minLoc; CvPoint maxLoc;
  CvPoint matchLoc;

  cvMinMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc, NULL);
  matchLoc = minLoc;

  /* Marker is at top left, move to center */
  matchLoc.x += template->width/2;
  matchLoc.y += template->height/2;

  /* Draw value of correllation */
  /*
  char value[10];
  snprintf((char *)&value, sizeof(value)-1, "%1.5f",minVal);
  CvFont font;
  cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 1, 1, 0, 1, 8);
  cvPutText(frame, (const char*)value, matchLoc, &font, CV_RGB(0,0,0));
  */

  return matchLoc;
}

/* Updates the feature location and stability params for the given frame */
void trackFeatures(IplImage *frame, int current_frame) {
  int frame_read = 0;
  if(fread(&frame_read, sizeof(int), 1, feature_cache_fh) == 1) {
    /* we have data for this frame */
  } else {
    fwrite(&current_frame, sizeof(int), 1, feature_cache_fh);
  }

  for(int i=0;i<FEATURE_COUNT;i++) {
    /* Try to load position from feature cache or calculte if not available */
    CvPoint location;
    if(fread(&location, sizeof(CvPoint), 1, feature_cache_fh) == 1) {
    } else {
      location = matchFeature(frame, i);
      fwrite(&location, sizeof(CvPoint), 1, feature_cache_fh);
    }

    /* Initialize if first frame */
    if(current_frame == 0) {
      last_location[i] = location;
      stable[i]        = 1;
    }

    /* Calculate distance to last location. If distance too big -> skip marker */
    double distance = okDistance(&last_location[i], &location);

    if(distance > 10.0) {
      stable[i] = 0;
    } else {
      stable[i] = 1;
      last_location[i] = location;
    }

    /* Draw marker */
    /* FIXME: Modifying the current frame is not allowed by OpenCV */
    CvScalar color = (current_frame % 2 == 0) ? CV_RGB(255,0,0):  CV_RGB(0,255,0);
    if(stable[i] == 0) {
      color = CV_RGB(255,255,0);
    }

    cvCircle(frame, location, 3, color, 1, CV_AA, 0);
  }
}
