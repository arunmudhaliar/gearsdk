//
//  Copyright 2024 homenet25
//  gameclient.hpp
//  networkclient
//
//  Created by Arun A on 15/10/23.
//

#ifndef gameclient_hpp
#define gameclient_hpp

#include "qnetworkclient.hpp"

using namespace client;

class gameclient : public qnetworkclient {
   public:
   virtual ~gameclient();
   protected:
	void onconnect(conn_io_client* qconnection) override;

   public:
	bool shutdown_client = false;
	void test_send_shutdown_event();
};

#endif /* gameclient_hpp */
