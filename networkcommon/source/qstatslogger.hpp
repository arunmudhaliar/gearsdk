//
//  Copyright 2024 homenet25
//  qstatslogger.hpp
//  networkcommon
//
//  Created by Arun A on 20/11/23.
//

#ifndef qstatslogger_hpp
#define qstatslogger_hpp

#include "../../common/qstring.hpp"
#include "qtextfilelogger.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qstatslogger"

class qstatslogger : public qtextfilelogger {
   public:
	qstatslogger();
	~qstatslogger();

	void init(const qstring& install_os, const qstring& device_name, const qstring& device_model, const qstring& app_id, const int TOTAL_RAM);

	size_t log_stats(const qstring& buffer);

	/*
	 CREATE TABLE qtest_pgdb_schema.stats_debug (session TEXT, pid TEXT, install_os TEXT,
						 server_tstamp timestamp, client_tstamp timestamp, message TEXT,
						 device_name TEXT, device_model TEXT, total_ram INT4);
	 */
	size_t client_open(const qstring& duid, const qstring& epic, const qstring& myth, const qstring& legend, const qstring& story);

	size_t server_count(const qstring& counter, long count_val, const qstring& session, const qstring& pid, const qstring& version = "", const qstring& epic = "", const qstring& myth = "", const qstring& legend = "",
						const qstring& story = "", const qstring& message = "");

	size_t server_count_internal(const qstring& counter, long count_val, const qstring& session, const qstring& pid, const qstring& version, const qstring& epic, const qstring& myth, const qstring& legend, const qstring& story,
								 const qstring& server_tstamp, const qstring& message);
	size_t client_count(const qstring& counter, long count_val, const qstring& epic = "", const qstring& myth = "", const qstring& legend = "", const qstring& story = "", const qstring& message = "");

	void set_client_session(const qstring& session_str);
	void set_client_version(const qstring& version_str);
	void set_client_pid(const qstring& pid_str);
	void set_total_ram(int ram);

   private:
	// only for client
	qstring client_session;
	qstring client_version;
	qstring client_pid;

	// for client and server
	qstring install_os;
	qstring device_name;
	qstring device_model;
	int total_ram = 0;
	qstring app_id;
	bool inited = false;
};

#endif /* qstatslogger_hpp */
