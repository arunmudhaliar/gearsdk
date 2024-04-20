//
//  Copyright 2024 homenet25
//  qstats-crawler.cpp
//  qstats-crawler
//
//  Created by Arun A on 25/11/23.
//

#include "qstats-crawler.h"

#define COUNT_STATS_BATCH_COUNT 200
#define OPEN_STATS_BATCH_COUNT 70

qstats_crawler::qstats_crawler() {}

qstats_crawler::~qstats_crawler() {}

void qstats_crawler::try_crawl(const qstring& root_filename, const qstring& host, const qstring& port, type_qstats_crawler_crawl_event_cb event_cb) {
	if (pgsql_client.connect_db(host, port) != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "failed to connect db. returning !!!");
		return;
	}

	fs::path crawled_dir = fs::path(root_filename.c_str()).parent_path() / fs::path("crawled");
	if (!fs::is_directory(crawled_dir)) {
		fs::create_directories(crawled_dir);
	}
	fs::path crawl_failed_dir = fs::path(root_filename.c_str()).parent_path() / fs::path("crawl-failed");
	if (!fs::is_directory(crawl_failed_dir)) {
		fs::create_directories(crawl_failed_dir);
	}

	std::vector<fs::path> files;
	fs::path logfile_path = root_filename.c_str();
	qlogfile::get_all_log_files(logfile_path, files, false);

    bool have_files_to_process = (files.size()>0);
    if (have_files_to_process)
    {
        event_cb(root_filename, files, CRAWL_START);
    }
	for (fs::path f : files) {
		DEBUG_PRINT_IMPORTANT(__LOGTAG__, "crawling...  %s", f.c_str());
		int parsed_lines = 0;
		if (parse_file(f, parsed_lines) == 0) {
			fs::path dest = crawled_dir / fs::path(f.c_str()).filename();
			int result = rename(f.c_str(), dest.c_str());
			if (!result) {
				DEBUG_PRINT_IMPORTANT(__LOGTAG__, "stats file crawled and moved to %s", dest.c_str());
			} else {
				DEBUG_PRINT_ERROR(__LOGTAG__, "failed to move stats file %s", f.c_str());
			}
		} else {
			fs::path dest = crawl_failed_dir / fs::path(f.c_str()).filename();
			int result = rename(f.c_str(), dest.c_str());
			if (!result) {
				DEBUG_PRINT_IMPORTANT(__LOGTAG__, "stats file failed while parsing !!!. moved to %s", dest.c_str());
			} else {
				DEBUG_PRINT_ERROR(__LOGTAG__, "failed to move parse-failed-stats file %s", f.c_str());
			}
			//            DEBUG_PRINT_ERROR(__LOGTAG__, "failed to crawl stats file %s", f.c_str());
		}
	}
	pgsql_client.close_db();
    if (have_files_to_process)
    {
        event_cb(root_filename, files, CRAWL_STOP);
    }
}

int qstats_crawler::parse_file(fs::path file, int& parsed_lines) {
	const int max_chars_in_a_line = 1024;
	char str[max_chars_in_a_line];
	total_records_sent_to_db_through_batching = 0;

	std::string fname = file.string();
	/* opening file for reading */
	FILE* fp = fopen(file.string().c_str(), "r");
	if (fp == NULL) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "Error opening stats file");
		return (-1);
	}
	parsed_lines = 0;
	while (fgets(str, max_chars_in_a_line, fp)) {
		/* writing content to stdout */
		// puts(str);
		if (parse_line(file, qstring(str)) == 0) {
			parsed_lines++;
		} else {
			fclose(fp);
			return -1;
		}
	}
	// reminders if any
	if (count_stats_counter > 0) {
		if (batch_send_count_stats(file) != 0) {
			fclose(fp);
			return -1;
		}
	}
	if (open_stats_counter > 0) {
		if (batch_send_open_stats(file) != 0) {
			fclose(fp);
			return -1;
		}
	}
	//
	fclose(fp);
	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%d records sent to stats db through batching.", total_records_sent_to_db_through_batching);
	return 0;
}

int qstats_crawler::parse_line(fs::path current_file, const qstring& line) {
	std::vector<qstring> list;
	line.split("|", list);

	if (list.size() == 0) {
		// nothing to parse.
		DEBUG_PRINT_WARN(__LOGTAG__, "list.size() == 0 : %s", current_file.string().c_str());
		return 0;
	}

	qstring single_quote("'");
	qstring single_hash("#");
	for (int x = 0; x < list.size(); x++) {
		list[x].replace(single_quote, single_hash);
	}
	if (list[0] == "count") {
		DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "processing count stats ...");
		if (append_count_stats(batch_count_stats_values, list) == 0) {
			count_stats_counter++;
		}
		if (count_stats_counter >= COUNT_STATS_BATCH_COUNT) {
			if (batch_send_count_stats(current_file) != 0) {
				return -1;
			}
		}
	} else if (list[0] == "open") {
		DEBUG_PRINT(LOG_LEVEL_3, __LOGTAG__, "processing open stats ...");
		if (append_open_stats(batch_open_stats_values, list) == 0) {
			open_stats_counter++;
		}
		if (open_stats_counter >= OPEN_STATS_BATCH_COUNT) {
			if (batch_send_open_stats(current_file) != 0) {
				return -1;
			}
		}
	} else {
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

int qstats_crawler::batch_send_count_stats(fs::path current_file) {
	qstring insert_header = qstring::format_string(
		"INSERT INTO qtest_pgdb_schema.stats_count(count, count_val, session, pid, version, epic, myth, legend, story, install_os, server_tstamp, client_tstamp, time, message, device_name, device_model, total_ram) VALUES");
	qstring sql_script = insert_header + batch_count_stats_values;
	sql_script += ";";
	if (pgsql_client.execute_query(sql_script) != 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "pgsql_client.execute_query failed. returning !!!");
		batch_count_stats_values.clear();
		count_stats_counter = 0;

		// write the error on to error file.
		fs::path current_stats_filename = fs::path(current_file).filename().replace_extension("txt");
		qstring stats_error_file_name = qstring::format_string("stats_error_out-%s", current_stats_filename.c_str());
		fs::path error_file_path = fs::path(current_file).parent_path() / fs::path(stats_error_file_name.c_str());

		FILE* err_file = fopen(error_file_path.c_str(), "a");
		if (err_file != nullptr) {
			fprintf(err_file, "%.*s", (int) sql_script.length(), sql_script.c_str());
			fclose(err_file);
		}
		return -1;
	}
	batch_count_stats_values.clear();
	total_records_sent_to_db_through_batching += count_stats_counter;
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
		format_string += (list[x] == "NULL") ? "%s" : (x == 2 || x == 12) ? "%s" : "'%s'";
		if (x < list.size() - 1) {
			format_string += ",";
		}
	}
	format_string += ")";
	qstring insert_values = qstring::format_string(format_string.c_str(), list[1].c_str(), list[2].c_str(), list[3].c_str(), list[4].c_str(), list[5].c_str(), list[6].c_str(), list[7].c_str(), list[8].c_str(), list[9].c_str(),
												   list[10].c_str(), list[11].c_str(), list[12].c_str(), list[13].c_str(), list[14].c_str(), list[15].c_str(), list[16].c_str(), list[17].c_str());
	if (count_stats_counter > 0) {
		values += ",";
	}
	values += insert_values;
	return 0;
}

int qstats_crawler::parse_count_stats(fs::path current_file, std::vector<qstring>& list) {
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
	qstring insert_header = qstring::format_string(
		"INSERT INTO qtest_pgdb_schema.stats_count(count, count_val, session, pid, version, epic, myth, legend, story, install_os, server_tstamp, client_tstamp, time, message, device_name, device_model, total_ram)\n VALUES");

	qstring format_string("(");
	for (size_t x = 1; x < list.size(); x++) {
		format_string += (list[x] == "NULL") ? "%s" : (x == 2 || x == 12) ? "%s" : "'%s'";
		if (x < list.size() - 1) {
			format_string += ",";
		}
	}
	format_string += ");";

	qstring insert_values = qstring::format_string(format_string.c_str(), list[1].c_str(), list[2].c_str(), list[3].c_str(), list[4].c_str(), list[5].c_str(), list[6].c_str(), list[7].c_str(), list[8].c_str(), list[9].c_str(),
												   list[10].c_str(), list[11].c_str(), list[12].c_str(), list[13].c_str(), list[14].c_str(), list[15].c_str(), list[16].c_str(), list[17].c_str());
	qstring sql_script = insert_header + insert_values;

	//    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "%.*s", sql_script.length(), sql_script.c_str());
	pgsql_client.execute_query(sql_script);
	return 0;
}

int qstats_crawler::parse_open_stats(fs::path current_file, std::vector<qstring>& list) {
	UNUSED(list);
	return 0;
}

int qstats_crawler::append_open_stats(qstring& values, std::vector<qstring>& list) {
	UNUSED(values);
	UNUSED(list);
	return 0;
}

int qstats_crawler::batch_send_open_stats(fs::path current_file) {
	return 0;
}
