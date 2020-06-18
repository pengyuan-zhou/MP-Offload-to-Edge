#ifndef DETECTOR_H
#define DETECTOR_H

struct object{
    char *name;
    float prob;
    int left;
    int right;
    int top;
    int bot;
};

struct result{
    int num;
    struct object *objects;
};

void test_detector(char *datacfg, char *cfgfile, char *weightfile, char *filename, float thresh, float hier_thresh, char *outfile, int fullscreen);
void load_params();
//last change
//struct result* detect(int frmID);
struct result* detect();

#endif
