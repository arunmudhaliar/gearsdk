//
//  qpgsql.cpp
//  servercommon
//
//  Created by Arun A on 19/11/23.
//

#include "qpgsql.hpp"

qpgsql::qpgsql() {
}

qpgsql::~qpgsql() {
    close_db();
}

int qpgsql::connect_db(const qstring& host_, const qstring& port_) {
    if (db_connection) {
        return -1;
    }
    host=host_;
    port=port_;
    /*
     PGconn *PQsetdbLogin(const char *pghost,
                          const char *pgport,
                          const char *pgoptions,
                          const char *pgtty,
                          const char *dbName,
                          const char *login,
                          const char *pwd);
     */
    db_connection = PQsetdbLogin(host.c_str(), port.c_str(), nullptr, nullptr, "qtest-pgdb", "postgres", "postgres");
    /* Check to see that the backend connection was successfully made */
    if (PQstatus(db_connection) != CONNECTION_OK) {
        fprintf(stderr, "\033[41m%s\x1b[0m", PQerrorMessage(db_connection));
        close_db();
        return -1;
    }

    /* Set always-secure search path, so malicious users can't take control. */
    PGresult* res = PQexec(db_connection,
        "SELECT pg_catalog.set_config('search_path', '', false)");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "\033[41mSET failed: %s\x1b[0m", PQerrorMessage(db_connection));
        PQclear(res);
        close_db();
        return -1;
    }

    /*
     * Should PQclear PGresult whenever it is no longer needed to avoid memory
     * leaks
     */
    PQclear(res);

    return 0;
}

int qpgsql::execute_query(const qstring& query) {
    if (db_connection == NULL) {
        connect_db();
    }
    
    /* Set always-secure search path, so malicious users can't take control. */
    PGresult* res = PQexec(db_connection, query.c_str());
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "\033[41mCMD failed: %s\x1b[0m", PQerrorMessage(db_connection));
        PQclear(res);
        close_db();
        return -1;
    }
    return 0;
}

void qpgsql::close_db() {
    if (db_connection) {
        PQfinish(db_connection);
        db_connection = nullptr;
    }
}
