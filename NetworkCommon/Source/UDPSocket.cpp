//
//  UDPSocket.cpp
//  NetworkCommon
//
//  Created by Arun A on 26/09/23.
//

#include "UDPSocket.hpp"
#include <fcntl.h>
#include <unistd.h>
#include "../../Common/gxCrc32.h"

UDPSocket::UDPSocket() {
}

UDPSocket::~UDPSocket() {
    Close();
}

bool UDPSocket::Open(MSocketListener* listener) {
    if (state != SOCKET_UNINITIALIZED) {
        DEBUG_PRINT_WARN(__LOGTAG__, "Socket [state:%d] not in SOCKET_UNINITIALIZED state. returning !!!", state);
        return false;
    }
    if (listener == nullptr) {
        DEBUG_PRINT_WARN(__LOGTAG__, "Socket listener is null. returning !!!");
        return false;
    }
    this->listener = listener;
    handle = socket( AF_INET,
                         SOCK_DGRAM,
                         IPPROTO_UDP );

    if ( handle <= 0 )
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to create socket");
        this->listener->OnSocketError(this);
        return false;
    }
    
    int32_t nonBlocking = 1;
    if ( fcntl( handle,
                F_SETFL,
                O_NONBLOCK,
                nonBlocking ) == -1 )
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to set non-blocking");
        this->listener->OnSocketError(this);
        Close();
        return false;
    }
    
    const int32_t trueValue = 1;
    int32_t result_opt = setsockopt(handle, SOL_SOCKET, SO_REUSEADDR, &trueValue, sizeof(trueValue));
  #ifdef __APPLE__
    result_opt = setsockopt(handle, SOL_SOCKET, SO_REUSEPORT, &trueValue, sizeof(trueValue));
  #endif
    
    state = SOCKET_OPEN;
    DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "Socket created, handle:%d", handle);
    this->listener->OnSocketOpen(this);
    return  true;
}

bool UDPSocket::Bind(const Address & destination) {
    if (state != SOCKET_OPEN) {
        DEBUG_PRINT_WARN(__LOGTAG__, "Socket [state:%d] not in SOCKET_OPEN state. Bind returning !!!", state);
        return false;
    }
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = destination.GetAddress();
    address.sin_port = htons( destination.GetPort() );
    memset(address.sin_zero, 0, sizeof(address.sin_zero));
    
//    struct hostent *host;
//    host= (struct hostent *) gethostbyname((char *)"127.0.0.1");
//    in_addr sin_addr = *((struct in_addr *)host->h_addr);
    
    int32_t result = bind( handle, (const sockaddr*) &address, sizeof(sockaddr_in) );
    if ( result < 0 )
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to bind socket %d", result);
        this->listener->OnSocketError(this);
        Close();
        return false;
    }
    this->isBinded = true;
    this->listener->OnSocketBind(this);
    return true;
}

int UDPSocket::Close() {
    int result = close( handle );
    if (result < 0) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Socket closure failed: %s", strerror (errno));
        this->listener->OnSocketError(this);
        return result;
    }
    state = SOCKET_CLOSE;
    DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "closed socket");
    listener->OnSocketClose(this);
    return result;
}

long UDPSocket::Send(const Address& destination, Buffer& buffer ) {
    return Send(destination, buffer.data, buffer.index);
}

long UDPSocket::Send( const Address & destination, const void * data, int32_t size ) {
    if (state < SOCKET_OPEN || state >= SOCKET_CLOSE) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Socket [state:%d] not in SOCKET_OPEN OR SOCKET_RECV_LOOP state. returning !!!", state);
        return 0;
    }
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = destination.GetAddress();
    address.sin_port = htons( destination.GetPort() );

//    if ( bind( handle, (const sockaddr*) &address, sizeof(sockaddr_in) ) < 0 )
//    {
//        std::cout << "failed to bind socket\n";
//        return false;
//    }
    
    ssize_t sent_bytes = sendto( handle, (const char*)data, size, 0, (sockaddr*)&address, sizeof(sockaddr_in) );
    if ( sent_bytes != size )
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "failed to send packet");
        this->listener->OnSocketError(this);
        return sent_bytes;
    }
    
    return sent_bytes;
}

void UDPSocket::ReceiveLoop() {
    if (state != SOCKET_OPEN) {
        DEBUG_PRINT_ERROR(__LOGTAG__, "Socket [state:%d] not in SOCKET_OPEN state. ReceiveLoop returning !!!", state);
        return;
    }
//    std::thread thread(NetworkInterface::ReceiveLoopInternal, this);
//    thread.detach();
    pthread_t thread_id;
    if( pthread_create( &thread_id, nullptr,  UDPSocket::ReceiveLoopInternal, (void*)this) < 0)
    {
        DEBUG_PRINT_ERROR(__LOGTAG__, "could not create thread: %s - %d", strerror (errno), errno);
        this->listener->OnSocketError(this);
    }
    state = SOCKET_RECV_LOOP;
}

void* UDPSocket::ReceiveLoopInternal(void* vargp) {
    UDPSocket* socket = ((UDPSocket *) vargp);
    Buffer buffer;
    buffer.Allocate(256);
    Address sender;
    while (socket->IsActive()) {
        long bytes_read = socket->Receive( sender, buffer.data, buffer.size);
        buffer.index = (int32_t)bytes_read;
        if ( !bytes_read )
            break;
        
        if (bytes_read>0) {
            int crc = gxCrc32::Calc(buffer.data, 0, (int)buffer.index, true);
            DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "bytesReceived %ld crc[0x%0x] from %s", bytes_read, crc, sender.ToString().c_str());
            socket->listener->OnSocketMessage(socket, sender, buffer);
        } else if (bytes_read<0) {
            if (errno == EBADF) {
                DEBUG_PRINT_ERROR(__LOGTAG__, "Socket recv error: %s - %d", strerror (errno), errno);
                break;
            }
        }
        socket->listener->OnUpdateLoop();
        usleep(10*1000);    // 10ms
    }
    pthread_exit(0);
}

long UDPSocket::Receive( Address & sender, void * data, int32_t size ) {
    sockaddr_in from;
    socklen_t fromLength = sizeof( from );

    ssize_t bytes = recvfrom( handle,
                          (char*)data,
                         size,
                          0,
                          (sockaddr*)&from,
                          &fromLength );
    if (bytes > 0) {
        unsigned short from_port = ntohs( from.sin_port );
        sender.Set(from.sin_addr.s_addr, from_port);
    }
    return bytes;
}

bool UDPSocket::IsActive() const {
    /*
    while (recv(handle, NULL, 1, MSG_PEEK | MSG_DONTWAIT) != 0) {
        sleep(rand() % 2); // Sleep for a bit to avoid spam
        DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "I am alive: %d\n", handle);
        return true;
    }
    // When the client has disconnected, this line will execute
    DEBUG_PRINT(LOG_LEVEL_1, __LOGTAG__, "Client %d went away :(\n", handle);
    return false;
     */
    return (state >= SOCKET_OPEN && state < SOCKET_CLOSE);
}

bool UDPSocket::IsBinded() const {
    return isBinded;
}
