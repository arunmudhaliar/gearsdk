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

    void try_crawl(const qstring& root_filename, const qstring& host, const qstring& port);

private:
    int parse_file(fs::path file, int& parsed_lines);
    int parse_line(fs::path current_file, const qstring& line);
    int parse_count_stats(fs::path current_file, std::vector<qstring>& list);
    int append_count_stats(qstring& values, std::vector<qstring>& list);
    int batch_send_count_stats(fs::path current_file);
    int parse_open_stats(fs::path current_file, std::vector<qstring>& list);
    int append_open_stats(qstring& values, std::vector<qstring>& list);
    int batch_send_open_stats(fs::path current_file);

    qpgsql pgsql_client;
    int count_stats_counter = 0;
    int total_records_sent_to_db_through_batching = 0;
    qstring batch_count_stats_values;
    int open_stats_counter = 0;
    qstring batch_open_stats_values;
};
#endif /* qstats_crawler_h */
