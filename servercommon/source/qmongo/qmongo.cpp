//
//  qmongo.cpp
//  servercommon
//
//  Created by Arun A on 03/11/23.
//

#include "qmongo.hpp"
#include "../../../common/gxcrc32.h"
#include <zlib.h>

qmongo::qmongo(interface_qmongo_connection* interfce_, const qstring& app_name, const qstring& db_name, const qstring& uri_string) :
    interface(interfce_), app_name(app_name), db_name(db_name), uri_string(uri_string) {
}

qmongo::~qmongo() {
    cleanup();
}

void qmongo::cleanup() {
    /*
     * Release our handles and clean up libmongoc
     */
    for (auto c : collections) {
        mongoc_collection_destroy(c.second);
    }
    collections.clear();
    if (database) {
        mongoc_database_destroy(database);
        database = nullptr;
    }
    if (uri) {
        mongoc_uri_destroy(uri);
        uri = nullptr;
    }
    if (client) {
        mongoc_client_destroy(client);
        client = nullptr;
    }
    mongoc_cleanup();
}

int qmongo::connect() {
    return connect(app_name, db_name, uri_string);
}

int qmongo::connect(const qstring& app_name, const qstring& db_name, const qstring& uri_string) {
    /*
     * Required to initialize libmongoc's internals
     */
    mongoc_init();

    bson_error_t error;
    uri = mongoc_uri_new_with_error(uri_string.c_str(), &error);
    if (!uri) {
        DEBUG_PRINT_ERROR(__LOGTAG__,
            "failed to parse URI: %s, error message: %s",
            uri_string.c_str(),
            error.message);
        cleanup();
        return EXIT_FAILURE;
    }

    /*
     * Create a new client instance
     */
    client = mongoc_client_new_from_uri(uri);
    if (!client) {
        cleanup();
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create client !!!");
        return EXIT_FAILURE;
    }

    /*
     * Register the application name so we can track it in the profile logs
     * on the server. This can also be done from the URI (see other examples).
     */
    mongoc_client_set_appname(client, app_name.c_str());

    /*
     * Get a handle on the database "db_name" and collection "coll_name"
     */
    database = mongoc_client_get_database(client, db_name.c_str());

    DEBUG_PRINT_IMPORTANT(__LOGTAG__, "Connected");
    if (interface) {
        interface->on_mongo_connect();
    }
    return EXIT_SUCCESS;
}

int qmongo::create_client_index_if_not(mongoc_collection_t* collection, const qstring& collection_name) {
    if (!interface) {
        return EXIT_FAILURE;
    }
    bson_error_t error;
    bson_t reply;
    bson_t indexkey;
    mongoc_index_opt_t opt;
    mongoc_index_opt_init(&opt);
    bson_init(&indexkey);
    interface->on_mongo_create_index_keys(collection_name.c_str(), &indexkey, &opt);
    if (!mongoc_collection_create_index_with_opts(collection, &indexkey, &opt, nullptr,  &reply, &error)) {
        fprintf(stderr, "%s : err_code %d\n", error.message, error.code);
        bson_destroy(&indexkey);
        bson_validate(&reply, BSON_VALIDATE_NONE, NULL);
        bson_destroy (&reply);
        return EXIT_FAILURE;
    }
    bson_validate (&reply, BSON_VALIDATE_NONE, NULL);
    bson_destroy (&reply);
    bson_destroy(&indexkey);
    return EXIT_SUCCESS;
}

/*
int qmongo::create_client_index_if_not(mongoc_collection_t* collection, const qstring& collection_name) {
    if (!interface) {
        return EXIT_FAILURE;
    }
    bson_error_t error;
    bson_t indexkey;
    bson_init (&indexkey);
    bson_t opt;
    bson_init (&opt);

    interface->on_mongo_create_index_keys(collection_name.c_str(), &indexkey, &opt);

    mongoc_index_model_t *im = mongoc_index_model_new(&indexkey, &opt);

    if (mongoc_collection_create_indexes_with_opts(collection, &im, 1, nullptr, nullptr, &error)) {
        printf ("Successfully created index\n");
    } else {
        fprintf (stderr, "%s\n", error.message);
        bson_destroy (&indexkey);
        mongoc_index_model_destroy(im);
        return EXIT_FAILURE;
    }
    mongoc_index_model_destroy(im);
    bson_destroy (&indexkey);
    return EXIT_SUCCESS;
}
*/

int qmongo::insert(const qstring& collection_name, bson_t& query) {
    bson_error_t error;
    bson_t reply;
    if (!mongoc_collection_insert_one(get_collection(collection_name.c_str()), &query, nullptr, &reply, &error)) {
        fprintf(stderr, "%s : err_code %d\n", error.message, error.code);
        bson_destroy (&reply);
        return EXIT_FAILURE;
    }
    bson_destroy (&reply);
    return EXIT_SUCCESS;
}

int qmongo::update(const qstring& collection_name, bson_t& query, bson_t& update) {
    bson_error_t error;
    mongoc_find_and_modify_opts_t* opts;
    bson_t reply;

    opts = mongoc_find_and_modify_opts_new();
    mongoc_find_and_modify_opts_set_update(opts, &update);

    const mongoc_find_and_modify_flags_t flags = (mongoc_find_and_modify_flags_t)(MONGOC_FIND_AND_MODIFY_UPSERT | MONGOC_FIND_AND_MODIFY_RETURN_NEW);
    /* Create the document if it didn't exist, and return the updated document */
    mongoc_find_and_modify_opts_set_flags(opts, flags);

    bool result = mongoc_collection_find_and_modify_with_opts(
        get_collection(collection_name), &query, opts, &reply, &error);
    if (!result) {
        fprintf(stderr, "%s : err_code %d\n", error.message, error.code);
    }

    //    char* json_string = bson_as_json(&reply, nullptr);
    bson_destroy (&reply);
    mongoc_find_and_modify_opts_destroy(opts);
    return result ? EXIT_SUCCESS : EXIT_FAILURE;
}

mongoc_cursor_t* qmongo::find(const qstring& collection_name, bson_t& query) {
    bson_t* empty = bson_new();
    mongoc_cursor_t* cursor = mongoc_collection_find_with_opts(get_collection(collection_name.c_str()), &query, empty, nullptr);
    bson_destroy(empty);
    return cursor;
}

mongoc_collection_t* qmongo::get_collection(const qstring& collection_name) {
    if (database == nullptr) {
        return nullptr;
    }
    unsigned long  crc = crc32(0L, Z_NULL, 0);
    crc = crc32_z(crc, (const unsigned char*)collection_name.c_str(), collection_name.length());

    std::map<unsigned long, mongoc_collection_t*>::iterator it = collections.find(crc);
    if (it != collections.end()) {
        return it->second;
    }

    const char* db_name = mongoc_database_get_name(database);
    mongoc_collection_t* collection = mongoc_client_get_collection(client, db_name, collection_name.c_str());
    if (collection == nullptr) {
        return nullptr;
    }
    create_client_index_if_not(collection, collection_name);
    collections[crc] = collection;
    return collection;
}

#if 0
int qmongo::qmongo_main(int argc, char* argv[]) {
    const char* uri_string = "mongodb://localhost:27017";
    mongoc_uri_t* uri;
    mongoc_client_t* client;
    mongoc_database_t* database;
    mongoc_collection_t* collection;
    bson_t* command, reply, * insert;
    bson_error_t error;
    char* str;
    bool retval;

    /*
     * Required to initialize libmongoc's internals
     */
    mongoc_init();

    /*
     * Optionally get MongoDB URI from command line
     */
    if (argc > 1) {
        uri_string = argv[1];
    }

    /*
     * Safely create a MongoDB URI object from the given string
     */
    uri = mongoc_uri_new_with_error(uri_string, &error);
    if (!uri) {
        fprintf(stderr,
            "failed to parse URI: %s\n"
            "error message:       %s\n",
            uri_string,
            error.message);
        return EXIT_FAILURE;
    }

    /*
     * Create a new client instance
     */
    client = mongoc_client_new_from_uri(uri);
    if (!client) {
        return EXIT_FAILURE;
    }

    /*
     * Register the application name so we can track it in the profile logs
     * on the server. This can also be done from the URI (see other examples).
     */
    mongoc_client_set_appname(client, "connect-example");

    /*
     * Get a handle on the database "db_name" and collection "coll_name"
     */
    database = mongoc_client_get_database(client, "db_name");
    collection = mongoc_client_get_collection(client, "db_name", "coll_name");

    /*
     * Do work. This example pings the database, prints the result as JSON and
     * performs an insert
     */
    command = BCON_NEW("ping", BCON_INT32(1));

    retval = mongoc_client_command_simple(
        client, "admin", command, NULL, &reply, &error);

    if (!retval) {
        fprintf(stderr, "%s\n", error.message);
        return EXIT_FAILURE;
    }

    str = bson_as_json(&reply, NULL);
    printf("%s\n", str);

    insert = BCON_NEW("hello", BCON_UTF8("world"));

    bson_t* update = BCON_NEW("$set", "{", "x", BCON_INT32(1), "}");

    if (!mongoc_collection_insert_one(collection, insert, NULL, NULL, &error)) {
        fprintf(stderr, "%s\n", error.message);
    }

    bson_destroy(insert);
    bson_destroy(&reply);
    bson_destroy(command);
    bson_free(str);

    /*
     * Release our handles and clean up libmongoc
     */
    mongoc_collection_destroy(collection);
    mongoc_database_destroy(database);
    mongoc_uri_destroy(uri);
    mongoc_client_destroy(client);
    mongoc_cleanup();

    return EXIT_SUCCESS;
}
#endif
