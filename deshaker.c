#include <cv.h>
#include <stdio.h>
#include "features.h"
#include "deshaker.h"

void deshake(IplImage *frame, IplImage *target) {
  /* This is the target transformation. FIXME: Use sane values so the wheel
   * will be aligned */
  CvPoint2D32f targets[] = {
    cvPoint2D32f(640.0, 360.0),   /* Axis */
    cvPoint2D32f(433.0, 360.0),   /* Top Center */
    cvPoint2D32f(454.0, 467.0),   /* Edge Left */
    cvPoint2D32f(447.0, 253.0),   /* Edge Right */
    cvPoint2D32f(815.0, 446.0),   /* Bottom Left */
    cvPoint2D32f(810.0, 253.0),   /* Bottom Right */
  };
  CvMat to = cvMat(6, 1, CV_32FC2, targets);

  // Matrix contains 2D vectors as channels (who designed this shit?)
  CvPoint2D32f points[6];
  for(int i=0;i<6;i++) {
    points[i] = cvPoint2D32f((double) last_location[i].x,
                             (double) last_location[i].y);
  }
  CvMat from = cvMat(6, 1, CV_32FC2, points);

  CvMat *transformation = cvCreateMat(2,3, CV_32FC1);

  cvEstimateRigidTransform(&from, &to, transformation, 0);

  cvWarpAffine(frame, target, transformation,
      CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS, CV_RGB(0,0,0));
}
