/*************************************************************************
 *
 * fsc_common.c
 * Common utility implementations for error handling and resource management
 *
 ************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Types.h"
#include "cJSON.h"
#include "fsc_common.h"
#include "fsc_utils.h"
#include "fsc_parser.h"

/*---------------------------------------------------------------------------
* @fn FscParseContext_Init
*
* @brief Initializes a parse context and loads JSON file
* @param[out] ctx Parse context to initialize
* @param[in] filename JSON file path
* @return FSC_OK on success, error code on failure
*---------------------------------------------------------------------------*/
int FscParseContext_Init(FscParseContext *ctx, const char *filename)
{
    FILE *file = NULL;
    long length = 0;
    size_t read_chars = 0;

    FSC_CHECK_NULL(ctx, "parse context");
    FSC_CHECK_NULL(filename, "filename");

    ctx->file_buffer = NULL;
    ctx->json_root = NULL;
    ctx->filename = filename;

    // Open file
    file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("fsc_parser: Failed to open file %s\n", filename);
        return FSC_ERR_IO;
    }

    // Get file size
    if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) < 0 || fseek(file, 0, SEEK_SET) != 0)
    {
        fclose(file);
        return FSC_ERR_IO;
    }

    // Allocate buffer
    ctx->file_buffer = (char *)malloc((size_t)length + 1);
    if (ctx->file_buffer == NULL)
    {
        fclose(file);
        return FSC_ERR_NULL;
    }

    // Read file
    read_chars = fread(ctx->file_buffer, sizeof(char), (size_t)length, file);
    fclose(file);

    if (read_chars != (size_t)length)
    {
        free(ctx->file_buffer);
        ctx->file_buffer = NULL;
        return FSC_ERR_IO;
    }

    ctx->file_buffer[read_chars] = '\0';

    // Parse JSON
    ctx->json_root = cJSON_Parse(ctx->file_buffer);
    if (ctx->json_root == NULL || cJSON_IsInvalid(ctx->json_root))
    {
        free(ctx->file_buffer);
        ctx->file_buffer = NULL;
        return FSC_ERR_PARSE;
    }

    return FSC_OK;

}

/*---------------------------------------------------------------------------
* @fn FscParseContext_Cleanup
*
* @brief Cleans up parse context resources
* @param[in] ctx Parse context to clean up
*---------------------------------------------------------------------------*/
void FscParseContext_Cleanup(FscParseContext *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    if (ctx->json_root != NULL)
    {
        cJSON_Delete(ctx->json_root);
        ctx->json_root = NULL;
    }

    if (ctx->file_buffer != NULL)
    {
        free(ctx->file_buffer);
        ctx->file_buffer = NULL;
    }

    ctx->filename = NULL;
}

/*---------------------------------------------------------------------------
* @fn ConvertcJSONToValue
*
* @brief Extracts a value from a cJSON object based on a given key.
* @param[in] pMap A pointer to the cJSON object to search within.
* @param[in] pcKey The key string for the desired value.
* @param[out] pValue A pointer to a variable to store the extracted value.
*                    For numbers, this should be a pointer to a double.
*                    For strings/objects, this should be a char buffer.
* @return 0 on success, or a negative value on failure.
*         -1: pMap, pcKey, pValue is NULL or key not found.
*---------------------------------------------------------------------------*/
int ConvertcJSONToValue(cJSON *pMap, char *pcKey, void *pValue)
{
    int ret = 0;
    cJSON *pNode = NULL;

    if (!pMap || !pcKey || !pValue)
    {
        printf("fsc_parser: invalid args in ConvertcJSONToValue\n");
        return -1;
    }

    pNode = cJSON_GetObjectItem(pMap, pcKey);
    if (!pNode)
    {
        return -1;
    }

    if (cJSON_IsNumber(pNode))
    {
        *(double *)pValue = pNode->valuedouble;
    }
    else if (cJSON_IsString(pNode) && pNode->valuestring)
    {
        snprintf(pValue, LABEL_LENGTH_MAX, "%s", pNode->valuestring);
    }
    else
    {
        ret = -1;
    }

    return ret;
}
