#include <cv.h>
#include <highgui.h>
#include <stdio.h> /* snprintf, f* */
#include "features.h"
#include "inline.h"
#include "overkill.h"
#include "deshaker.h"

int frame_sync[FEATURE_COUNT];

/* Loads feature templates and opens cache file */
void initFeatures(const char* cache_file) {
  IplImage** f = feature_templates;

  /* FIXME: Dynamic from config file. Also change FEATURE_COUNT */
  #define PATH "./data/"

  f[FEATURE_AXIS]         = cvLoadImage(PATH "axis.png",         1);
  f[FEATURE_TOP_CENTER]   = cvLoadImage(PATH "top-center.png",   1);
  f[FEATURE_EDGE_LEFT]    = cvLoadImage(PATH "edge-left.png",    1);
  f[FEATURE_EDGE_RIGHT]   = cvLoadImage(PATH "edge-right.png",   1);
  f[FEATURE_BOTTOM_LEFT]  = cvLoadImage(PATH "bottom-left.png",  1);
  f[FEATURE_BOTTOM_RIGHT] = cvLoadImage(PATH "bottom-right.png", 1);
  f[FEATURE_BOTTOM_RIGHT] = cvLoadImage(PATH "bottom-right.png", 1);

  f[FEATURE_ZERO] = cvLoadImage(PATH "zero.png", 1);
  f[FEATURE_90]   = cvLoadImage(PATH "90.png",   1);
  f[FEATURE_270]  = cvLoadImage(PATH "270.png",   1);

  /* Initialize result matrices and frame syncs */
  for(int i=0;i<FEATURE_COUNT;i++) {
    feature_results[i] = NULL;
    frame_sync[i] = -1;
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

    if(i < STATIC_FEATURE_COUNT) {
      /* Calculate distance to last location. If distance too big -> skip marker */
      double distance = okDistance(&last_location[i], &location);

      /* Only static features can by instable */
      if(distance > 10.0) {
        stable[i] = 0;
      } else {
        stable[i] = 1;
        last_location[i] = location;
      }
    } else {
      stable[i] = 1;
      last_location[i] = location;
    }

    /* Draw marker */
    /* FIXME: Modifying the current frame is not allowed by OpenCV */
#ifdef RENDER_DEBUG
    CvScalar color = (current_frame % 2 == 0) ? CV_RGB(255,0,0):  CV_RGB(0,255,0);
    if(stable[i] == 0) {
      color = CV_RGB(255,255,0);
    }

    cvCircle(frame, location, 3, color, 1, CV_AA, 0);
#endif
  }
}

/* returns sync zone if synced, -1 otherwise */
int resyncByStatic(int current_frame, IplImage* target) {
  /* Bounding boxes for features */
  static double bounding[] = {
    820, 790, 420, 300, /* x,x,y,y for FEATURE_ZERO */
    800, 500, 540, 485, /* FEATURE_90 */
    750, 500, 225, 170, /* FEATURE_270 */
  };

  int detected = -1;

  for(int i=STATIC_FEATURE_COUNT;i<FEATURE_COUNT;i++) {
    if(frame_sync[i] == -1) {
      frame_sync[i] = current_frame;
    }

    /* Transform to current */
    CvPoint v;
    v = last_location[i];
    CvMat*       m = ok_transformation;
    double tx = cvmGet(m,0,0)*v.x + cvmGet(m,0,1)*v.y + cvmGet(m,0,2);
    double ty = cvmGet(m,1,0)*v.x + cvmGet(m,1,1)*v.y + cvmGet(m,1,2);

    double box_x1 = bounding[(i-STATIC_FEATURE_COUNT)*4];
    double box_x2 = bounding[(i-STATIC_FEATURE_COUNT)*4+1];
    double box_y1 = bounding[(i-STATIC_FEATURE_COUNT)*4+2];
    double box_y2 = bounding[(i-STATIC_FEATURE_COUNT)*4+3];

    assert(box_x1 > box_x2);
    assert(box_y1 > box_y2);

#ifdef RENDER_DEBUG
    if(target) {
      cvRectangle(target, cvPoint(box_x1, box_y1), cvPoint(box_x2, box_y2), CV_RGB(255, 255, 0), 1, 8, 0);
    }
#endif

    if(tx < box_x1 && tx > box_x2 && ty < box_y1 && ty > box_y2) {
      //printf("Marker in zone %d: (%f,%f)\n",i-STATIC_FEATURE_COUNT,tx,ty);
      /* FIXME: Hardcoded center */
      tx -= 640;
      ty -= 358;
      double norm = sqrt(pow(tx,2) + pow(ty,2));

      /* Check if within sane bounds. otherwise: false positive */
      if(norm > 150.0 && norm < 190.0) {
        rotation_angle = atan2(ty/norm,tx/norm);
        frame_sync[i] = current_frame;

        detected = i;
      } else {
        printf("norm: %f out of bounds\n", norm);
      }
    }
  }

  return detected;
}
