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

  return ret;
}
