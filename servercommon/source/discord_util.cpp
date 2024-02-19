//
//  discord_util.cpp
//  servercommon
//
//  Created by Arun A on 16/02/24.
//

#include "discord_util.hpp"
#include <dpp/dpp.h>
#include "../../common/sdktypes.hpp"

qstring discord_util::current_web_hook = "https://discord.com/api/webhooks/1207911659214082058/A0S49aiBOJKVZJk5FUUQaAw3Qxl2oRmRFdf7R93B8Y60QPuagXS0F3gLKS3yYRQrTyo4";

void discord_util::set_web_hook(const qstring& web_hook) {
    //https://discord.com/api/webhooks/1207911659214082058/A0S49aiBOJKVZJk5FUUQaAw3Qxl2oRmRFdf7R93B8Y60QPuagXS0F3gLKS3yYRQrTyo4
    current_web_hook = web_hook;
}


int discord_util::send(const qstring& msg) {
     dpp::cluster bot(""); /* Normally, you put your bot token in here, but its not required for webhooks. */

     bot.on_log(dpp::utility::cout_logger());

    /* Construct a webhook object using the URL you got from Discord */
    dpp::webhook wh(current_web_hook.c_str());

    try {
        /* Send a message with this webhook */
        bot.execute_webhook_sync(wh, dpp::message(msg.c_str()));
    } catch (const dpp::rest_exception& e) {
        DEBUG_PRINT( LOG_LEVEL_0, __LOGTAG__, "Caught exception: %s", e.what());
        // Implement retry logic here, respecting the Retry-After header
    }
    return 0;
}
