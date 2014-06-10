#include <cv.h>
#include <stdio.h>
#include "features.h"
#include "deshaker.h"

void deshake(IplImage *frame, IplImage *target) {
  /* This is the target transformation. FIXME: Use sane values so the wheel
   * will be aligned */
  /* TODO: Optimization: Static */
  CvPoint2D32f targets[] = {
    cvPoint2D32f(640.0, 360.0),   /* Axis */
    cvPoint2D32f(433.0, 360.0),   /* Top Center */
    cvPoint2D32f(454.0, 467.0),   /* Edge Left */
    cvPoint2D32f(447.0, 253.0),   /* Edge Right */
    cvPoint2D32f(815.0, 446.0),   /* Bottom Left */
    cvPoint2D32f(810.0, 253.0),   /* Bottom Right */
  };

  /* We create the point->point matricies and skip every coordinate that is
   * considered instable by the feature tracking */
  int coordinate_count=0;
  CvPoint2D32f to_pts[STATIC_FEATURE_COUNT];
  for(int i=0;i<STATIC_FEATURE_COUNT;i++) {
    if(stable[i]) {
      to_pts[coordinate_count++] = targets[i];
    }
  }
  CvMat to = cvMat(coordinate_count, 1, CV_32FC2, to_pts);

  /* Same for the source matrix */
  coordinate_count=0;
  CvPoint2D32f from_pts[STATIC_FEATURE_COUNT];
  for(int i=0;i<STATIC_FEATURE_COUNT;i++) {
    if(stable[i]) {
      from_pts[coordinate_count++] = cvPoint2D32f((double) last_location[i].x,
                                                  (double) last_location[i].y);
    }
  }
  CvMat from = cvMat(coordinate_count, 1, CV_32FC2, from_pts);

  // FIXME: Memory Leak
  ok_transformation = cvCreateMat(2, 3, CV_32FC1);

  cvEstimateRigidTransform(&from, &to, ok_transformation, 0);

  cvWarpAffine(frame, target, ok_transformation,
      CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS, CV_RGB(0,0,0));
}
