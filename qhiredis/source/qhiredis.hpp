//
//  qhiredis.hpp
//  qhiredis
//
//  Created by Arun A on 01/11/23.
//

#ifndef qhiredis_hpp
#define qhiredis_hpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hiredis.h>

//https://www.tutorialspoint.com/redis/redis_server.htm
//https://redis.io/docs/management/persistence/

class qhiredis {
public:
    void example_argv_command(redisContext *c, int n);
    int hiredis_main(int argc, char **argv);
};

#endif /* qhiredis_hpp */
