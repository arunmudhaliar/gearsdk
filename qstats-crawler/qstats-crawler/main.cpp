//
//  main.cpp
//  qstats-crawler
//
//  Created by Arun A on 19/11/23.
//

#include <iostream>
#include "qstats-crawler.h"

int main(int argc, const char * argv[]) {
    UNUSED(argc);
    UNUSED(argv);
    
    fs::path root_file_path = "./stats/qh3_statfile";
    
    if (argc%2<2) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Failed to resolve arguments !!!. Using default path %s", root_file_path.c_str());
        DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Usage : <executable> '--f <root file name>'");
    } else {
        // default to root
        int pairs = (argc-1) / 2;
        for(int x=0;x<pairs;x++) {
            const char* lf = argv[1+x*2+0];
            const char* rg = argv[1+x*2+1];
            
            if (strcmp(lf, "--f")==0) {
                root_file_path = fs::path(rg);
            }
        }
        //
    }
    
    init_gsdk();
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qstats crawler started...");
    qstats_crawler crawler;
    crawler.try_crawl(root_file_path.c_str());
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qstats crawler finished...");
    return 0;
}
