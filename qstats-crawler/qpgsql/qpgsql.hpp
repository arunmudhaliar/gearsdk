//
//  Copyright 2024 homenet25
//  qpgsql.hpp
//  servercommon
//
//  Created by Arun A on 19/11/23.
//

#ifndef qpgsql_hpp
#define qpgsql_hpp

#include "../../common/qstring.hpp"
extern "C" {
#include "./libpq-fe.h"
}


#undef __LOGTAG__
#define __LOGTAG__ "qpgsql"

class qpgsql {
public:
    qpgsql();
    ~qpgsql();

    int connect_db(const qstring& host="127.0.0.1", const qstring& port="5432");
    void close_db();
    int execute_query(const qstring& query);

private:
    PGconn* db_connection = nullptr;
    qstring host = "127.0.0.1";
    qstring port = "5432";
};
#endif /* qpgsql_hpp */
