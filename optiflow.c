#include <cv.h>
#include "optiflow.h"

void findTrackingPoints(IplImage *deshaked_frame, IplImage *mask, int* max_corners,
    CvPoint2D32f* corners) {

  /* Convert to grey for corner tracking */
  IplImage* temp_grey = cvCreateImage(cvGetSize(deshaked_frame), IPL_DEPTH_8U, 1);
  cvCvtColor(deshaked_frame, temp_grey, CV_RGB2GRAY);

  cvGoodFeaturesToTrack(temp_grey, NULL, NULL, corners, max_corners, 0.05,
      20, mask, 8, 1, 0.01);

  /* Refine by subpixel corner detection */

  /* Termination criteria */
  int type = CV_TERMCRIT_ITER|CV_TERMCRIT_EPS;
  double eps = 0.01;
  int iter = 10;

  CvTermCriteria crit = cvTermCriteria(type,iter,eps);

  cvFindCornerSubPix(temp_grey, corners, *max_corners, cvSize(5,5), cvSize(-1,-1), crit);
  cvReleaseImage(&temp_grey);
}
