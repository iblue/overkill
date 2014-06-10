#include <cv.h>
#include "overkill.h"

#define NELEMS(x)  (sizeof(x) / sizeof(x[0]))

IplImage* mask(IplImage *source, IplImage **visual) {
  /* Positive tracking zones */
  CvPoint tracking_add[] = {
    cvPoint(420, 600), cvPoint(780, 480), /* Left Outer */
    cvPoint(420, 240), cvPoint(780, 100), /* Right Outer */
    cvPoint(490, 450), cvPoint(750, 380), /* Left Inner */
    cvPoint(490, 260), cvPoint(750, 320), /* Right Inner */
    cvPoint(440, 520), cvPoint(370, 390), /* Top Left */
    cvPoint(440, 210), cvPoint(370, 340), /* Top Right */
  };

  /* Negative tracking zones */
  CvPoint tracking_sub[] = {
    cvPoint(660, 255), cvPoint(720, 200), /* Dirt on Wall: Do not track */
  };

  /* Draw circles to check if deshaking worked */
  IplImage* vis = *visual = cvCloneImage(source);

#ifdef RENDER_INFO
  CvPoint center = cvPoint(633,355);
  CvScalar color = CV_RGB(0,0,255);

  cvCircle(vis, center, 203, color, 1, CV_AA, 0);
  cvCircle(vis, center, 150, color, 1, CV_AA, 0);
  cvCircle(vis, center, 250, color, 1, CV_AA, 0);
#endif

  /* Colors and init for tracking mask */
  CvScalar notrack = CV_RGB(0,0,0);
  CvScalar track   = CV_RGB(255,255,255);
#ifdef RENDER_DEBUG
  CvScalar notrack_color = CV_RGB(255,0,0);
  CvScalar track_color = CV_RGB(0,255,0);
#endif
  IplImage* temp_mask = cvCreateImage(cvGetSize(source), IPL_DEPTH_8U, 1);

  /* Initialize mask first. Everything non-zero will be tracked */
  for(int i=0;i<temp_mask->imageSize;i++) {
    temp_mask->imageData[i] = 0;
  }

  for(int i=0;i<NELEMS(tracking_add);i+=2) {
#ifdef RENDER_DEBUG
    /* Outline on displayed frame */
    cvRectangle(vis,       tracking_add[i], tracking_add[i+1], track_color,  1, 8, 0);
#endif

    /* Filled rectangle on tracking mask */
    cvRectangle(temp_mask, tracking_add[i], tracking_add[i+1], track, -1, 8, 0);
  }

  for(int i=0;i<NELEMS(tracking_sub);i+=2) {
#ifdef RENDER_DEBUG
    /* Outline on displayed frame */
    cvRectangle(vis,       tracking_sub[i], tracking_sub[i+1], notrack_color,  1, 8, 0);
#endif

    /* Filled rectangle on tracking mask */
    cvRectangle(temp_mask, tracking_sub[i], tracking_sub[i+1], notrack, -1, 8, 0);
  }

  return temp_mask;
}
