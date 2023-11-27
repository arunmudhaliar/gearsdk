//
//  qstats-crawler.h
//  qstats-crawler
//
//  Created by Arun A on 25/11/23.
//

#ifndef qstats_crawler_h
#define qstats_crawler_h
#include "../qpgsql/qpgsql.hpp"
#include "../../networkcommon/source/qstatslogger.hpp"

class qstats_crawler {
public:
    qstats_crawler();
    ~qstats_crawler();
    
    void try_crawl(const qstring& root_filename);
    
private:
    int parse_file(fs::path file, int& parsed_lines);
    int parse_line(const qstring& line);
    int parse_count_stats(std::vector<qstring>& list);
    int parse_open_stats(std::vector<qstring>& list);
    
    qpgsql pgsql_client;
};
#endif /* qstats_crawler_h */
