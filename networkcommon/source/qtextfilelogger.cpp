//
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

qlogfile::qlogfile(const qstring& path, float flush_time, size_t max_size_of_file) :
    logfile_path(path), flush_time(flush_time), max_size_of_file(max_size_of_file) {
    fs::path log_dir = fs::path(logfile_path.c_str()).parent_path();
    if (!fs::is_directory(log_dir)) {
        fs::create_directory(log_dir);
    }
    create_new_logfile(false);
}

qlogfile::~qlogfile() {
    flush(false);
    if (fp == nullptr) {
        return;
    }
    int result = fclose(fp);
    if (result != 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to close log file - %s, ret_val %d", logfile_path.c_str(), result);
    }
}

bool qlogfile::file_exists(const qstring& filename) {
    return access(filename.c_str(), F_OK) == 0;
}

void qlogfile::get_all_log_files(fs::path& path, std::vector<fs::path>& files) {
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
                if (files.size() > 50) {
                    DEBUG_PRINT_ERROR(__LOGTAG__, "----- CLEAN UP LOG FOLDER -----", path.native().c_str());
                } else if (files.size() == 0) {
                    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "----- NO LOG FILE FOUND -----", path.native().c_str());
                }
                break;
            }
        }
        file_exist_ = file_exists(current_logfile_path_.native().c_str());
        if (file_exist_) {
            files.push_back(current_logfile_path_);
        }
        else {
            file_not_exist_counter++;
        }
    } while (file_not_exist_counter < INT_MAX-1);
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
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "log file renamed successfully");
    }
    else {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Couldn't rename logfile");
    }

    if (fp) {
        fclose(fp);
        fp = nullptr;
    }

    return result;
}

int qlogfile::create_new_logfile(bool finalize_prev_file) {
    if (finalize_prev_file) {
        finalise_logfile(); // rename the file after closing, thus finalize it for processing.
    }
    else {
        if (fp) {           // just close the file.
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
                DEBUG_PRINT_ERROR(__LOGTAG__, "----- CLEAN UP LOG FOLDER -----", logfile_path.c_str());
                DEBUG_PRINT_ERROR(__LOGTAG__, "----- CLEAN UP LOG FOLDER -----", logfile_path.c_str());
                DEBUG_PRINT_ERROR(__LOGTAG__, "----- CLEAN UP LOG FOLDER -----", logfile_path.c_str());
            }
        }

        if (file_exists(current_logfile_path)) {
            // check if this file is above threshold
            struct stat st;
            stat(current_logfile_path.c_str(), &st);
            if (st.st_size < max_size_of_file) {
                break;  // open this file to append
            }
            //
        }
    } while (file_exists(current_logfile_path) || file_exists(finalized_logfile_path));

    fp = fopen(current_logfile_path.c_str(), "a");
    if (fp == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to open log file to append - %s - %d", current_logfile_path.c_str(), errno);
        return -1;
    }
    else {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "log file opend for append - %s", current_logfile_path.c_str());
    }

    struct stat st;
    stat(current_logfile_path.c_str(), &st);
    logfile_size = st.st_size;
    return 0;
}

qbuffer* qlogfile::create_new_record(size_t buffer_size) {
    if (fp == nullptr) {
        return nullptr;
    }
    qbuffer* record = DEBUG_NEW qbuffer();
    record->allocate(buffer_size);
    records.push_back(record);
    return record;
}

size_t qlogfile::log(qlogfile::log_lvls lvl, const char* tag, const char* buffer, size_t buffer_length) {
    log_mutex.tryLock(__FUNCTION__);
    if (fp == nullptr) {
        return -1;
    }
    time_t givemetime = time(NULL);
    const char* ctime_str = ctime(&givemetime);
    const char* ctime_str_mod = strtok(ctime(&givemetime), "\n");
    ssize_t ctime_str_len = strlen(ctime_str);
    ssize_t tag_length = strlen(tag);
    ssize_t spaces = 10;    //9 space + 1 extra space added !
    size_t total_record_sz = ctime_str_len + spaces + tag_length + buffer_length;
    qbuffer* record = create_new_record(ctime_str_len + spaces + tag_length + buffer_length);
    snprintf((char*)record->data, total_record_sz, "%s : [%s] - %s\n", ctime_str_mod, tag, buffer);
    record->index = total_record_sz;
    log_mutex.unLock();
    return total_record_sz;
}

size_t qlogfile::log_buffer(const char* buffer, size_t buffer_length) {
    log_mutex.tryLock(__FUNCTION__);
    if (fp == nullptr) {
        return -1;
    }
    size_t total_record_sz = buffer_length + 2;   //+2 for \n
    qbuffer* record = create_new_record(total_record_sz);   //+1 for \n
    snprintf((char*)record->data, total_record_sz, "%s\n", buffer);
    record->index = total_record_sz;
    log_mutex.unLock();
    return total_record_sz;
}

int qlogfile::flush(bool check_for_log_file_size) {
    log_mutex.tryLock(__FUNCTION__);
    if (fp == nullptr) {
        return -1;
    }

    int err_cnt = 0;
    for (auto it = records.cbegin();it != records.cend();it++) {
        qbuffer* record = *it;
        ssize_t bytes_written = fprintf(fp, "%.*s", (int)record->index, record->data);
        if (bytes_written < 0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "error while flushing - record - '%s' !!!", record->data);
            err_cnt++;
        }
        else {
            logfile_size += bytes_written;
        }
        GX_DELETE(record);
    }

    if (err_cnt) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "error while logging - lines not written %d !!!", err_cnt);
    }
    ssize_t flushed_cnt = records.size();
    if (flushed_cnt) {
        fflush(fp);
    }
    records.clear();
    if (check_for_log_file_size && logfile_size > max_size_of_file) {
        create_new_logfile(true);
    }
    log_mutex.unLock();
    return (int)flushed_cnt;
}

qtextfilelogger::qtextfilelogger() :
    qtimer_sceduler() {
}

qtextfilelogger::~qtextfilelogger() {
}

int qtextfilelogger::start_session(const qstring& path, float flush_time, size_t max_size_of_file) {
    if (!config.finished) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "file logger already running !!!");
        return -1;
    }
    config.logfile_path = path;
    config.flush_time = flush_time;
    config.max_size_of_file = max_size_of_file;
    config.logger = this;
    if (pthread_create(&log_thread_id, nullptr, qtextfilelogger::run_log_session, (void*)&config) < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
        return -1;
    }
    return 0;
}

void* qtextfilelogger::run_log_session(void* data) {
    qlog_config* config = (qlog_config*)data;
    qtextfilelogger* logger = config->logger;

    if (logger->log_session_mutex.tryLock(__FUNCTION__) != 0) {
        config->finished = true;
        config->pthread_returnValue = -1;
        pthread_exit(&config->pthread_returnValue);
    }
    GX_DELETE(logger->logfile);
    logger->logfile = DEBUG_NEW qlogfile(config->logfile_path, config->flush_time, config->max_size_of_file);

    config->finished = false;
    logger->log_loop = ev_loop_new(0);
    ev_tstamp creation_time = ev_now(logger->log_loop);
    logger->set_ev_lopp(logger->log_loop);
    logger->logtimer = logger->schedule_repeat_timer([logger, creation_time](qtimer& timer) {
        if (logger->logfile->flush(true) > 0) {
            DEBUG_PRINT(LOG_LEVEL_4, __LOGTAG__, "flush - t:%10.2fs", ev_now(logger->log_loop) - creation_time);
        }
        }, config->flush_time);
    logger->log(qlogfile::level_0, __LOGTAG__, "start-session");
    ev_run(logger->log_loop, 0);

    logger->logtimer = nullptr; // scheduler will delete the timer;
    config->finished = true;
    GX_DELETE(logger->logfile);
    DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "file logger exiting ...");
    if (logger->log_session_mutex.unLock(__FUNCTION__) != 0) {
        config->pthread_returnValue = -1;
        pthread_exit(&config->pthread_returnValue);
    }

    pthread_exit(0);
}

size_t qtextfilelogger::log(qlogfile::log_lvls lvl, const char* tag, const char* format, ...) {
    if (logfile == nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "log file not created yet !!!");
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
    cancel_and_destroy_timer(logtimer);
    logtimer = nullptr;
    return 0;
}
