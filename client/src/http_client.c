#include "http_client.h"

#include <curl/curl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "json_payload.h"
#include "state.h"

#define CURL_TIMEOUT_SECONDS 10L

static CURL *g_curl;
static struct curl_slist *g_hdrs;

static size_t curl_discard(void *ptr, size_t size, size_t nmemb, void *ud) {
    (void)ptr;
    (void)ud;
    return size * nmemb;
}

int http_client_init(void) {
    g_curl = curl_easy_init();
    if (!g_curl) {
        return -1;
    }
    g_hdrs = curl_slist_append(NULL, "Content-Type: application/json");
    g_hdrs = curl_slist_append(g_hdrs, "Authorization: " API_KEY);
    curl_easy_setopt(g_curl, CURLOPT_URL, API_URL);
    curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, g_hdrs);
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, curl_discard);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SECONDS);
    return 0;
}

void http_client_cleanup(void) {
    if (g_hdrs) {
        curl_slist_free_all(g_hdrs);
        g_hdrs = NULL;
    }
    if (g_curl) {
        curl_easy_cleanup(g_curl);
        g_curl = NULL;
    }
}

void flush_to_api(int force) {
    if (!force && !atomic_load_explicit(&g_had_activity, memory_order_acquire)) {
        return;
    }

    char *json = NULL;
    ssize_t len = build_json(
        g_total_lc,
        g_total_rc,
        g_total_mc,
        g_total_sc,
        g_total_kp,
        g_total_mm,
        &json
    );
    if (len < 0 || !json) {
        fprintf(stderr, "[collector] JSON build failed\n");
        return;
    }

    if (!g_curl) {
        free(json);
        return;
    }

    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, json);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDSIZE, (long)len);

    CURLcode res = curl_easy_perform(g_curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "[collector] curl error: %s\n", curl_easy_strerror(res));
    } else {
        fprintf(stderr, "[collector] sent timestamp and summary\n");
        atomic_store_explicit(&g_had_activity, 0, memory_order_release);
    }

    free(json);
}
