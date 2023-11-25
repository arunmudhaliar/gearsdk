//
//  qpgsql.hpp
//  servercommon
//
//  Created by Arun A on 19/11/23.
//

#ifndef qpgsql_hpp
#define qpgsql_hpp

#include "../../common/qstring.h"
extern "C" {
#include "./libpq-fe.h"
}

class qpgsql {
public:
    qpgsql();
    ~qpgsql();
    
    int connect_db();
    void close_db();
    int execute_query(const qstring& query);
    
private:
    PGconn* db_connection = nullptr;
};
#endif /* qpgsql_hpp */
