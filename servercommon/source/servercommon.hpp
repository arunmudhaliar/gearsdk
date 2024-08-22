/**
 * @file servercommon.hpp
 * @brief This file contains the declaration of the server common module, which provides
 * functionality for initializing the server common module, updating the public IP
 * address, and a callback function for writing public IP address using cURL.
 *
 * @author Arun A
 * @date 2024
 * @copyright 2024 homenet25
 */

#ifndef servercommon_hpp
#define servercommon_hpp

#include "../common/sdktypes.hpp"

namespace gsdk {
namespace servercommon {

/**
 * @brief Initializes the server common module.
 */
extern "C" void init_server_common();

/**
 * @brief Updates the public IP address.
 */
extern "C" void update_public_ip();

/**
 * @brief Callback function for writing public IP address using cURL.
 *
 * @param contents Pointer to the received data.
 * @param size Size of each element.
 * @param nmemb Number of elements.
 * @param userp Pointer to user data.
 * @return The number of bytes written.
 */
extern "C" size_t curl_write_cb_get_public_ip(void* contents, size_t size, size_t nmemb, void* userp);

}  // namespace servercommon
}  // namespace gsdk

#endif /* servercommon_hpp */
