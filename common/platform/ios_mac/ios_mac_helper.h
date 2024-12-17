//
//  ios_mac_helper.h
//  common
//
//  Created by Arun A on 10/12/24.
//

#ifndef ios_mac_helper_h
#define ios_mac_helper_h
#if defined(__APPLE__)
#include "../../sdktypes.hpp"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define APPLE_APP_SERVICE_NAME "gsdk-app-service"
#define APPLE_DEVICE_USERNAME "gsdk_account"

#undef __LOGTAG__
#define __LOGTAG__ "gsdk-platform-apple"

extern "C" {
namespace gsdk {
namespace platform {
namespace apple {

class ios_mac_helper {
   public:
	// Caller must free
	static char* cf_string_to_c_string(CFStringRef cf_string);
	// Caller must free
	static char* c_string_copy(const char* c_str);
	// Caller must free
	static char* get_locale();
	// Caller must free
	static char* get_duid(const char* service_name = APPLE_APP_SERVICE_NAME, const char* account = APPLE_DEVICE_USERNAME);
};
}  // namespace apple
}  // namespace platform
}  // namespace gsdk
}
#endif
#endif /* ios_mac_helper_h */
