//
//  qstats-crawler.cpp
//  qstats-crawler
//
//  Created by Arun A on 25/11/23.
//

#include "qstats-crawler.h"

#define COUNT_STATS_BATCH_COUNT 200
#define OPEN_STATS_BATCH_COUNT 70

qstats_crawler::qstats_crawler() {
}

qstats_crawler::~qstats_crawler() {
}

void qstats_crawler::try_crawl(const qstring& root_filename) {
    if (pgsql_client.connect_db()!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect db. returning !!!");
        return;
    }
    
    fs::path crawled_dir = fs::path(root_filename.c_str()).parent_path() / fs::path("crawled");
    if (!fs::is_directory(crawled_dir)) {
        fs::create_directories(crawled_dir);
    }
    std::vector<fs::path> files;
    fs::path logfile_path = root_filename.c_str();
    qlogfile::get_all_log_files(logfile_path, files);

    for (fs::path f : files) {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "crawling...  %s", f.c_str());
        int parsed_lines = 0;
        if (parse_file(f, parsed_lines) == 0) {
            fs::path dest = crawled_dir / fs::path(f.c_str()).filename();
            int result = rename(f.c_str(), dest.c_str());
            if (!result) {
                DEBUG_PRINT_IMPORTANT(__LOGTAG__, "stats file crawled and moved to %s", dest.c_str());
            }
            else {
                DEBUG_PRINT_ERROR(__LOGTAG__, "failed to move stats file %s", f.c_str());
            }
        }
        else {
            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to crawl stats file %s", f.c_str());
        }
    }
    pgsql_client.close_db();
}

int qstats_crawler::parse_file(fs::path file, int& parsed_lines) {
    const int max_chars_in_a_line = 1024;
    char str[max_chars_in_a_line];

    std::string fname = file.string();
    /* opening file for reading */
    FILE* fp = fopen(file.string().c_str(), "r");
    if (fp == NULL) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Error opening stats file");
        return(-1);
    }
    parsed_lines = 0;
    while (fgets(str, max_chars_in_a_line, fp)) {
        /* writing content to stdout */
        puts(str);
        if (parse_line(qstring(str)) == 0) {
            parsed_lines++;
        }
    }
    // reminders if any
    if (count_stats_counter > 0) {
        if (batch_send_count_stats() != 0) {
            fclose(fp);
            return -1;
        }
    }
    if (open_stats_counter > 0) {
        if (batch_send_open_stats() != 0) {
            fclose(fp);
            return -1;
        }
    }
    //
    fclose(fp);
    return 0;
}

int qstats_crawler::parse_line(const qstring& line) {
    std::vector<qstring> list;
    line.split("|", list);

    if (list.size() == 0) {
        // nothing to parse.
        return -1;
    }

    if (list[0] == "count") {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "processing count stats ...");
        if (append_count_stats(batch_count_stats_values, list) == 0) {
            count_stats_counter++;
        }
        if (count_stats_counter >= COUNT_STATS_BATCH_COUNT) {
            batch_send_count_stats();
        }
    }
    else if (list[0] == "open") {
        DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "processing open stats ...");
        if (append_open_stats(batch_open_stats_values, list) == 0) {
            open_stats_counter++;
        }
        if (open_stats_counter >= OPEN_STATS_BATCH_COUNT) {
            batch_send_open_stats();
        }
    }
    else {
        DEBUG_PRINT_WARN(__LOGTAG__, "Unrecognised stats ... %s", list[0].c_str());
    }

    return 0;
}

/*
 CREATE TABLE qtest_pgdb_schema.stats_count (count TEXT, count_val bigint, session TEXT, pid TEXT, version TEXT,
                      epic TEXT, myth TEXT, legend TEXT, story TEXT,
                      install_os TEXT, server_tstamp timestamp, client_tstamp timestamp, time bigint, message TEXT,
                      device_name TEXT, device_model TEXT, total_ram INT4);

 CREATE TABLE qtest_pgdb_schema.stats_open (version TEXT, duid TEXT,
                      epic TEXT, myth TEXT, legend TEXT, story TEXT,
                      install_os TEXT, client_tstamp timestamp, time bigint,
                      device_name TEXT, device_model TEXT, total_ram INT4);
 */

int qstats_crawler::batch_send_count_stats() {
    qstring insert_header = qstring::format_string("INSERT INTO qtest_pgdb_schema.stats_count(count, count_val, session, pid, version, epic, myth, legend, story, install_os, server_tstamp, client_tstamp, time, message, device_name, device_model, total_ram) VALUES");
    qstring sql_script = insert_header + batch_count_stats_values;
    sql_script += ";";
    if (pgsql_client.execute_query(sql_script) !=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "pgsql_client.execute_query failed. returning !!!");
        return -1;
    }
    batch_count_stats_values.clear();
    count_stats_counter = 0;
    return 0;
}

int qstats_crawler::append_count_stats(qstring& values, std::vector<qstring>& list) {
    if (list.size() != 18) {
        DEBUG_PRINT_WARN(__LOGTAG__, "Unrecognised stats format ... size 16!=%d", list.size());
        return 1;
    }

    qstring format_string("(");
    for (size_t x = 1; x < list.size(); x++) {
        format_string += (list[x] == "NULL") ? "%s" : (x==2 || x==12) ? "%s" : "'%s'";
        if (x < list.size() - 1) {
            format_string += ",";
        }
    }
    format_string += ")";
    qstring insert_values = qstring::format_string(format_string.c_str(),
        list[1].c_str()
        ,list[2].c_str()
        ,list[3].c_str()
        ,list[4].c_str()
        ,list[5].c_str()
        ,list[6].c_str()
        ,list[7].c_str()
        ,list[8].c_str()
        ,list[9].c_str()
        ,list[10].c_str()
        ,list[11].c_str()
        ,list[12].c_str()
        ,list[13].c_str()
        ,list[14].c_str()
        ,list[15].c_str()
        ,list[16].c_str()
        ,list[17].c_str()
    );
    if (count_stats_counter > 0) {
        values += ",";
    }
    values += insert_values;
    return 0;
}

int qstats_crawler::parse_count_stats(std::vector<qstring>& list) {
    if (list.size() != 18) {
        DEBUG_PRINT_WARN(__LOGTAG__, "Unrecognised stats format ... size 18!=%d", list.size());
        return 1;
    }

    /*
     INSERT INTO
         qtest_pgdb_schema.stats_count(count, count_val, session, pid, version, epic, myth, legend, story, install_os, server_tstamp, client_tstamp, time, message, device_name, device_model, total_ram)
     VALUES
         ('0',0,'test0','','','','','','','','2020-07-06 09:30:00.646533','2020-07-06 09:30:00.646533', 0, '','','',0),
         ('1',0,'test2','','','','','','','','2020-07-06 09:30:00.646533','2020-07-06 09:30:00.646533', 0, '','','',0)

     */
    qstring insert_header = qstring::format_string("INSERT INTO qtest_pgdb_schema.stats_count(count, count_val, session, pid, version, epic, myth, legend, story, install_os, server_tstamp, client_tstamp, time, message, device_name, device_model, total_ram) VALUES");

    qstring format_string("(");
    for (size_t x = 1; x < list.size(); x++) {
        format_string += (list[x] == "NULL") ? "%s" : (x==2 || x==12) ? "%s" : "'%s'";
        if (x < list.size() - 1) {
            format_string += ",";
        }
    }
    format_string += ");";

    qstring insert_values = qstring::format_string(format_string.c_str(),
        list[1].c_str()
        ,list[2].c_str()
        ,list[3].c_str()
        ,list[4].c_str()
        ,list[5].c_str()
        ,list[6].c_str()
        ,list[7].c_str()
        ,list[8].c_str()
        ,list[9].c_str()
        ,list[10].c_str()
        ,list[11].c_str()
        ,list[12].c_str()
        ,list[13].c_str()
        ,list[14].c_str()
        ,list[15].c_str()
        ,list[16].c_str()
        ,list[17].c_str()
    );
    qstring sql_script = insert_header + insert_values;

    //    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%.*s", sql_script.length(), sql_script.c_str());
    pgsql_client.execute_query(sql_script);
    return 0;
}

int qstats_crawler::parse_open_stats(std::vector<qstring>& list) {
    UNUSED(list);
    return 0;
}

int qstats_crawler::append_open_stats(qstring& values, std::vector<qstring>& list) {
    UNUSED(values);
    UNUSED(list);
    return 0;
}

int qstats_crawler::batch_send_open_stats() {
    return 0;
}
