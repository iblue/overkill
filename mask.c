#include <cv.h>

IplImage* mask(IplImage *source) {
  IplImage *ret = cvCloneImage(source);
  CvPoint center = cvPoint(633,355);
  CvScalar color = CV_RGB(0,0,255);

  cvCircle(ret, center, 203, color, 1, CV_AA, 0);
  cvCircle(ret, center, 150, color, 1, CV_AA, 0);
  cvCircle(ret, center, 250, color, 1, CV_AA, 0);
  //cvLine(ret, cvPoint(420, 480), cvPoint(420, 240), CV_RGB(255,0,255), 1, CV_AA, 0);

  /* Outer tracking zone */
  color = CV_RGB(255,0,255);
  cvRectangle(ret, cvPoint(420, 610), cvPoint(800, 470), color, 1, CV_AA, 0);
  cvRectangle(ret, cvPoint(420, 240), cvPoint(800, 100), color, 1, CV_AA, 0);

  /* Inner tracking zone */
  cvRectangle(ret, cvPoint(490, 260), cvPoint(750, 320), color, 1, CV_AA, 0);
  cvRectangle(ret, cvPoint(490, 450), cvPoint(750, 380), color, 1, CV_AA, 0);

  /* FIXME: Does not belong here! */
  color = CV_RGB(255,255,255);
  IplImage* temp_mask = cvCreateImage(cvGetSize(source), IPL_DEPTH_8U, 1);

  /* Outer tracking zone */
  cvRectangle(temp_mask, cvPoint(420, 610), cvPoint(800, 470), color, -1, CV_AA, 0);
  cvRectangle(temp_mask, cvPoint(420, 240), cvPoint(800, 100), color, -1, CV_AA, 0);

  /* Inner tracking zone */
  cvRectangle(temp_mask, cvPoint(490, 260), cvPoint(750, 320), color, -1, CV_AA, 0);
  cvRectangle(temp_mask, cvPoint(490, 450), cvPoint(750, 380), color, -1, CV_AA, 0);

  /* Convert to grey for corner tracking */
  IplImage* temp_grey = cvCreateImage(cvGetSize(source), IPL_DEPTH_8U, 1);
  cvCvtColor(source, temp_grey, CV_RGB2GRAY);

  #define MAX_CORNERS 20
  CvPoint2D32f corners[MAX_CORNERS];
  int corner_count=MAX_CORNERS;
  cvGoodFeaturesToTrack(temp_grey, NULL, NULL, corners, &corner_count, 0.20, 10, temp_mask, 3, 1, 0.04);
  cvReleaseImage(&temp_grey);
  cvReleaseImage(&temp_mask);

  /* Highlight corners */
  for(int i=0;i<corner_count;i++) {
    cvCircle(ret, cvPointFrom32f(corners[i]), 2, CV_RGB(255,255,255), 1, CV_AA, 0);
  }

  return ret;
}
