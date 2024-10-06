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
		debug_print_error(__LOGTAG__, "failed to connect db. returning !!!");
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

	bool have_files_to_process = (files.size() > 0);
	if (have_files_to_process) {
		event_cb(root_filename, files, CRAWL_START);
	}
	for (fs::path f : files) {
		debug_print_important(__LOGTAG__, "crawling...  %s", f.c_str());
		int parsed_lines = 0;
		if (parse_file(f, parsed_lines) == 0) {
			fs::path dest = crawled_dir / fs::path(f.c_str()).filename();
			int result = rename(f.c_str(), dest.c_str());
			if (!result) {
				debug_print_important(__LOGTAG__, "stats file crawled and moved to %s", dest.c_str());
			} else {
				debug_print_error(__LOGTAG__, "failed to move stats file %s", f.c_str());
			}
		} else {
			fs::path dest = crawl_failed_dir / fs::path(f.c_str()).filename();
			int result = rename(f.c_str(), dest.c_str());
			if (!result) {
				debug_print_important(__LOGTAG__, "stats file failed while parsing !!!. moved to %s", dest.c_str());
			} else {
				debug_print_error(__LOGTAG__, "failed to move parse-failed-stats file %s", f.c_str());
			}
			//            debug_print_error(__LOGTAG__, "failed to crawl stats file %s", f.c_str());
		}
	}
	pgsql_client.close_db();
	if (have_files_to_process) {
		event_cb(root_filename, files, CRAWL_STOP);
	}
}

int qstats_crawler::parse_file(fs::path file, int& parsed_lines) {
	const int MAX_CHARS_IN_A_LINE = 1024;
	char str[MAX_CHARS_IN_A_LINE];
	total_records_sent_to_db_through_batching = 0;
    batches.clear();
    
	std::string fname = file.string();
	/* opening file for reading */
	FILE* fp = fopen(file.string().c_str(), "r");
	if (fp == NULL) {
		debug_print_error(__LOGTAG__, "Error opening stats file");
		return (-1);
	}
	parsed_lines = 0;
	while (fgets(str, MAX_CHARS_IN_A_LINE, fp)) {
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
	if (batches.size() > 0) {
		batch_send_count_stats(file);
	}
	if (open_stats_counter > 0) {
		if (batch_send_open_stats(file) != 0) {
			fclose(fp);
			return -1;
		}
	}
	//
	fclose(fp);
	debug_print(LOG_LEVEL_0, __LOGTAG__, "%d records sent to stats db through batching.", total_records_sent_to_db_through_batching);
	return 0;
}

int qstats_crawler::get_number_of_count_stats() {
	int total = 0;
	for (auto batch : batches) {
		total += batch.second.count;
	}
	return total;
}

int qstats_crawler::parse_line(fs::path current_file, const qstring& line) {
	std::vector<qstring> list;
	line.split("|", list);

	if (list.size() == 0) {
		// nothing to parse.
		debug_print_warn(__LOGTAG__, "list.size() == 0, file: %s, line %s", current_file.string().c_str(), line.c_str());
		return 0;
	}

	qstring single_quote("'");
	qstring single_hash("#");
	for (int x = 0; x < list.size(); x++) {
		list[x].replace(single_quote, single_hash);
	}
	if (list[0] == "count") {
		debug_print(LOG_LEVEL_3, __LOGTAG__, "processing count stats ...");
		append_count_stats(list);
		if (get_number_of_count_stats() >= COUNT_STATS_BATCH_COUNT) {
			batch_send_count_stats(current_file);
		}
	} else if (list[0] == "open") {
		debug_print(LOG_LEVEL_3, __LOGTAG__, "processing open stats ...");
		if (append_open_stats(batch_open_stats_values, list) == 0) {
			open_stats_counter++;
		}
		if (open_stats_counter >= OPEN_STATS_BATCH_COUNT) {
			if (batch_send_open_stats(current_file) != 0) {
				return -1;
			}
		}
	} else {
		debug_print_warn(__LOGTAG__, "Unrecognised stats ... %s", list[0].c_str());
	}

	return 0;
}

int qstats_crawler::batch_send_count_stats(fs::path current_file) {
	int previous_total = total_records_sent_to_db_through_batching;
	for (auto itr : batches) {
		qstring insert_header = qstring::format_string(
			"INSERT INTO qtest_pgdb_schema.stats_%s(count_val, session, pid, version, epic, myth, legend, story, install_os, server_tstamp, client_tstamp, time, message, device_name, device_model, total_ram) VALUES", itr.first.c_str());
		qstring sql_script = insert_header + itr.second.value;
		sql_script += ";";
		if (pgsql_client.execute_query(sql_script) != 0) {
			debug_print_error(__LOGTAG__, "pgsql_client.execute_query failed. returning !!!");
			// write the error on to error file.
			fs::path current_stats_filename = fs::path(current_file).filename().replace_extension("txt");
			qstring stats_error_file_name = qstring::format_string("stats_error_out-%s", current_stats_filename.c_str());
			fs::path error_file_path = fs::path(current_file).parent_path() / fs::path(stats_error_file_name.c_str());

			FILE* err_file = fopen(error_file_path.c_str(), "a");
			if (err_file != nullptr) {
				fprintf(err_file, "%.*s", (int) sql_script.length(), sql_script.c_str());
				fclose(err_file);
			}
			continue;
		}
		total_records_sent_to_db_through_batching += itr.second.count;
	}
	batches.clear();
	return total_records_sent_to_db_through_batching - previous_total;
}

int qstats_crawler::append_count_stats(std::vector<qstring>& list) {
	if (list.size() != 18) {
		debug_print_warn(__LOGTAG__, "Unrecognised stats format ... size 18!=%d", list.size());
		return 1;
	}

	qstring format_string("(");
	for (size_t x = 2; x < list.size(); x++) {
		format_string += (list[x] == "NULL") ? "%s" : (x == 2 || x == 12) ? "%s" : "'%s'";
		if (x < list.size() - 1) {
			format_string += ",";
		}
	}
	format_string += ")";
	qstring insert_values = qstring::format_string(format_string.c_str(), list[2].c_str(), list[3].c_str(), list[4].c_str(), list[5].c_str(), list[6].c_str(), list[7].c_str(), list[8].c_str(), list[9].c_str(), list[10].c_str(),
												   list[11].c_str(), list[12].c_str(), list[13].c_str(), list[14].c_str(), list[15].c_str(), list[16].c_str(), list[17].c_str());
	const qstring& key = list[0] + "_" + list[1];
	if (batches.find(key) != batches.end()) {
		batches[list[0] + "_" + list[1]].value += ",";
	}
	batches[list[0] + "_" + list[1]].value += insert_values;
	batches[list[0] + "_" + list[1]].count++;
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

/*
 DO $$
 DECLARE
	 table_names text[] := ARRAY['flush_egress'
								 'validate_token',
								 'parse',
								 'recv_cb'
								];
	 current_table text;
 BEGIN
	 FOREACH current_table IN ARRAY table_names
	 LOOP
		 -- Create the table
		 EXECUTE format('CREATE TABLE IF NOT EXISTS qtest_pgdb_schema.stats_count_%I
		 (
			 count_val bigint,
			 session text COLLATE pg_catalog."default",
			 pid text COLLATE pg_catalog."default",
			 version text COLLATE pg_catalog."default",
			 epic text COLLATE pg_catalog."default",
			 myth text COLLATE pg_catalog."default",
			 legend text COLLATE pg_catalog."default",
			 story text COLLATE pg_catalog."default",
			 install_os text COLLATE pg_catalog."default",
			 server_tstamp timestamp without time zone NOT NULL,  -- Add NOT NULL constraint
			 client_tstamp timestamp without time zone,
			 "time" bigint,
			 message text COLLATE pg_catalog."default",
			 device_name text COLLATE pg_catalog."default",
			 device_model text COLLATE pg_catalog."default",
			 total_ram integer
		 ) PARTITION BY RANGE (server_tstamp);', current_table);

		 -- Create the parent partition
		 EXECUTE format('SELECT partman.create_parent(
			 p_parent_table => ''qtest_pgdb_schema.stats_count_%I'',
			 p_control => ''server_tstamp'',      -- Column to partition by
			 p_interval => ''5 days'',             -- Partitioning interval
			 p_type => ''range'',                   -- Use time-based partitioning
			 p_premake => 7
		 );', current_table);
	 END LOOP;
 END $$;
 */
