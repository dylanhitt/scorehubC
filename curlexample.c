#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <json-c/json.h>

struct curl_fetch_st
{
    char *payload;
    size_t size;
};

struct myStruct
{
    int id;
};

/* callback for curl fetch */
size_t curl_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;                          /* calculate buffer size */
    struct curl_fetch_st *p = (struct curl_fetch_st *)userp; /* cast pointer to fetch struct */

    /* expand buffer using a temporary pointer to avoid memory leaks */
    char *temp = realloc(p->payload, p->size + realsize + 1);

    /* check allocation */
    if (temp == NULL)
    {
        /* this isn't good */
        fprintf(stderr, "ERROR: Failed to expand buffer in curl_callback");
        /* free buffer */
        free(p->payload);
        /* return */
        return 1;
    }

    /* assign payload */
    p->payload = temp;

    /* copy contents to buffer */
    memcpy(&(p->payload[p->size]), contents, realsize);

    /* set new buffer size */
    p->size += realsize;

    /* ensure null termination */
    p->payload[p->size] = 0;

    /* return size */
    return realsize;
}

/* fetch and return url body via curl */
CURLcode curl_fetch_url(CURL *ch, const char *url, struct curl_fetch_st *fetch)
{
    CURLcode rcode; /* curl result code */

    /* init payload */
    fetch->payload = (char *)calloc(1, sizeof(fetch->payload));

    /* check payload */
    if (fetch->payload == NULL)
    {
        /* log error */
        fprintf(stderr, "ERROR: Failed to allocate payload in curl_fetch_url");
        /* return error */
        return CURLE_FAILED_INIT;
    }

    /* init size */
    fetch->size = 0;

    /* set url to fetch */
    curl_easy_setopt(ch, CURLOPT_URL, url);

    /* set calback function */
    curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, curl_callback);

    /* pass fetch struct pointer */
    curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *)fetch);

    /* set default user agent */
    curl_easy_setopt(ch, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* set timeout */
    curl_easy_setopt(ch, CURLOPT_TIMEOUT, 15);

    /* enable location redirects */
    curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);

    /* set maximum allowed redirects */
    curl_easy_setopt(ch, CURLOPT_MAXREDIRS, 1);

    /* fetch the url */
    rcode = curl_easy_perform(ch);

    /* return */
    return rcode;
}

int main(void)
{
    CURL *curl;
    CURLcode response;
    struct curl_fetch_st curl_fetch;
    struct curl_fetch_st *cf = &curl_fetch; /* pointer to fetch struct */

    json_object *json;
    enum json_tokener_error jerr = json_tokener_success; /* json parse error */

    /* In windows, this will init the winsock stuff */
    curl_global_init(CURL_GLOBAL_ALL);

    // The object with data we are sending in.
    cJSON *dataObj;
    // Creating json object
    dataObj = cJSON_CreateObject();
    // Adding 1 number and 3 string to the object
    // Chosing first which object to add it to, keyname, value. Function depending on type
    cJSON_AddItemToObject(dataObj, "analysType", cJSON_CreateNumber(1));
    cJSON_AddItemToObject(dataObj, "data", cJSON_CreateString("AJHDGAHSJDHAS=="));
    cJSON_AddItemToObject(dataObj, "deviceId", cJSON_CreateString("EA:61:59:19:93:9"));
    cJSON_AddItemToObject(dataObj, "startTime", cJSON_CreateString("2020-12-21 13:03:12"));

    /* Get a curl handle */
    curl = curl_easy_init();

    if (curl)
    {
        /* Initialize struct that will hold auth token for request header */
        struct curl_slist *headers = NULL;

        /* Add request headers */
        headers = curl_slist_append(headers, "Accept: application/json");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, "charset: utf-8");

        /* Set request header */
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Setting the body with data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, cJSON_Print(dataObj));

        /* Set URL for POST */
        char *url = "http://jsonplaceholder.typicode.com/posts/";

        /* Post the HTTP request, and store response in "response" variable */
        response = curl_fetch_url(curl, url, cf);
        /* Check for errors */
        if (response != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(response));

        /* Cleanup */
        curl_easy_cleanup(curl);

        /* free headers */
        curl_slist_free_all(headers);

        /* check return code */
        if (response != CURLE_OK || cf->size < 1)
        {
            /* log error */
            fprintf(stderr, "ERROR: Failed to fetch url (%s) - curl said: %s",
                    url, curl_easy_strerror(response));
            /* return error */
            return 1;
        }

        /* check payload */
        if (cf->payload != NULL)
        {
            /* print result */
            printf("CURL Returned: \n%s\n", cf->payload);
            /* parse return */
            json = json_tokener_parse_verbose(cf->payload, &jerr);
            /* free payload */
            free(cf->payload);
        }
        else
        {
            /* error */
            fprintf(stderr, "ERROR: Failed to populate payload");
            /* free payload */
            free(cf->payload);
            /* return */
            return 1;
        }

        /* check error */
        if (jerr != json_tokener_success)
        {
            /* error */
            fprintf(stderr, "ERROR: Failed to parse json string");
            /* free json object */
            json_object_put(json);
            /* return */
            return 1;
        }

        /* debugging */
        printf("Parsed JSON: %s\n", json_object_to_json_string(json));

        struct myStruct myS;
        myS.id = json_object_get_int(json_object_object_get(json, "analysType"));

        /* free json object before return */
        json_object_put(json);

        printf("ID: %d\n", myS.id);

        /* exit */
        return 0;
    }

    return 0;
}
