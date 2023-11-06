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

qlogfile::qlogfile(const char* path, size_t path_len, float flush_time, size_t max_size_of_file) :
    logfile_path_len(path_len), flush_time(flush_time), max_size_of_file(max_size_of_file)
{
    logfile_path = new char[logfile_path_len];
    memcpy(logfile_path, path, logfile_path_len);
    create_new_logfile();
}

qlogfile::~qlogfile() {
    flush(false);
    GX_DELETE_ARY(logfile_path);
    GX_DELETE_ARY(current_logfile_path);
    if (fp==nullptr) {
        return;
    }
    int result = fclose(fp);
    if (result!=0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to close log file - %s, ret_val %d", logfile_path, result);
    }
}

bool file_exists(const char *filename)
{
    return access(filename, F_OK) == 0;
}

int qlogfile::create_new_logfile() {
    if (fp) {
        fclose(fp);
        fp = nullptr;
    }
    
    do {
        unsigned long size_ = logfile_path_len + NumberOfDigits(next_minor_counter)+6+NumberOfDigits(next_major_version);
        GX_DELETE_ARY(current_logfile_path);
        current_logfile_path = new char[size_];
        snprintf(current_logfile_path, size_, "%s-%d-%d.log", logfile_path, next_major_version, next_minor_counter);
        next_minor_counter++;
        if (next_minor_counter>=MINOR_VERSION_RESET_AT) {
            next_major_version++;
            next_minor_counter = 0;
            if (next_major_version>MAJOR_VERSION_ALARM_AT) {
                DEBUG_PRINT_ERROR(__LOGTAG__, "----- CLEAN UP LOG FOLDER -----", logfile_path);
                DEBUG_PRINT_ERROR(__LOGTAG__, "----- CLEAN UP LOG FOLDER -----", logfile_path);
                DEBUG_PRINT_ERROR(__LOGTAG__, "----- CLEAN UP LOG FOLDER -----", logfile_path);
            }
        }
    } while(file_exists(current_logfile_path));

    fp = fopen(current_logfile_path, "a");
    if (fp==nullptr) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to open log file to append - %s", current_logfile_path);
        return -1;
    } else {
        DEBUG_PRINT_IMPORTANT(__LOGTAG__, "log file created - %s", current_logfile_path);
    }
    logfile_size = 0;
    return 0;
}

qbuffer* qlogfile::create_new_record(size_t buffer_size) {
    if (fp==nullptr) {
        return nullptr;
    }
    qbuffer* record = new qbuffer();
    record->allocate(buffer_size);
    records.push_back(record);
    return record;
}

size_t qlogfile::log(qlogfile::log_lvls lvl, const char* tag, const char* buffer, size_t buffer_length) {
    log_mutex.tryLock(__FUNCTION__);
    if (fp==nullptr) {
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

int qlogfile::flush(bool check_for_log_file_size) {
    log_mutex.tryLock(__FUNCTION__);
    if (fp==nullptr) {
        return -1;
    }

    int err_cnt = 0;
    for(auto it = records.cbegin();it!=records.cend();it++) {
        qbuffer* record = *it;
        ssize_t bytes_written = fprintf(fp, "%*.*s", (int)record->index, (int)record->index, record->data);
        if (bytes_written<0) {
            DEBUG_PRINT_ERROR(__LOGTAG__, "error while flushing - record - '%s' !!!", record->data);
            err_cnt++;
        } else {
            logfile_size+=bytes_written;
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
    if (check_for_log_file_size && logfile_size>max_size_of_file) {
        create_new_logfile();
    }
    log_mutex.unLock();
    return (int)flushed_cnt;
}

qtextfilelogger::qtextfilelogger() :
    qtimer_sceduler() {
    
}

qtextfilelogger::~qtextfilelogger() {
}

int qtextfilelogger::start_session(const char* path, size_t path_len, float flush_time, size_t max_size_of_file) {
    if (!config.finished) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "file logger already running !!!");
        return -1;
    }
    config.logfile_path = const_cast<char*>(path);
    config.logfile_path_len = path_len;
    config.flush_time = flush_time;
    config.max_size_of_file = max_size_of_file;
    config.logger = this;
    if (pthread_create(&log_thread_id, nullptr, qtextfilelogger::run_log_session, (void *)&config) < 0)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror(errno), errno);
        return -1;
    }
    return 0;
}

void *qtextfilelogger::run_log_session(void *data) {
    qlog_config* config = (qlog_config*)data;
    qtextfilelogger* logger = config->logger;
    
    if (logger->log_session_mutex.tryLock(__FUNCTION__) != 0)
    {
        config->finished = true;
        config->pthread_returnValue = -1;
        pthread_exit(&config->pthread_returnValue);
    }
    GX_DELETE(logger->logfile);
    logger->logfile = new qlogfile(config->logfile_path, config->logfile_path_len, config->flush_time, config->max_size_of_file );
    
    config->finished = false;
    logger->log_loop = ev_loop_new(0);
    ev_tstamp creation_time = ev_now(logger->log_loop);
    logger->set_ev_lopp(logger->log_loop);
    logger->logtimer = logger->schedule_repeat_timer([logger, creation_time](qtimer& timer) {
        if (logger->logfile->flush(true) > 0) {
            DEBUG_PRINT(LOG_LEVEL_0, __LOGTAG__, "flush - t:%10.2fs", ev_now(logger->log_loop) - creation_time );
        }
    }, config->flush_time);
    logger->log(qlogfile::level_0, __LOGTAG__, "start-session");
    ev_run(logger->log_loop, 0);
    
    logger->logtimer = nullptr; // scheduler will delete the timer;
    config->finished = true;
    GX_DELETE(logger->logfile);
    if (logger->log_session_mutex.unLock(__FUNCTION__) != 0)
    {
        config->pthread_returnValue = -1;
        pthread_exit(&config->pthread_returnValue);
    }
    
    pthread_exit(0);
}

size_t qtextfilelogger::log(qlogfile::log_lvls lvl, const char* tag, const char *format, ...) {
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
    cancel_timer(logtimer);
    return 0;
}
