#define FEATURE_AXIS       0
#define FEATURE_TOP_CENTER 1
#define FEATURE_EDGE_LEFT  2
#define FEATURE_EDGE_RIGHT 3

#define FEATURE_COUNT      4

IplImage* feature_templates[FEATURE_COUNT];
CvMat* feature_results[FEATURE_COUNT];

void initFeatures(void);
CvPoint matchFeature(IplImage*, int);
