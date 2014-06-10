#include <cv.h>
#include <stdio.h>
#include "overkill.h"
#include "optiflow.h"

void findTrackingPoints(IplImage *deshaked_frame, IplImage *mask, int* max_corners,
    CvPoint2D32f* corners) {

  /* Convert to grey for corner tracking */
  IplImage* temp_grey = cvCreateImage(cvGetSize(deshaked_frame), IPL_DEPTH_8U, 1);
  cvCvtColor(deshaked_frame, temp_grey, CV_RGB2GRAY);

  cvGoodFeaturesToTrack(temp_grey, NULL, NULL, corners, max_corners, 0.15,
      20, mask, 20, 1, 0.04);

  /* Termination criteria for subpixel */
  int type = CV_TERMCRIT_ITER|CV_TERMCRIT_EPS;
  double eps = 0.01;
  int iter = 10;

  CvTermCriteria crit = cvTermCriteria(type,iter,eps);

  /* Refine by subpixel corner detection */
  cvFindCornerSubPix(temp_grey, corners, *max_corners, cvSize(3,3), cvSize(-1,-1), crit);
  cvReleaseImage(&temp_grey);
}

void opticalFlow(IplImage* prev, IplImage* curr, IplImage *mask, CvPoint2D32f*
    prev_pts, CvPoint2D32f *curr_pts, int* pts_cnt) {

  static char corner_status[MAX_CORNERS];

  if(!prev) {
    return;
  }

  /* Termination criteria: FIXME static */
  int type = CV_TERMCRIT_ITER|CV_TERMCRIT_EPS;
  double eps = 0.01;
  int iter = 10;

  CvTermCriteria crit = cvTermCriteria(type,iter,eps);
  cvCalcOpticalFlowPyrLK(prev, curr,
      NULL, NULL, prev_pts, curr_pts, *pts_cnt,
      cvSize(10,10), 3, corner_status, NULL, crit, 0);

  /* Remove points that left the tracking mask */
  for(int i=0;i<*pts_cnt;i++) {
    /* Not found by flow tracking -> skip */
    if(corner_status[i] == 0) {
      continue;
    }

    /* Point left the tracking mask */
    CvPoint pnt = cvPointFrom32f(curr_pts[i]);
    int col = mask->imageData[1280*pnt.y+pnt.x];
    if(col == 0) {
      corner_status[i] = 0;
      //printf("Feature %d left bounds\n", i);
    }
  }

  /* Remove all points marked for deletion */
  filterPts(curr_pts, corner_status, *pts_cnt);
  *pts_cnt = filterPts(prev_pts, corner_status, *pts_cnt);
}

int filterPts(CvPoint2D32f *pts, char* status, int size) {
  CvPoint2D32f new[MAX_CORNERS];

  int pos=0;
  for(int i=0;i<size;i++) {
    if(status[i] != 0) {
      new[pos++] = pts[i];
    }
  }
  memcpy(pts, new, pos*sizeof(pts[0]));
  return pos;
}
