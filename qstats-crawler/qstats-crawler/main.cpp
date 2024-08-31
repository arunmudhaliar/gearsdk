//
//  Copyright 2024 homenet25
//  main.cpp
//  qstats-crawler
//
//  Created by Arun A on 19/11/23.
//

#include "qstats-crawler.h"

#include <iostream>

static bool no_log_file_found_shown = false;
bool do_crawl(const fs::path& root_dir, const fs::path& file_pattern, const qstring& host, const qstring& port) {
	std::vector<fs::path> log_dirs;
	essentials::get_all_child_folders(root_dir, log_dirs);
	//	for (auto d : log_dirs) {
	//		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "--> %s", d.c_str());
	//	}
	bool result = false;
	qstats_crawler crawler;
	for (auto dir : log_dirs) {
		fs::path log_path = dir / file_pattern;
		crawler.try_crawl(log_path.c_str(), host, port, [&result, dir](const qstring& root_filename, const std::vector<fs::path>& files, qstats_crawler::CRAWL_EVENT event) {
			if (event == qstats_crawler::CRAWL_START) {
				DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qstats crawler started... --> %s", dir.native().c_str());
				if (files.size() > 50) {
					DEBUG_PRINT_ERROR(__LOGTAG__, "----- CLEAN UP LOG FOLDER ----- %s", dir.native().c_str());
					no_log_file_found_shown = false;
				} else if (files.size() == 0 && !no_log_file_found_shown) {
					DEBUG_PRINT_IMPORTANT(__LOGTAG__, "----- NO LOG FILE FOUND ----- %s", dir.native().c_str());
					no_log_file_found_shown = true;
				} else {
					no_log_file_found_shown = false;
				}
			} else {
				result = true;
				DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qstats crawler finished... --> %s", dir.native().c_str());
			}
		});
	}
	return result;
}

int main(int argc, const char* argv[]) {
	fs::path root_dir = "./stats";
	fs::path file_pattern = "qh3_statfile";
	qstring host = "127.0.0.1";
	qstring port = "5432";

	if (argc % 2 == 0) {
		DEBUG_PRINT_ERROR(__LOGTAG__, "Failed to resolve arguments !!!. Using default path %s, argc %d", root_dir.c_str(), argc);
		DEBUG_PRINT_IMPORTANT2(__LOGTAG__, "Usage : <executable> '--d <root dir>' --f <filepattern-prefix> --host <postgresql ip> --port <postgresql port>");
	} else {
		// default to root
		int pairs = (argc - 1) / 2;
		for (int x = 0; x < pairs; x++) {
			const char* lf = argv[1 + x * 2 + 0];
			const char* rg = argv[1 + x * 2 + 1];

			if (strcmp(lf, "--d") == 0) {
				root_dir = fs::path(rg);
			}
			if (strcmp(lf, "--f") == 0) {
				file_pattern = fs::path(rg);
			}
			if (strcmp(lf, "--host") == 0) {
				host = rg;
			}
			if (strcmp(lf, "--port") == 0) {
				port = rg;
			}
		}
		//
	}

	init_gsdk();

	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qstats crawler inited. root dir --> %s,  db --> %s:%s ", root_dir.native().c_str(), host.c_str(), port.c_str());

	do_crawl(root_dir, file_pattern, host, port);

	qtimer_sceduler scheduler;
	struct ev_loop* loop = ev_default_loop(0);
	scheduler.set_ev_lopp(loop);

	qtimer* keep_alive_loop = scheduler.schedule_repeat_timer(
		[loop, root_dir, file_pattern, host, port](qtimer& timer) {
			UNUSED(timer);
			ev_tstamp creation_time = ev_now(loop);
			if (do_crawl(root_dir, file_pattern, host, port)) {
				DEBUG_PRINT_IMPORTANT(__LOGTAG__, "CRAWL - t:%5.2fs", ev_now(loop) - creation_time);
			}
		},
		60);
	UNUSED(keep_alive_loop);
	ev_run(loop, 0);
	return 0;
}
