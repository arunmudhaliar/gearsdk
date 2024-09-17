//
//  Copyright 2024 homenet25
//  gclient.hpp
//  qgfist
//
//  Created by Arun A on 17/09/2024.
//

#ifndef gclient_hpp
#define gclient_hpp

#include "../qclient/source/qnetworkclient.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "gclient"

using namespace client;

class gclient : public qnetworkclient {
   public:
	virtual ~gclient();

   protected:
	void onconnect(conn_io_client* qconnection) override;
};

#endif /* gclient_hpp */
