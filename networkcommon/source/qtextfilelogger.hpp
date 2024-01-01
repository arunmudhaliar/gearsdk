//
//  qtextfilelogger.hpp
//  networkcommon
//
//  Created by Arun A on 01/11/23.
//

#ifndef qtextfilelogger_hpp
#define qtextfilelogger_hpp

#include "qtimer.hpp"
#include "qbuffer.hpp"
#include "essentials.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qtextfilelogger"

#define SINGLE_LOG_RECORD_LENGTH 512
#define MINOR_VERSION_RESET_AT 100
#define MAJOR_VERSION_ALARM_AT 10000
#define MAX_LOG_FILE_SIZE_IN_BYTES 1024 * 256

class qlogfile {
public:
    enum log_lvls {
        level_0,
        level_1,
        level_2,
        level_3,
        level_4
    };

    qlogfile(const qstring& path, float flush_time = 60.0f, size_t max_size_of_file = MAX_LOG_FILE_SIZE_IN_BYTES);
    ~qlogfile();
    size_t log(log_lvls lvl, const char* tag, const char* buffer, size_t buffer_length);
    size_t log_buffer(const char* buffer, size_t buffer_length);
    int flush(bool check_for_log_file_size);
    qbuffer* create_new_record(size_t buffer_size, std::vector<qbuffer*>& list);

    static bool file_exists(const qstring& filename);
    static void get_all_log_files(fs::path& path, std::vector<fs::path>& files);

private:
    int finalise_logfile();
    int create_new_logfile(bool finalize_prev_file);
    FILE* fp = nullptr;
    float flush_time = 60.0f;   // in 60 sec
    qstring logfile_path;
    const size_t logfile_path_len = 0;
    size_t max_size_of_file = MAX_LOG_FILE_SIZE_IN_BYTES;
    size_t logfile_size = 0;
    unsigned int next_minor_counter = 0;
    unsigned int next_major_version = 0;
    qstring current_logfile_path;
    std::vector<qbuffer*> records;
    std::vector<qbuffer*> records_waiting;
    qbuffer_writer buffer_writer;
    qmutex log_mutex;
};

class qtextfilelogger : public qtimer_sceduler {
public:
    qtextfilelogger();
    virtual ~qtextfilelogger();
    struct qlog_config {
        qlog_config() {}
        qlog_config(const qstring& path, float flush_time, qtextfilelogger* logger, size_t max_size_of_file) :
            logfile_path(path), flush_time(flush_time), max_size_of_file(max_size_of_file), logger(logger) {
        }
        qstring logfile_path;
        float flush_time = 60;
        size_t max_size_of_file = MAX_LOG_FILE_SIZE_IN_BYTES;
        qtextfilelogger* logger = nullptr;
        bool finished = true;
        int pthread_returnValue = 0;
    };
    int start_session(const qstring& path, float flush_time = 60.0f, size_t max_size_of_file = MAX_LOG_FILE_SIZE_IN_BYTES);
    size_t log(qlogfile::log_lvls lvl, const char* tag, const char* format, ...);
    int end_session();

    static void* run_log_session(void* data);

    qmutex log_session_mutex;
    pthread_t log_thread_id;
    qlog_config config;
    struct ev_loop* log_loop = nullptr;
    qlogfile* logfile = nullptr;
    qtimer* logtimer = nullptr;
    char log_record_buffer[SINGLE_LOG_RECORD_LENGTH + 1];
};
#endif /* qtextfilelogger_hpp */
