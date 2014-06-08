#define FEATURE_AXIS         0
#define FEATURE_TOP_CENTER   1
#define FEATURE_EDGE_LEFT    2
#define FEATURE_EDGE_RIGHT   3
#define FEATURE_BOTTOM_LEFT  4
#define FEATURE_BOTTOM_RIGHT 5

#define FEATURE_COUNT 6

IplImage* feature_templates[FEATURE_COUNT];
CvMat* feature_results[FEATURE_COUNT];
CvPoint last_location[FEATURE_COUNT];
bool stable[FEATURE_COUNT];
FILE* feature_cache_fh;

void initFeatures(const char* feature_cache_file);
CvPoint matchFeature(IplImage*, int);
void trackFeatures(IplImage*, int);
