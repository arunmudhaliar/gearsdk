//
//  qh3serverplugin.h
//  qh3server
//
//  Created by Arun A on 22/12/24.
//

#ifndef QH3SERVERPLUGIN_H
#define QH3SERVERPLUGIN_H

#include "../../../common/sdktypes.hpp"
#include "../qh3server.hpp"

#undef __LOGTAG__
#define __LOGTAG__ "qunitysdk"

namespace gsdk {
namespace server {

#if PLATFORM == PLATFORM_WINDOWS
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default"))) __attribute__((unused))
#endif

extern "C" {
EXPORT qh3server* spawn_qh3server();
}

}  // namespace server
}  // namespace gsdk

#endif /* QH3SERVERPLUGIN_H */
