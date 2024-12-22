//
//  servercommon.cpp
//  servercommon
//
//  Created by Arun A on 10/05/24.
//

#include "servercommon.hpp"

#include <curl/curl.h>

extern "C" {
void init_server_common() {
	gsdk::servercommon::update_public_ip();
}

size_t curl_write_cb_get_public_ip(void* contents, size_t size, size_t nmemb, void* userp) {
	size_t realsize = size * nmemb;
	strncpy(gsdk::device::public_ip, (char*) contents, realsize);
	debug_print(LOG_LEVEL_0, __DEFAULT_LOG_TAG__, "Public ip : %s", gsdk::device::public_ip);
	return realsize;
}

void update_public_ip() {
	CURL* curl;
	CURLcode res;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "http://api.ipify.org");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb_get_public_ip);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			debug_print_error(__DEFAULT_LOG_TAG__, "update_public_ip : curl_easy_perform() failed: %s", curl_easy_strerror(res));

		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
}
}
