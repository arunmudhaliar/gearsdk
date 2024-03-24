//
//  main.cpp
//  qstats-crawler
//
//  Created by Arun A on 19/11/23.
//

#include "qstats-crawler.h"

#include <iostream>

void do_crawl(const fs::path& root_dir, const fs::path& file_pattern, const qstring& host, const qstring& port) {
	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qstats crawler started...");
	std::vector<fs::path> log_dirs;
	essentials::get_all_child_folders(root_dir, log_dirs);
	for (auto d : log_dirs) {
		DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "--> %s", d.c_str());
	}
	qstats_crawler crawler;
	for (auto dir : log_dirs) {
		fs::path log_path = dir / file_pattern;
		crawler.try_crawl(log_path.c_str(), host, port);
	}
	DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "qstats crawler finished...");
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

	do_crawl(root_dir, file_pattern, host, port);

	qtimer_sceduler scheduler;
	struct ev_loop* loop = ev_default_loop(0);
	ev_tstamp creation_time = ev_now(loop);
	scheduler.set_ev_lopp(loop);

	qtimer* keep_alive_loop = scheduler.schedule_repeat_timer(
		[loop, creation_time, root_dir, file_pattern, host, port](qtimer& timer) {
			UNUSED(timer);
			do_crawl(root_dir, file_pattern, host, port);
			DEBUG_PRINT_IMPORTANT(__LOGTAG__, "CRAWL - t:%5.2fs", ev_now(loop) - creation_time);
		},
		60);
	UNUSED(keep_alive_loop);
	ev_run(loop, 0);
	return 0;
}
