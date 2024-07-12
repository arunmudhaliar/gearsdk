//
//  servercommon.hpp
//  servercommon
//
//  Created by Arun A on 10/05/24.
//

#ifndef servercommon_hpp
#define servercommon_hpp

#include "../common/sdktypes.hpp"

namespace gsdk {
namespace servercommon {
extern "C" void init_server_common();
extern "C" void update_public_ip();
extern "C" size_t curl_write_cb_get_public_ip(void *contents, size_t size, size_t nmemb, void *userp);
}
}

#endif /* servercommon_hpp */
