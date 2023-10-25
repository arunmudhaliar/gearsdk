//
//  Address.hpp
//  NetworkCommon
//
//  Created by Arun A on 26/09/23.
//

#ifndef Address_hpp
#define Address_hpp

#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include "../../Common/SDKTypes.hpp"

class Address
{
public:
    Address();
    Address( unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port );
    Address( uint32_t address, unsigned short port );
    uint32_t GetAddress() const;
    unsigned char GetA() const;
    unsigned char GetB() const;
    unsigned char GetC() const;
    unsigned char GetD() const;
    unsigned short GetPort() const;
    void Set(uint32_t address, unsigned short port);
    std::string ToString() const;
    
private:
    uint32_t address = INADDR_ANY;
    unsigned short port = GSDK_UDP_DEFAULT_PORT;
};
#endif /* Address_hpp */
