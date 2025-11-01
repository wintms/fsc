#ifndef FSC_UTILS_H
#define FSC_UTILS_H

#include <stdio.h>

#define     FSC_DEBUG
#ifdef      FSC_DEBUG
#define     FSCPRINT(fmt,...) printf("[%15s-%30s:%04d] "fmt,__FILE__,__func__,__LINE__, ##__VA_ARGS__)
#else
#define     FSCPRINT(fmt,...)
#endif

#define     FSC_CONF_B2F_FILE    "/conf/fsc/fsc_z9964f_b2f.json"
#define     FSC_CONF_F2B_FILE    "/conf/fsc/fsc_z9964f_f2b.json"
#define     FSC_CONF_LIQUID_FILE "/conf/fsc/fsc_z9964fl.json"

#endif // FSC_UTILS_H