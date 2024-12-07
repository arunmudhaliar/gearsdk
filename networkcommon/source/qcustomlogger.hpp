//
//  qcustomlogger.hpp
//  networkcommon
//
//  Created by Arun A on 22/10/24.
//

#ifndef qcustomlogger_hpp
#define qcustomlogger_hpp

#include "qtextfilelogger.hpp"

#define LOG_FILE(logger, log_type, tag, ...) LOG_FILE_WITH_ROOMID(logger, log_type, tag, nullptr, nullptr, __VA_ARGS__)
#define LOG_FILE_WITH_PID(logger, log_type, tag, pid, ...) LOG_FILE_WITH_ROOMID(logger, log_type, tag, pid, nullptr, __VA_ARGS__)
#define LOG_FILE_WITH_ROOMID(logger, log_type, tag, pid, roomid, ...) logger->log(qlogfile::LEVEL_0, log_type, tag, pid, roomid, __VA_ARGS__)

#define SINGLE_MSG_LENGTH SINGLE_LOG_RECORD_LENGTH - 64

class qcustomlogger : public qtextfilelogger {
   public:
	enum elog_type { INFO_LOG, DEBUG_LOG, WARN_LOG, ERROR_LOG, LOG_TYPE_MAX };
	const char* elog_type_string[LOG_TYPE_MAX] = {"INFO", "DEBUG", "WARN", "ERROR"};
	qcustomlogger();
	virtual ~qcustomlogger();

	uint64_t log(qlogfile::log_lvls lvl, elog_type type, const char* tag, const char* pid, const char* roomid, const char* format, ...);

	char formatted_message_buffer[SINGLE_MSG_LENGTH + 1];
};

#endif /* qcustomlogger_hpp */
