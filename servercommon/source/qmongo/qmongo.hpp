//
//  qmongo.hpp
//  servercommon
//
//  Created by Arun A on 03/11/23.
//

#ifndef qmongo_hpp
#define qmongo_hpp

#include <mongoc/mongoc.h>
#include <map>

class interface_qmongo_connection {
public:
    virtual void on_mongo_connect() = 0;
    virtual void on_mongo_create_index_keys(const char* collection_name, bson_t& indexkey, mongoc_index_opt_t& opt) = 0;
};

class qmongo {
public:
    qmongo(interface_qmongo_connection* interfce, const char* app_name, const char* db_name, const char* uri_string = "mongodb://localhost:27017");
    ~qmongo();

    void cleanup();
    int insert(const char* collection_name, bson_t& data);
    int update(const char* collection_name, const char* key, const char* value);
    mongoc_collection_t* get_collection(const char* collection_name);
    
private:
    qmongo(){}
    int connect(const char* app_name, const char* db_name, const char* uri_string);
    int create_client_index_if_not(mongoc_collection_t* collection, const char* collection_name);
    
    mongoc_uri_t *uri = nullptr;
    mongoc_client_t *client = nullptr;
    mongoc_database_t *database = nullptr;
    std::map<int, mongoc_collection_t*> collections;
    interface_qmongo_connection* interface = nullptr;
};

#endif /* qmongo_hpp */
