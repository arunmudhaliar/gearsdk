//
//  Copyright 2024 homenet25
//  qhiredis.cpp
//  qhiredis
//
//  Created by Arun A on 01/11/23.
//

#include "qhiredis.hpp"

qhiredis::qhiredis(const qstring NAME, const qstring& redis_ip, uint16_t redis_port) : NAME(NAME), redis_ip(redis_ip), redis_port(redis_port) {
	context = nullptr;
}

qhiredis::~qhiredis() {
	disconnect_redis();
}

int qhiredis::connect_redis(bool unix_socket) {
	return connect_redis_internal(redis_ip, redis_port, unix_socket);
}

int qhiredis::connect_redis_internal(const qstring& hostname, uint16_t port, bool unix_socket) {
	const char* logtag = NAME.c_str();
	if (context) {
		debug_print_important(logtag, "Already connected !!! - %s:%d", hostname.c_str(), port);
		return 0;
	}
	struct timeval timeout = {30, 500000};	// 30.5 seconds
	if (unix_socket) {
		context = redisConnectUnixWithTimeout(hostname.c_str(), timeout);
	} else {
		context = redisConnectWithTimeout(hostname.c_str(), (int) port, timeout);
	}
	if (context == nullptr || context->err) {
		if (context) {
			debug_print_error(logtag, "Connection error: %s - %s:%d", context->errstr, hostname.c_str(), port);
			disconnect_redis();
		} else {
			debug_print_error(logtag, "Connection error: can't allocate redis context - %s:%d", hostname.c_str(), port);
		}
		return 1;
	}

	// Setting the notify-keyspace-events to Ex
	redisReply* reply = (redisReply*) redisCommand(context, "CONFIG SET notify-keyspace-events Ex");
	if (reply->type == REDIS_REPLY_ERROR) {
		debug_print_error(logtag, "Error setting hiredis CONFIG: %s", reply->str);
	} else {
		debug_print(LOG_LEVEL_4, logtag, "hiredis CONFIG set successfully");
	}

	debug_print_important(logtag, "Connected - %s:%d", hostname.c_str(), port);
	return 0;
}

void qhiredis::disconnect_redis() {
	if (!context) {
		return;
	}
	/* Disconnects and frees the context */
	redisFree(context);
	context = nullptr;
}

int qhiredis::retry_connection() {
	const char* logtag = NAME.c_str();
	debug_print_important(logtag, "Retrying qhiredis ...");
	disconnect_redis();
	int result = connect_redis();
	if (result != 0) {
		debug_print_error(logtag, "Redis retry FAILED !!!");
	}
	return result;
}

int qhiredis::set_value(const qstring& key, const qstring& value) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	redisReply* reply = (redisReply*) redisCommand(context, "SET %b %b", key.c_str(), key.length(), value.c_str(), value.length());
	if (reply == nullptr) {
		if (context->err) {
			debug_print_error(logtag, "Redis error: %s", context->errstr);
			retry_connection();
		}
		return 1;
	}
	freeReplyObject(reply);
	return 0;
}

int qhiredis::set_value(const qstring& key, const qstring& value, int expiry_in_sec) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	redisReply* reply = (redisReply*) redisCommand(context, "SET %b %b EX %d", key.c_str(), key.length(), value.c_str(), value.length(), expiry_in_sec);
	if (reply == nullptr) {
		if (context->err) {
			debug_print_error(logtag, "Redis error: %s", context->errstr);
			retry_connection();
		}
		return 1;
	}
	freeReplyObject(reply);
	return 0;
}

int qhiredis::get_value(const qstring& key, qstring& value) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	redisReply* reply = (redisReply*) redisCommand(context, "GET %b", key.c_str(), key.length());
	if (reply == nullptr) {
		if (context->err) {
			debug_print_error(logtag, "Redis error: %s", context->errstr);
			retry_connection();
		}
		return 1;
	}
	value = qstring::format_string("%.*s", reply->len, reply->str);
	freeReplyObject(reply);
	return 0;
}

int qhiredis::set_hash_value(const qstring& hashkey, const qstring& field, const qstring& value) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	redisReply* reply = (redisReply*) redisCommand(context, "HSET %b %b %b", hashkey.c_str(), hashkey.length(), field.c_str(), field.length(), value.c_str(), value.length());
	if (reply == nullptr) {
		if (context->err) {
			debug_print_error(logtag, "Redis error: %s", context->errstr);
			retry_connection();
		}
		return 1;
	}

	// Check reply->type to confirm the command was successful
	if (reply->type == REDIS_REPLY_INTEGER) {
		debug_print(LOG_LEVEL_4, logtag, "The field was newly set: %lld, hkey:%s, field:%s, value:%s", reply->integer, hashkey.c_str(), field.c_str(), value.c_str());
	} else if (reply->type == REDIS_REPLY_STATUS) {
		debug_print(LOG_LEVEL_4, logtag, "Status: %s", reply->str);
	} else {
		debug_print_warn(logtag, "Unexpected reply type: %d", reply->type);
	}

	freeReplyObject(reply);
	return 0;
}

int qhiredis::get_hash_value(const qstring& hashkey, const qstring& field, qstring& value) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	// Get the hash field value.
	redisReply* reply = (redisReply*) redisCommand(context, "HGET %b %b", hashkey.c_str(), hashkey.length(), field.c_str(), field.length());
	if (reply == nullptr) {
		if (context->err) {
			debug_print_error(logtag, "Redis error: %s", context->errstr);
			retry_connection();
		}
		return 1;
	}

	// Check if the value was successfully retrieved
	if (reply->type == REDIS_REPLY_STRING) {
		value = qstring::format_string("%.*s", reply->len, reply->str);
	} else if (reply->type == REDIS_REPLY_NIL) {
		debug_print_warn(logtag, "The field does not exist.");
	} else {
		debug_print_warn(logtag, "Unexpected reply type: %d", reply->type);
	}

	freeReplyObject(reply);
	return 0;
}

int qhiredis::delete_hash_field(const qstring& hashkey, const qstring& field) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	// Execute HDEL command to delete the field from the Hash Set
	redisReply* reply = (redisReply*) redisCommand(context, "HDEL %b %b", hashkey.c_str(), hashkey.length(), field.c_str(), field.length());
	if (reply == NULL) {
		debug_print_error(logtag, "Error executing HDEL command");
		return 1;
	}

	// Check if the field was successfully deleted
	if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1) {
		// printf("Field '%s' deleted from Hash Set '%s'\n", field, key);
	} else {
		debug_print_warn(logtag, "Field '%s' not found [key:%s] or error occurred", field.c_str(), hashkey.c_str());
	}

	// Free the reply object
	freeReplyObject(reply);
	return 0;
}

void qhiredis::iterate_hash(const qstring& key, void* arg, type_redis_hash_iterator_field_value_cb callback) {
	const char* logtag = NAME.c_str();
	if (!context) {
		debug_print_error(logtag, "Error. Context is null.");
		return;
	}
	iterate_hash(context, key.c_str(), arg, callback);
}

void qhiredis::iterate_hash(redisContext* context, const char* hash_key, void* arg, type_redis_hash_iterator_field_value_cb callback) {
	unsigned long cursor = 0;
	redisReply* reply;
	do {
		// Execute HSCAN command
		reply = (redisReply*) redisCommand(context, "HSCAN %s %lu", hash_key, cursor);

		if (reply != nullptr && reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
			// The first element of the reply is the new cursor to use in the next call.
			cursor = strtoul(reply->element[0]->str, nullptr, 10);

			// The second element of the reply is an array of key-value pairs.
			redisReply* data = reply->element[1];
			for (size_t i = 0; i < data->elements; i += 2) {
				//                printf("Field: %s, Value: %s\n", data->element[i]->str, data->element[i + 1]->str);
				callback(data->element[i]->str, data->element[i + 1]->str, arg);
			}
		} else {
			debug_print_error(__LOGTAG__, "Error: %s", reply->str);
			break;
		}
		freeReplyObject(reply);
	} while (cursor != 0);
}

void qhiredis::scan(const qstring& prefix_key, void* arg, type_redis_scan_iterator_key_field_value_cb callback) {
	const char* logtag = NAME.c_str();
	if (!context) {
		debug_print_error(logtag, "Error. Context is null.");
		return;
	}
	// Using SCAN to iterate over keys that match a pattern
	redisReply* reply;
	unsigned long cursor = 0;
	do {
		reply = (redisReply*) redisCommand(context, "SCAN %lu MATCH %s:*", cursor, prefix_key.c_str());
		if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 2) {
			cursor = strtoul(reply->element[0]->str, NULL, 10);	 // Update cursor

			// Process the list of keys
			redisReply* keys = reply->element[1];
			for (size_t i = 0; i < keys->elements; i++) {
				//                printf("Found key: %s\n", keys->element[i]->str);

				// Fetch all fields and values of the hash stored at the key
				redisReply* hash_content = (redisReply*) redisCommand(context, "HGETALL %s", keys->element[i]->str);
				if (hash_content->type == REDIS_REPLY_ARRAY) {
					for (size_t j = 0; j < hash_content->elements; j += 2) {
						//                        printf("  %s: %s\n", hashContent->element[j]->str, hashContent->element[j+1]->str);
						callback(keys->element[i]->str, hash_content->element[j]->str, hash_content->element[j + 1]->str, arg);
					}
				}
				freeReplyObject(hash_content);
			}
		}
		freeReplyObject(reply);
	} while (cursor != 0);
}

int qhiredis::incr(const qstring& key, long long& value) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	redisReply* reply = (redisReply*) redisCommand(context, "INCR %b", key.c_str(), key.length());
	if (reply != nullptr) {
		value = reply->integer;
		debug_print(LOG_LEVEL_3, __LOGTAG__, "The incremented value of '%s' is: %lld", key.c_str(), reply->integer);
	} else {
		if (context->err) {
			debug_print_error(logtag, "Redis error: %s", context->errstr);
			retry_connection();
		}
		return 1;
	}
	freeReplyObject(reply);
	return 0;
}

int qhiredis::decr(const qstring& key, long long& value) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	redisReply* reply = (redisReply*) redisCommand(context, "DECR %b", key.c_str(), key.length());
	if (reply != nullptr) {
		value = reply->integer;
		debug_print(LOG_LEVEL_3, __LOGTAG__, "The decremented value of '%s' is: %lld", key.c_str(), reply->integer);
	} else {
		if (context->err) {
			debug_print_error(logtag, "Redis error: %s", context->errstr);
			retry_connection();
		}
		return 1;
	}
	freeReplyObject(reply);
	return 0;
}

int qhiredis::incr_by(const qstring& key, const int DELTA, long long& value) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	redisReply* reply = (redisReply*) redisCommand(context, "INCRBY %b %d", key.c_str(), key.length(), DELTA);
	if (reply != nullptr) {
		value = reply->integer;
		debug_print(LOG_LEVEL_3, __LOGTAG__, "The incremented value of '%s' is: %lld", key.c_str(), reply->integer);
	} else {
		if (context->err) {
			debug_print_error(logtag, "Redis error: %s", context->errstr);
			retry_connection();
		}
		return 1;
	}
	freeReplyObject(reply);
	return 0;
}

int qhiredis::decr_by(const qstring& key, const int DELTA, long long& value) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	redisReply* reply = (redisReply*) redisCommand(context, "DECRBY %b %d", key.c_str(), key.length(), DELTA);
	if (reply != nullptr) {
		value = reply->integer;
		debug_print(LOG_LEVEL_3, __LOGTAG__, "The decremented value of '%s' is: %lld", key.c_str(), reply->integer);
	} else {
		if (context->err) {
			debug_print_error(logtag, "Redis error: %s", context->errstr);
			retry_connection();
		}
		return 1;
	}
	freeReplyObject(reply);
	return 0;
}

int qhiredis::expire_key(const qstring& key, int expiry_in_sec) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	redisReply* reply = (redisReply*) redisCommand(context, "EXPIRE %b %d", key.c_str(), key.length(), expiry_in_sec);
	if (reply != nullptr) {
		if (reply->type == REDIS_REPLY_INTEGER && reply->integer == 1) {
			debug_print(LOG_LEVEL_4, logtag, "'%s' expiration time set successfully.\n", key.c_str());
		} else {
			debug_print_error(logtag, "Failed to set expiration time for '%s'.", key.c_str());
		}
	} else {
		if (context->err) {
			debug_print_error(logtag, "Redis error: %s", context->errstr);
			retry_connection();
		}
		return 1;
	}
	freeReplyObject(reply);
	return 0;
}

int qhiredis::delete_key(const qstring& key) {
	if (!context) {
		return 2;
	}
	const char* logtag = NAME.c_str();
	redisReply* reply = (redisReply*) redisCommand(context, "DEL %b", key.c_str(), key.length());
	if (reply == nullptr) {
		debug_print_error(logtag, "hiredis DEL execution failed for key %s", key.c_str());
		return 1;
	}
	freeReplyObject(reply);
	return 0;
}

#if 0
void qhiredis::example_argv_command(redisContext* c, int n) {
    char** argv, tmp[42];
    size_t* argvlen;
    redisReply* reply;

    /* We're allocating two additional elements for command and key */
    argv = (char**)malloc(sizeof(*argv) * (2 + n));
    argvlen = (size_t*)malloc(sizeof(*argvlen) * (2 + n));

    /* First the command */
    argv[0] = (char*)"RPUSH";
    argvlen[0] = sizeof("RPUSH") - 1;

    /* Now our key */
    argv[1] = (char*)"argvlist";
    argvlen[1] = sizeof("argvlist") - 1;

    /* Now add the entries we wish to add to the list */
    for (size_t i = 2; i < (n + 2); i++) {
        argvlen[i] = snprintf(tmp, sizeof(tmp), "argv-element-%zu", i - 2);
        argv[i] = strdup(tmp);
    }

    /* Execute the command using redisCommandArgv.  We're sending the arguments with
     * two explicit arrays.  One for each argument's string, and the other for its
     * length. */
    reply = (redisReply*)redisCommandArgv(c, n + 2, (const char**)argv, (const size_t*)argvlen);

    if (reply == NULL || c->err) {
        fprintf(stderr, "Error:  Couldn't execute redisCommandArgv\n");
        exit(1);
    }

    if (reply->type == REDIS_REPLY_INTEGER) {
        printf("%s reply: %lld\n", argv[0], reply->integer);
    }

    freeReplyObject(reply);

    /* Clean up */
    for (size_t i = 2; i < (n + 2); i++) {
        free(argv[i]);
    }

    free(argv);
    free(argvlen);
}

int qhiredis::hiredis_main(int argc, char** argv) {
    unsigned int j, isunix = 0;
    redisContext* c;
    redisReply* reply;
    const char* hostname = (argc > 1) ? argv[1] : "127.0.0.1";

    if (argc > 2) {
        if (*argv[2] == 'u' || *argv[2] == 'U') {
            isunix = 1;
            /* in this case, host is the path to the unix socket */
            printf("Will connect to unix socket @%s\n", hostname);
        }
    }

    int port = (argc > 2) ? atoi(argv[2]) : 6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    if (isunix) {
        c = redisConnectUnixWithTimeout(hostname, timeout);
    }
    else {
        c = redisConnectWithTimeout(hostname, port, timeout);
    }
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        }
        else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    /* PING server */
    reply = (redisReply*)redisCommand(c, "PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key */
    reply = (redisReply*)redisCommand(c, "SET %s %s", "foo", "hello world");
    printf("SET: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key using binary safe API */
    reply = (redisReply*)redisCommand(c, "SET %b %b", "bar", (size_t)3, "hello", (size_t)5);
    printf("SET (binary API): %s\n", reply->str);
    freeReplyObject(reply);

    /* Try a GET and two INCR */
    reply = (redisReply*)redisCommand(c, "GET foo");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply);

    reply = (redisReply*)redisCommand(c, "INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);
    /* again ... */
    reply = (redisReply*)redisCommand(c, "INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);

    /* Create a list of numbers, from 0 to 9 */
    reply = (redisReply*)redisCommand(c, "DEL mylist");
    freeReplyObject(reply);
    for (j = 0; j < 10; j++) {
        char buf[64];

        snprintf(buf, 64, "%u", j);
        reply = (redisReply*)redisCommand(c, "LPUSH mylist element-%s", buf);
        freeReplyObject(reply);
    }

    /* Let's check what we have inside the list */
    reply = (redisReply*)redisCommand(c, "LRANGE mylist 0 -1");
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            printf("%u) %s\n", j, reply->element[j]->str);
        }
    }
    freeReplyObject(reply);

    /* See function for an example of redisCommandArgv */
    example_argv_command(c, 10);

    /* Disconnects and frees the context */
    redisFree(c);

    return 0;
}
#endif
