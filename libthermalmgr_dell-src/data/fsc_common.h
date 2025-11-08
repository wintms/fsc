/*************************************************************************
 *
 * fsc_common.h
 * Common utilities for error handling and resource management
 *
 ************************************************************************/
#ifndef FSC_COMMON_H
#define FSC_COMMON_H

#include "Types.h"
#include "cJSON.h"

// Error code definitions
#define FSC_OK              0
#define FSC_ERR_NULL        -1
#define FSC_ERR_PARSE       -2
#define FSC_ERR_RANGE       -3
#define FSC_ERR_IO          -4
#define FSC_ERR_NOT_FOUND   -5

// Resource management context for JSON parsing
typedef struct
{
    char *file_buffer;
    cJSON *json_root;
    const char *filename;
} FscParseContext;

// Parse error handling macros
#define FSC_CHECK_NULL(ptr, msg) \
    do { \
        if ((ptr) == NULL) { \
            printf("fsc: NULL pointer error - %s\n", (msg)); \
            return FSC_ERR_NULL; \
        } \
    } while(0)

#define FSC_CHECK_PARSE_RESULT(result, msg) \
    do { \
        if ((result) != 0) { \
            printf("fsc: Parse error - %s\n", (msg)); \
            goto cleanup; \
        } \
    } while(0)

#define FSC_PARSE_OR_GOTO(json, key, var, msg) \
    do { \
        if (ConvertcJSONToValue(json, key, &var) != 0) { \
            printf("fsc_parser: %s\n", msg); \
            goto cleanup; \
        } \
    } while(0)

// Function prototypes
int FscParseContext_Init(FscParseContext *ctx, const char *filename);
void FscParseContext_Cleanup(FscParseContext *ctx);
int ConvertcJSONToValue(cJSON *pMap, char *pcKey, void *pValue);

#endif // FSC_COMMON_H
