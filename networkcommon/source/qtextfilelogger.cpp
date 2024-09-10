//
//  Copyright 2024 homenet25
//  qtextfilelogger.cpp
//  networkcommon
//
//  Created by Arun A on 01/11/23.
//

#include "qtextfilelogger.hpp"

#include "../../common/sdktypes.hpp"

#include <iostream>
#include <time.h>
#include <unistd.h>

qlogfile::qlogfile(const qstring& path, uint64_t max_size_of_file) : logfile_path(path), max_size_of_file(max_size_of_file) {
	fs::path log_dir = fs::path(logfile_path.c_str()).parent_path();
	if (!fs::is_directory(log_dir)) {
		fs::create_directories(log_dir);
	}
	create_new_logfile(false);
}

qlogfile::~qlogfile() {
	// move pending records if any
	// This may cause some ordering issue. Will fix later. For stats it wont be an issue since timestamp attached to each stat.
	for (std::vector<qbuffer*>::iterator it = records_waiting.begin(); it != records_waiting.end(); it++) {
		records.push_back(*it);
	}
	int flush_result = flush(false);
	if (flush_result < 0) {
		debug_print_error(__LOGTAG__, "failed to flush records in qlogfile destructor - %s, flush_result %d", logfile_path.c_str(), flush_result);
		// Cann not delete (Need to implement conditional wait for mutex here)
		//        for (std::vector<qbuffer*>::iterator it = records.begin(); it!=records.end(); it++) {
		//            GX_DELETE(*it);
		//        }
		//        records.clear();
	}
	if (fp == nullptr) {
		return;
	}
	int result = fclose(fp);
	if (result != 0) {
		debug_print_error(__LOGTAG__, "failed to close log file - %s, ret_val %d", logfile_path.c_str(), result);
	}
}

bool qlogfile::file_exists(const qstring& filename) {
	return access(filename.c_str(), F_OK) == 0;
}

void qlogfile::get_all_log_files(fs::path& path, std::vector<fs::path>& files, bool print_error_logs) {
	essentials::get_all_files(path.parent_path(), files, ".log");

#if 0
	unsigned int next_minor_counter_ = 0;
	unsigned int next_major_version_ = 0;
	fs::path current_logfile_path_;
	bool file_exist_ = false;
	unsigned int file_not_exist_counter = 0;

	do {
		current_logfile_path_ = path;
		qstring file_version_string;
		file_version_string.format("-%d-%d.log", next_major_version_, next_minor_counter_);
		current_logfile_path_ += file_version_string.c_str();
		next_minor_counter_++;
		if (next_minor_counter_ >= MINOR_VERSION_RESET_AT) {
			next_major_version_++;
			next_minor_counter_ = 0;
			if (next_major_version_ > MAJOR_VERSION_ALARM_AT) {
                if (print_error_logs) {
                    if (files.size() > 50) {
                        debug_print_error(__LOGTAG__, "----- CLEAN UP LOG FOLDER ----- %s", path.native().c_str());
                    } else if (files.size() == 0) {
                        debug_print_important(__LOGTAG__, "----- NO LOG FILE FOUND ----- %s", path.native().c_str());
                    }
                }
				break;
			}
		}
		file_exist_ = file_exists(current_logfile_path_.native().c_str());
		if (file_exist_) {
			files.push_back(current_logfile_path_);
		} else {
			file_not_exist_counter++;
		}
	} while (file_not_exist_counter < INT_MAX - 1);
#endif
}

int qlogfile::finalise_logfile() {
	if (!fp) {
		return 1;
	}

	qstring finalized_path = current_logfile_path;
	finalized_path.replace(".tmp", ".log");
	int result = rename(current_logfile_path.c_str(), finalized_path.c_str());

	// Print the result
	if (!result) {
		debug_print_important(__LOGTAG__, "log file renamed to --> %s", finalized_path.c_str());
	} else {
		debug_print_error(__LOGTAG__, "Couldn't rename logfile '%s'", current_logfile_path.c_str());
	}

	if (fp) {
		fclose(fp);
		fp = nullptr;
	}

	return result;
}

int qlogfile::create_new_logfile(bool finalize_prev_file) {
	if (finalize_prev_file) {
		finalise_logfile();	 // rename the file after closing, thus finalize it for processing.
	} else {
		if (fp) {  // just close the file.
			fclose(fp);
			fp = nullptr;
		}
	}

	qstring finalized_logfile_path;
	do {
		finalized_logfile_path.clear();
		current_logfile_path.clear();
		current_logfile_path.format("%s-%d-%d.tmp", logfile_path.c_str(), next_major_version, next_minor_counter);
		finalized_logfile_path.format("%s-%d-%d.log", logfile_path.c_str(), next_major_version, next_minor_counter);
		next_minor_counter++;
		if (next_minor_counter >= MINOR_VERSION_RESET_AT) {
			next_major_version++;
			next_minor_counter = 0;
			if (next_major_version > MAJOR_VERSION_ALARM_AT) {
				debug_print_error(__LOGTAG__, "----- CLEAN UP LOG FOLDER -----", logfile_path.c_str());
				debug_print_error(__LOGTAG__, "----- CLEAN UP LOG FOLDER -----", logfile_path.c_str());
				debug_print_error(__LOGTAG__, "----- CLEAN UP LOG FOLDER -----", logfile_path.c_str());
			}
		}

		if (file_exists(current_logfile_path)) {
			// check if this file is above threshold
			struct stat st;
			stat(current_logfile_path.c_str(), &st);
			if ((uint64_t) st.st_size < max_size_of_file) {
				break;	// open this file to append
			}
			//
		}
	} while (file_exists(current_logfile_path) || file_exists(finalized_logfile_path));

	fp = fopen(current_logfile_path.c_str(), "a");
	if (fp == nullptr) {
		debug_print_error(__LOGTAG__, "failed to open log file to append - %s - %d", current_logfile_path.c_str(), errno);
		return -1;
	} else {
		debug_print_important(__LOGTAG__, "log file opened for append - %s", current_logfile_path.c_str());
	}

	struct stat st;
	stat(current_logfile_path.c_str(), &st);
	logfile_size = st.st_size;
	return 0;
}

qbuffer* qlogfile::create_new_record(uint64_t buffer_size, std::vector<qbuffer*>& list) {
	if (fp == nullptr) {
		return nullptr;
	}
	qbuffer* record = DEBUG_NEW qbuffer();
	record->allocate(buffer_size);
	list.push_back(record);
	return record;
}

uint64_t qlogfile::log(qlogfile::log_lvls lvl, const char* tag, const char* buffer, uint64_t buffer_length) {
	UNUSED(lvl);
	bool locked = (log_mutex.try_lock(__FUNCTION__) == 0);
	if (!locked) {
		debug_warn(LOG_LEVEL_3, __LOGTAG__, "logging failed - '%s' !!!", buffer);
	} else {
		// move pending records if any
		for (std::vector<qbuffer*>::iterator it = records_waiting.begin(); it != records_waiting.end(); it++) {
			records.push_back(*it);
		}
		debug_warn_cond(__LOGTAG__, records_waiting.size() > 0, "pending logs pushed - '%d' !!!", records_waiting.size());
		records_waiting.clear();
	}
	if (fp == nullptr) {
		if (locked) {
			log_mutex.unlock();
		}
		return -1;
	}
	time_t givemetime = time(NULL);
	const char* ctime_str = ctime(&givemetime);
	const char* ctime_str_mod = strtok(ctime(&givemetime), "\n");
	ssize_t ctime_str_len = strlen(ctime_str);
	ssize_t tag_length = strlen(tag);
	ssize_t spaces = 10;  // 9 space + 1 extra space added !
	uint64_t total_record_sz = ctime_str_len + spaces + tag_length + buffer_length;
	qbuffer* record = create_new_record(ctime_str_len + spaces + tag_length + buffer_length, locked ? records : records_waiting);
	snprintf((char*) record->data, total_record_sz, "%s : [%s] - %s\n", ctime_str_mod, tag, buffer);
	record->index = total_record_sz;
	if (locked) {
		log_mutex.unlock();
	}
	return total_record_sz;
}

uint64_t qlogfile::log_buffer(const char* buffer, uint64_t buffer_length) {
	bool locked = (log_mutex.try_lock(__FUNCTION__) == 0);
	if (!locked) {
		debug_warn(LOG_LEVEL_4, __LOGTAG__, "logging failed (buffer) - '%s' !!!", buffer);
	} else {
		// move pending records if any
		for (std::vector<qbuffer*>::iterator it = records_waiting.begin(); it != records_waiting.end(); it++) {
			records.push_back(*it);
		}
		debug_warn_cond(__LOGTAG__, records_waiting.size() > 0, "pending logs(buffer) pushed - '%d' !!!", records_waiting.size());
		records_waiting.clear();
	}
	if (fp == nullptr) {
		if (locked) {
			log_mutex.unlock();
		}
		return -1;
	}
	uint64_t total_record_sz = buffer_length + 2;											   //+2 for \n
	qbuffer* record = create_new_record(total_record_sz, locked ? records : records_waiting);  //+1 for \n
	snprintf((char*) record->data, total_record_sz, "%s\n", buffer);
	record->index = total_record_sz;
	if (locked) {
		log_mutex.unlock();
	}
	return total_record_sz;
}

int qlogfile::flush(bool check_for_log_file_size) {
	if (log_mutex.try_lock(__FUNCTION__) != 0) {
		debug_warn(LOG_LEVEL_3, __LOGTAG__, "flush failed - '%d' !!!", check_for_log_file_size);
		return -2;
	}
	if (fp == nullptr) {
		log_mutex.unlock();
		return -1;
	}

	int err_cnt = 0;
	for (auto it = records.cbegin(); it != records.cend(); it++) {
		qbuffer* record = *it;
		ssize_t bytes_written = fprintf(fp, "%.*s", (int) record->index, record->data);
		if (bytes_written < 0) {
			debug_print_error(__LOGTAG__, "error while flushing - record - '%s' !!!", record->data);
			err_cnt++;
		} else {
			logfile_size += bytes_written;
		}
		GX_DELETE(record);
	}

	if (err_cnt) {
		debug_print_error(__LOGTAG__, "error while logging - lines not written %d !!!", err_cnt);
	}
	ssize_t flushed_cnt = records.size();
	if (flushed_cnt) {
		fflush(fp);
	}
	records.clear();
	if (check_for_log_file_size && logfile_size > max_size_of_file) {
		create_new_logfile(true);
	}
	log_mutex.unlock();
	return (int) flushed_cnt;
}

qtextfilelogger::qtextfilelogger() : qtimer_uv_scheduler() {}

qtextfilelogger::~qtextfilelogger() {
	//	if (log_loop) {
	//		if (!essentials::cleanup_and_destroy_uv_loop(log_loop)) {
	//			debug_print_error(__LOGTAG__, "Failed to delete log_loop !!!");
	//		} else {
	//			log_loop = nullptr;
	//		}
	//	}
}

int qtextfilelogger::start_session(const qstring& path, float flush_time, uint64_t max_size_of_file) {
	if (!config.finished) {
		debug_print_error(__LOGTAG__, "file logger already running !!!");
		return -1;
	}
	config.logfile_path = path;
	config.flush_time = flush_time;
	config.max_size_of_file = max_size_of_file;
	config.logger = this;
	if (pthread_create(&log_thread_id, nullptr, qtextfilelogger::run_log_session, (void*) &config) < 0) {
		debug_print_error(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
		return -1;
	}
	return 0;
}

void* qtextfilelogger::run_log_session(void* data) {
	qlog_config* config = (qlog_config*) data;
	qtextfilelogger* logger = config->logger;

	PTHREAD_NAME("qtextfilelogger");
	if (logger->log_session_mutex.try_lock(__FUNCTION__) != 0) {
		config->finished = true;
		config->pthread_return_value = -1;
		pthread_exit(&config->pthread_return_value);
	}
	GX_DELETE(logger->logfile);
	logger->logfile = DEBUG_NEW qlogfile(config->logfile_path, config->max_size_of_file);

	config->finished = false;
	logger->log_loop = uv_loop_new();
	uv_timeval64_t creation_time;
	uv_gettimeofday(&creation_time);  // Get current time
	logger->set_uv_loop(logger->log_loop);
	logger->logtimer = logger->schedule_repeat_timer(
		[logger, creation_time](qtimer_uv& timer) {
			UNUSED(timer);
			uv_timeval64_t now;
			uv_gettimeofday(&now);
			// Calculate elapsed time
			double elapsed_time = (now.tv_sec - creation_time.tv_sec) + (now.tv_usec - creation_time.tv_usec) / 1e6;
			if (logger->logfile->flush(true) > 0) {
				debug_print(LOG_LEVEL_5, __LOGTAG__, "flush - t:%10.2fs", elapsed_time);
			}
		},
		config->flush_time);
	logger->log(qlogfile::LEVEL_0, __LOGTAG__, "start-session");
	uv_run(logger->log_loop, UV_RUN_DEFAULT);

	logger->logtimer = nullptr;	 // scheduler will delete the timer;
	config->finished = true;
	GX_DELETE(logger->logfile);

	if (logger->log_loop) {
		if (!essentials::cleanup_and_destroy_uv_loop(logger->log_loop)) {
			debug_print_error(__LOGTAG__, "Failed to delete log_loop !!!");
		} else {
			logger->log_loop = nullptr;
		}
	}

	debug_print(LOG_LEVEL_0, __LOGTAG__, "file logger exiting ...");
	if (logger->log_session_mutex.unlock(__FUNCTION__) != 0) {
		config->pthread_return_value = -1;
		pthread_exit(&config->pthread_return_value);
	}

	pthread_exit(0);
}

uint64_t qtextfilelogger::log(qlogfile::log_lvls lvl, const char* tag, const char* format, ...) {
	if (logfile == nullptr) {
		debug_print_error(__LOGTAG__, "log file not created yet !!!");
		return -1;
	}
	memset(log_record_buffer, 0, SINGLE_LOG_RECORD_LENGTH);
	va_list v;
	va_start(v, format);
	vsnprintf(log_record_buffer, SINGLE_LOG_RECORD_LENGTH, format, v);
	va_end(v);
	ssize_t len = strlen(log_record_buffer);
	return logfile->log(lvl, tag, log_record_buffer, len);
}

int qtextfilelogger::end_session() {
	if (logtimer == nullptr) {
		return -1;
	}
	async_cancel_and_destroy_timer(logtimer, [&](bool timer_deleted) {
		if (timer_deleted) {
			logtimer = nullptr;
		}
	});
	return 0;
}
