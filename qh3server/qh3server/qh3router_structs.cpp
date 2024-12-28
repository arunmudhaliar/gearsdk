//
//  Copyright 2024 homenet25
//  qh3router_structs.cpp
//  qh3server
//
//  Created by Arun A on 30/12/23.
//

#include "qh3router_structs.h"

#include <fcntl.h>

#if 0
void test_router_command_recv_cb(EV_P_ ev_io* w, int revents)  {
    UNUSED(revents);
    
    debug_print(LOG_LEVEL_0, __LOGTAG__, "RECEIVED !!!");
    route* route_client = (route*)w->data;
    static uint8_t buf_r[65535];
    while (1) {
        struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(peer_addr);
        memset(&peer_addr, 0, peer_addr_len);

        ssize_t read = recvfrom(route_client->bridge_sock, buf_r, sizeof(buf_r), 0,
            (struct sockaddr*)&peer_addr,
            &peer_addr_len);

        if (read < 0) {
            if ((errno == EWOULDBLOCK) || (errno == EAGAIN)) {
                debug_print(LOG_LEVEL_5, __LOGTAG__, "recv would block");
                break;
            }

            debug_print_error(__LOGTAG__, "failed to read");
            return;
        }
    }
}
#endif

ssize_t route::relay(uint8_t* buf, ssize_t len) {
	ssize_t sent = sendto(bridge_sock, buf, len, 0, peer->ai_addr, peer->ai_addrlen);
	if (sent != len) {
		debug_print_error(__LOGTAG__, "failed to send");
		return -1;
	}
	return sent;
}
int route::create_bridge(struct ev_loop* loop, void* arg, void (*router_command_recv_cb_ptr)(EV_P_ ev_io* w, int revents)) {
	const struct addrinfo HINTS = {.ai_family = PF_UNSPEC, .ai_socktype = SOCK_DGRAM, .ai_protocol = IPPROTO_UDP};

	if (getaddrinfo(host.c_str(), port.c_str(), &HINTS, &peer) != 0) {
		debug_print_error(__LOGTAG__, "bridge - failed to resolve host");
		return -1;
	}

	bridge_sock = socket(peer->ai_family, SOCK_DGRAM, 0);
	if (bridge_sock < 0) {
		debug_print_error(__LOGTAG__, "failed to create bridge socket");
		freeaddrinfo(peer);
		return -1;
	}

	if (fcntl(bridge_sock, F_SETFL, O_NONBLOCK) != 0) {
		debug_print_error(__LOGTAG__, "failed to make bridge socket non-blocking");
		freeaddrinfo(peer);
		close_bridge_socket();
		return -1;
	}

	if (router_command_recv_cb_ptr != nullptr) {
		if (bind(bridge_sock, peer->ai_addr, peer->ai_addrlen) < 0) {
			debug_print_error(__LOGTAG__, "failed to bind bridge socket - port[%s:%s]", host.c_str(), port.c_str());
			freeaddrinfo(peer);
			close_bridge_socket();
			return -1;
		}
		ev_io_init(&command_watcher, router_command_recv_cb_ptr, bridge_sock, EV_READ);
		ev_io_start(loop, &command_watcher);
		this->arg = arg;
		command_watcher.data = this;
		debug_print_important2(__LOGTAG__, "Bridge socket enabled for receive in %s:%s", host.c_str(), port.c_str());
	}

	return 0;
}

int route::close_bridge_socket() {
	if (bridge_sock == -1) {
		return -1;
	}
	int result = close(bridge_sock);
	if (result < 0) {
		debug_print_error(__LOGTAG__, "Bridge socket closure failed: %s", strerror(errno));
	}
	return result;
}

void route::refresh_hb_timestamp(struct ev_loop* loop) {
	last_hb_received_time = ev_now(loop);
}
