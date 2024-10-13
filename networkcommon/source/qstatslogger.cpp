//
//  Copyright 2024 homenet25
//  qstatslogger.cpp
//  networkcommon
//
//  Created by Arun A on 20/11/23.
//

#include "qstatslogger.hpp"

qstatslogger::qstatslogger() : qtextfilelogger() {
	inited = false;
}

qstatslogger::~qstatslogger() {}

void qstatslogger::init(const qstring& install_os, const qstring& device_name, const qstring& device_model, const qstring& app_id, const int TOTAL_RAM) {
	this->install_os = install_os;
	this->device_name = device_name;
	this->device_model = device_model;
	this->app_id = app_id;
	this->total_ram = TOTAL_RAM;
	this->inited = true;
}

size_t qstatslogger::log_stats(const qstring& buffer) {
	if (logfile == nullptr) {
		debug_print_error(__LOGTAG__, "stats file not created yet !!!");
		return -1;
	}
	if (inited == false) {
		debug_print_error(__LOGTAG__, "stats not inited yet !!!");
	}
	return logfile->log_buffer(buffer.c_str(), buffer.length());
}

void qstatslogger::set_client_session(const qstring& session_str) {
	client_session = session_str;
}

void qstatslogger::set_client_version(const qstring& version_str) {
	client_version = version_str;
}

void qstatslogger::set_client_pid(const qstring& pid_str) {
	client_pid = pid_str;
}

void qstatslogger::set_total_ram(int ram) {
	total_ram = ram;
}

/*
 CREATE TABLE stats_count (count TEXT, session TEXT, pid TEXT, version TEXT,
					 epic TEXT, myth TEXT, legend TEXT, story TEXT,
					 install_os TEXT, server_tstamp timestamp, client_tstamp timestamp, time bigint, message TEXT,
					 device_name TEXT, device_model TEXT, total_ram INT4);

 CREATE TABLE stats_open (version TEXT, duid TEXT,
					 epic TEXT, myth TEXT, legend TEXT, story TEXT,
					 install_os TEXT, client_tstamp timestamp, time bigint,
					 device_name TEXT, device_model TEXT, total_ram INT4);
 */

size_t qstatslogger::client_open(const qstring& duid, const qstring& epic, const qstring& myth, const qstring& legend, const qstring& story) {
	qstring seperator("|");
	qstring buffer("open");
	qstring client_utc_tstamp = essentials::get_time_utc_postgresql_format();
	time_t utc_time_value;
	qstring utc_time = essentials::get_time_utc_string(utc_time_value);
	buffer += seperator;
	buffer += client_version;
	buffer += seperator;
	buffer += duid;
	buffer += seperator;
	buffer += epic;
	buffer += seperator;
	buffer += myth;
	buffer += seperator;
	buffer += legend;
	buffer += seperator;
	buffer += story;
	buffer += seperator;
	buffer += install_os;
	buffer += seperator;
	buffer += client_utc_tstamp;
	buffer += seperator;  // client_tstamp
	buffer += utc_time;
	buffer += seperator;  // utc_time
	buffer += device_name;
	buffer += seperator;
	buffer += device_model;
	buffer += seperator;
	buffer += qstring(total_ram);
	buffer += seperator;
	buffer += app_id;
	return log_stats(buffer);
}

size_t qstatslogger::server_count(const qstring& counter, long count_val, const qstring& session, const qstring& pid, const qstring& version, const qstring& epic, const qstring& myth, const qstring& legend, const qstring& story,
								  const qstring& message) {
	qstring server_utc_tstamp = essentials::get_time_utc_postgresql_format();
	return server_count_internal(counter, count_val, session, pid, version, epic, myth, legend, story, server_utc_tstamp, message);
}

size_t qstatslogger::server_count_internal(const qstring& counter, long count_val, const qstring& session, const qstring& pid, const qstring& version, const qstring& epic, const qstring& myth, const qstring& legend, const qstring& story,
										   const qstring& server_tstamp, const qstring& message) {
	time_t utc_time_value;
	qstring utc_time = essentials::get_time_utc_string(utc_time_value);
	qstring seperator("|");
	qstring buffer("count");
	buffer += seperator;
	buffer += counter;
	buffer += seperator;
	buffer += count_val;
	buffer += seperator;
	buffer += session;
	buffer += seperator;
	buffer += pid;
	buffer += seperator;
	buffer += client_version;
	buffer += seperator;
	buffer += epic;
	buffer += seperator;
	buffer += myth;
	buffer += seperator;
	buffer += legend;
	buffer += seperator;
	buffer += story;
	buffer += seperator;
	buffer += install_os;
	buffer += seperator;
	buffer += server_tstamp;
	buffer += seperator;  // server_tstamp
	buffer += "NULL";
	buffer += seperator;  // client_tstamp
	buffer += utc_time;
	buffer += seperator;  // utc_time
	buffer += message;
	buffer += seperator;
	buffer += device_name;
	buffer += seperator;
	buffer += device_model;
	buffer += seperator;
	buffer += qstring(total_ram);
	buffer += seperator;
	buffer += app_id;
	return log_stats(buffer);
}

size_t qstatslogger::client_count(const qstring& counter, long count_val, const qstring& epic, const qstring& myth, const qstring& legend, const qstring& story, const qstring& message) {
	qstring client_utc_tstamp = essentials::get_time_utc_postgresql_format();
	time_t utc_time_value;
	qstring utc_time = essentials::get_time_utc_string(utc_time_value);
	qstring seperator("|");
	qstring buffer("count");
	buffer += seperator;
	buffer += counter;
	buffer += seperator;
	buffer += count_val;
	buffer += seperator;
	buffer += client_session;
	buffer += seperator;
	buffer += client_pid;
	buffer += seperator;
	buffer += client_version;
	buffer += seperator;
	buffer += epic;
	buffer += seperator;
	buffer += myth;
	buffer += seperator;
	buffer += legend;
	buffer += seperator;
	buffer += story;
	buffer += seperator;
	buffer += install_os;
	buffer += seperator;
	buffer += "NULL";
	buffer += seperator;  // server_tstamp
	buffer += client_utc_tstamp;
	buffer += seperator;  // client_tstamp
	buffer += utc_time;
	buffer += seperator;  // utc_time
	buffer += message;
	buffer += seperator;
	buffer += device_name;
	buffer += seperator;
	buffer += device_model;
	buffer += seperator;
	buffer += qstring(total_ram);
	buffer += seperator;
	buffer += app_id;
	return log_stats(buffer);
}
