/**
 * @file qmongo.hpp
 * @brief This file contains the declaration of the qmongo class and the interface_qmongo_connection class.
 * 
 * The qmongo class provides a way to interact with MongoDB using the qmongo library. It allows inserting, updating, and finding documents in a collection, as well as connecting to the MongoDB server.
 * 
 * The interface_qmongo_connection class is an interface for handling qmongo connection events. It defines two pure virtual functions: on_mongo_connect() and on_mongo_create_index_keys().
 * 
 * @author Arun A
 * @date 2023
 * @copyright 2024 homenet25
 */


#ifndef qmongo_hpp
#define qmongo_hpp

#include "../../common/qstring.h"
#include "../../common/sdktypes.hpp"
#include "../../networkcommon/source/essentials.hpp"

#include <map>
#include <mongoc/mongoc.h>

#undef __LOGTAG__
#define __LOGTAG__ "qmongo"

/**
 * @brief The interface for handling qmongo connection events.
 */
class interface_qmongo_connection {
   public:
	/**
	 * @brief Called when the connection to MongoDB is established.
	 */
	virtual void on_mongo_connect() = 0;

	/**
	 * @brief Called when creating index keys for a collection.
	 * @param collection_name The name of the collection.
	 * @param indexkey The BSON index key.
	 * @param opt The index options.
	 */
	virtual void on_mongo_create_index_keys(const qstring& collection_name, bson_t* indexkey, mongoc_index_opt_t* opt) = 0;
};

/**
 * @brief A class for interacting with MongoDB using the qmongo library.
 */
class qmongo {
   public:
	/**
	 * @brief Constructs a qmongo object.
	 * @param interfce The interface for handling qmongo connection events.
	 * @param app_name The name of the application.
	 * @param db_name The name of the database.
	 * @param uri_string The MongoDB connection URI string.
	 */
	qmongo(interface_qmongo_connection* interfce, const qstring& app_name, const qstring& db_name, const qstring& uri_string = "mongodb://localhost:27017");

	/**
	 * @brief Destroys the qmongo object.
	 */
	~qmongo();

	/**
	 * @brief Cleans up the qmongo object.
	 */
	void cleanup();

	/**
	 * @brief Inserts a document into the specified collection.
	 * @param collection_name The name of the collection.
	 * @param query The BSON query document.
	 * @return The result of the insert operation.
	 */
	int insert(const qstring& collection_name, bson_t& query);

	/**
	 * @brief Updates documents in the specified collection that match the query.
	 * @param collection_name The name of the collection.
	 * @param query The BSON query document.
	 * @param update The BSON update document.
	 * @return The result of the update operation.
	 */
	int update(const qstring& collection_name, bson_t& query, bson_t& update);

	/**
	 * @brief Finds documents in the specified collection that match the query.
	 * @param collection_name The name of the collection.
	 * @param query The BSON query document.
	 * @return A cursor to iterate over the result set.
	 */
	mongoc_cursor_t* find(const qstring& collection_name, bson_t& query);

	/**
	 * @brief Gets the specified collection.
	 * @param collection_name The name of the collection.
	 * @return The mongoc_collection_t object representing the collection.
	 */
	mongoc_collection_t* get_collection(const qstring& collection_name);

	/**
	 * @brief Connects to the MongoDB server.
	 * @return The result of the connection operation.
	 */
	int connect();

   private:
	qmongo() {}

	/**
	 * @brief Connects to the MongoDB server.
	 * @param app_name The name of the application.
	 * @param db_name The name of the database.
	 * @param uri_string The MongoDB connection URI string.
	 * @return The result of the connection operation.
	 */
	int connect(const qstring& app_name, const qstring& db_name, const qstring& uri_string);

	/**
	 * @brief Creates an index for the specified collection if it does not exist.
	 * @param collection The mongoc_collection_t object representing the collection.
	 * @param collection_name The name of the collection.
	 * @return The result of the index creation operation.
	 */
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
