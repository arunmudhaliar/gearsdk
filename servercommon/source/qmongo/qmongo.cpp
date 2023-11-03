//
//  qmongo.cpp
//  servercommon
//
//  Created by Arun A on 03/11/23.
//

#include "qmongo.hpp"
#include "../../../common/gxcrc32.h"

qmongo::qmongo(interface_qmongo_connection* interfce_, const char* app_name, const char* db_name, const char* uri_string) :
    interface(interfce_)
{
    connect(app_name, db_name, uri_string);
}

qmongo::~qmongo() {
    cleanup();
}

void qmongo::cleanup() {
    /*
     * Release our handles and clean up libmongoc
     */
    for (auto c : collections) {
        mongoc_collection_destroy (c.second);
    }
    collections.clear();
    if (database) {
        mongoc_database_destroy (database);
        database = nullptr;
    }
    if (uri) {
        mongoc_uri_destroy (uri);
        uri = nullptr;
    }
    if (client) {
        mongoc_client_destroy (client);
        client = nullptr;
    }
    mongoc_cleanup ();
}

int qmongo::connect(const char* app_name, const char* db_name, const char* uri_string) {
    /*
     * Required to initialize libmongoc's internals
     */
    mongoc_init ();
    
    bson_error_t error;
    uri = mongoc_uri_new_with_error (uri_string, &error);
    if (!uri) {
        fprintf (stderr,
            "failed to parse URI: %s\n"
            "error message:       %s\n",
            uri_string,
            error.message);
        cleanup ();
        return EXIT_FAILURE;
    }
    
    /*
     * Create a new client instance
     */
    client = mongoc_client_new_from_uri (uri);
    if (!client) {
        cleanup ();
       return EXIT_FAILURE;
    }
    
    /*
     * Register the application name so we can track it in the profile logs
     * on the server. This can also be done from the URI (see other examples).
     */
    mongoc_client_set_appname (client, app_name);
    
    /*
     * Get a handle on the database "db_name" and collection "coll_name"
     */
    database = mongoc_client_get_database (client, db_name);
    
    if (interface) {
        interface->on_mongo_connect();
    }
    return EXIT_SUCCESS;
}

int qmongo::create_client_index_if_not(mongoc_collection_t* collection, const char* collection_name) {
    if (!interface) {
        return EXIT_FAILURE;
    }
    bson_error_t error;
    bson_t indexkey;
    mongoc_index_opt_t opt;
    bson_init (&indexkey);
    interface->on_mongo_create_index_keys(collection_name, &indexkey, &opt);
    if (!mongoc_collection_create_index (collection, &indexkey, &opt, &error)) {
        fprintf (stderr, "%s\n", error.message);
        bson_destroy (&indexkey);
         return EXIT_FAILURE;
     }
    bson_destroy (&indexkey);
    return EXIT_SUCCESS;
}

int qmongo::insert(const char* collection_name, bson_t& data) {
    bson_error_t error;
    if (!mongoc_collection_insert_one (get_collection(collection_name), &data, NULL, NULL, &error)) {
       fprintf (stderr, "%s\n", error.message);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int qmongo::update(const char* collection_name, const char* key, const char* value) {
    return 0;
}

mongoc_collection_t* qmongo::get_collection(const char* collection_name) {
    if (database == nullptr) {
        return nullptr;
    }
    int crc32 = gxcrc32::Calc((unsigned char*)collection_name, 0, (int)strlen(collection_name));
    std::map<int, mongoc_collection_t*>::iterator it = collections.find(crc32);
    if (it!=collections.end()) {
        return it->second;
    }
    
    const char* db_name = mongoc_database_get_name(database);
    mongoc_collection_t* collection = mongoc_client_get_collection (client, db_name, collection_name);
    if (collection == nullptr) {
        return nullptr;
    }
    create_client_index_if_not(collection, collection_name);
    collections[crc32] = collection;
    return collection;
}

#if 0
int qmongo::qmongo_main(int argc, char *argv[])
{
   const char *uri_string = "mongodb://localhost:27017";
   mongoc_uri_t *uri;
   mongoc_client_t *client;
   mongoc_database_t *database;
   mongoc_collection_t *collection;
   bson_t *command, reply, *insert;
   bson_error_t error;
   char *str;
   bool retval;

   /*
    * Required to initialize libmongoc's internals
    */
   mongoc_init ();

   /*
    * Optionally get MongoDB URI from command line
    */
   if (argc > 1) {
      uri_string = argv[1];
   }

   /*
    * Safely create a MongoDB URI object from the given string
    */
   uri = mongoc_uri_new_with_error (uri_string, &error);
   if (!uri) {
      fprintf (stderr,
               "failed to parse URI: %s\n"
               "error message:       %s\n",
               uri_string,
               error.message);
      return EXIT_FAILURE;
   }

   /*
    * Create a new client instance
    */
   client = mongoc_client_new_from_uri (uri);
   if (!client) {
      return EXIT_FAILURE;
   }

   /*
    * Register the application name so we can track it in the profile logs
    * on the server. This can also be done from the URI (see other examples).
    */
   mongoc_client_set_appname (client, "connect-example");

   /*
    * Get a handle on the database "db_name" and collection "coll_name"
    */
   database = mongoc_client_get_database (client, "db_name");
   collection = mongoc_client_get_collection (client, "db_name", "coll_name");

   /*
    * Do work. This example pings the database, prints the result as JSON and
    * performs an insert
    */
   command = BCON_NEW ("ping", BCON_INT32 (1));

   retval = mongoc_client_command_simple (
      client, "admin", command, NULL, &reply, &error);

   if (!retval) {
      fprintf (stderr, "%s\n", error.message);
      return EXIT_FAILURE;
   }

   str = bson_as_json (&reply, NULL);
   printf ("%s\n", str);

   insert = BCON_NEW ("hello", BCON_UTF8 ("world"));

    bson_t *update = BCON_NEW ("$set", "{", "x", BCON_INT32 (1), "}");
    
   if (!mongoc_collection_insert_one (collection, insert, NULL, NULL, &error)) {
      fprintf (stderr, "%s\n", error.message);
   }

   bson_destroy (insert);
   bson_destroy (&reply);
   bson_destroy (command);
   bson_free (str);

   /*
    * Release our handles and clean up libmongoc
    */
   mongoc_collection_destroy (collection);
   mongoc_database_destroy (database);
   mongoc_uri_destroy (uri);
   mongoc_client_destroy (client);
   mongoc_cleanup ();

   return EXIT_SUCCESS;
}
#endif
