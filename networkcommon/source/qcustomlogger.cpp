//
//  qcustomlogger.cpp
//  networkcommon
//
//  Created by Arun A on 22/10/24.
//

#include "qcustomlogger.hpp"

qcustomlogger::qcustomlogger() : qtextfilelogger() {}

qcustomlogger::~qcustomlogger() {}

uint64_t qcustomlogger::log(qlogfile::log_lvls lvl, elog_type type, const char* tag, const char* pid, const char* roomid, const char* format, ...) {
	//	return qtextfilelogger::log(lvl, tag, <#const char *format, ...#>);
	if (logfile == nullptr) {
		debug_print_error(__LOGTAG__, "log file not created yet !!!");
		return -1;
	}
	//    qstring seperator("|");
	//    qstring buffer("open");
	//    qstring client_utc_tstamp = essentials::get_time_utc_postgresql_format();
	time_t utc_time_value;
	qstring utc_time = essentials::get_time_utc_string(utc_time_value);
	//    buffer += seperator;

	memset(formatted_message_buffer, 0, SINGLE_MSG_LENGTH);
	va_list v;
	va_start(v, format);
	vsnprintf(formatted_message_buffer, SINGLE_MSG_LENGTH, format, v);
	va_end(v);
	//	ssize_t formatted_msg_len = strlen(formatted_message_buffer);

	// Construct the final log message: "elog_type|tag|formatted_message"
	memset(log_record_buffer, 0, SINGLE_LOG_RECORD_LENGTH);
	snprintf(log_record_buffer, SINGLE_LOG_RECORD_LENGTH, "%s|%s|%s|%s|%s|%s", utc_time.c_str(), elog_type_string[type], tag, pid == nullptr ? "" : pid, roomid == nullptr ? "" : roomid, formatted_message_buffer);

	return logfile->log_buffer(log_record_buffer, strlen(log_record_buffer));
}
