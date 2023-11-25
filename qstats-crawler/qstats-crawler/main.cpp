//
//  main.cpp
//  qstats-crawler
//
//  Created by Arun A on 19/11/23.
//

#include <iostream>
#include "qstats-crawler.h"

int main(int argc, const char * argv[]) {
    // insert code here...
    init_gsdk();
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qstats crawler started...");
    
//    qstring server_utc_tstamp;
//    server_utc_tstamp.format("%ld", essentials::get_time_utc()) ;
//    qstring server_local_tstamp;
//    server_local_tstamp.format("%ld", essentials::get_time_local()) ;
//    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%s", server_utc_tstamp.c_str());
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%s", essentials::get_time_utc_tostring().c_str());
//    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%s", server_local_tstamp.c_str());
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%s", essentials::get_time_local_tostring().c_str());
    qstring server_utc_tstamp = essentials::get_time_utc_postgresql_format();
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%s", server_utc_tstamp.c_str());
    
    qstats_crawler crawler;
    crawler.try_crawl("./stats/qh3_statfile");
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qstats crawler finished...");
    return 0;
}
