//
//  Address.cpp
//  NetworkCommon
//
//  Created by Arun A on 26/09/23.
//

#include "Address.hpp"
#include <sstream>

Address::Address() {
    Set(address, port);
}

Address::Address( unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port ) {
    uint32_t address = ( a ) |
               ( b << 8 ) |
               ( c << 16  ) |
                 (d<<24);
    Set(address, port);
}

Address::Address( uint32_t address, unsigned short port ) {
    Set(address, port);
}

void Address::Set(uint32_t address, unsigned short port) {
    this->address = address;
    this->port = port;
}

uint32_t Address::GetAddress() const {
    return address;
}

unsigned char Address::GetA() const {
    return (address&0x000000ff);

}

unsigned char Address::GetB() const {
    return (address&0x0000ff00) >> 8;
}

unsigned char Address::GetC() const {
    return (address&0x00ff0000) >> 16;
}

unsigned char Address::GetD() const {
    return (address&0xff000000) >> 24;
}

unsigned short Address::GetPort() const {
    return  port;
}

std::string Address::ToString() const {
    std::stringstream ss;    
    ss << (int)GetA() << "." << (int)GetB() << "." << (int)GetC() << "." << (int)GetD() << ":" << (int)GetPort();
    std::string str = ss.str();
    return ss.str();
}
