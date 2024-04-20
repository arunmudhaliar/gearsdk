//
//  Copyright 2024 homenet25
//  qmongo.hpp
//  servercommon
//
//  Created by Arun A on 03/11/23.
//

#ifndef qmongo_hpp
#define qmongo_hpp

#include "../../common/qstring.h"
#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"

#include <map>
#include <mongoc/mongoc.h>

#undef __LOGTAG__
#define __LOGTAG__ "qmongo"

class interface_qmongo_connection {
   public:
	virtual void on_mongo_connect() = 0;
	virtual void on_mongo_create_index_keys(const qstring& collection_name, bson_t* indexkey, mongoc_index_opt_t* opt) = 0;
};

class qmongo {
   public:
	qmongo(interface_qmongo_connection* interfce, const qstring& app_name, const qstring& db_name, const qstring& uri_string = "mongodb://localhost:27017");
	~qmongo();

	void cleanup();
	int insert(const qstring& collection_name, bson_t& query);
	int update(const qstring& collection_name, bson_t& query, bson_t& update);
	mongoc_cursor_t* find(const qstring& collection_name, bson_t& query);
	mongoc_collection_t* get_collection(const qstring& collection_name);
	int connect();

   private:
	qmongo() {}
	int connect(const qstring& app_name, const qstring& db_name, const qstring& uri_string);
	int create_client_index_if_not(mongoc_collection_t* collection, const qstring& collection_name);

	mongoc_uri_t* uri = nullptr;
	mongoc_client_t* client = nullptr;
	mongoc_database_t* database = nullptr;
	std::map<unsigned long, mongoc_collection_t*> collections;
	interface_qmongo_connection* interface = nullptr;

	qstring app_name;
	qstring db_name;
	qstring uri_string;
};

#endif /* qmongo_hpp */
