import { serversdk } from "./libserverplugin";

function LOG_FILE(native_server: serversdk.qh3server_ptr, type: serversdk.elog_type, tag: string, message: string) {
    LOG_FILE_WITH_ROOMID(native_server, type, tag, '', '', message);
}
function LOG_FILE_WITH_PID(native_server: serversdk.qh3server_ptr, type: serversdk.elog_type, tag: string, pid: string, message: string) {
    LOG_FILE_WITH_ROOMID(native_server, type, tag, pid, '', message);
}
function LOG_FILE_WITH_ROOMID(native_server: serversdk.qh3server_ptr, type: serversdk.elog_type, tag: string, pid: string, roomid: string, message: string): void {
    serversdk.sdklib.qh3server_logfile(native_server, serversdk.log_lvls.LEVEL_0, type, tag, pid, roomid, message);
}

// qh3 log
export function QH3_INFO(native_server: serversdk.qh3server_ptr, tag: string, message: string) {
    LOG_FILE(native_server, serversdk.elog_type.INFO_LOG, tag, message);
}
export function QH3_DEBUG(native_server: serversdk.qh3server_ptr, tag: string, message: string) {
    LOG_FILE(native_server, serversdk.elog_type.DEBUG_LOG, tag, message);
}
export function QH3_WARN(native_server: serversdk.qh3server_ptr, tag: string, message: string) {
    LOG_FILE(native_server, serversdk.elog_type.WARN_LOG, tag, message);
}
export function QH3_ERROR(native_server: serversdk.qh3server_ptr, tag: string, message: string) {
    LOG_FILE(native_server, serversdk.elog_type.ERROR_LOG, tag, message);
}

export function QH3_INFO_WITH_PID(native_server: serversdk.qh3server_ptr, pid: string, tag: string, message: string) {
    LOG_FILE_WITH_PID(native_server, serversdk.elog_type.INFO_LOG, tag, pid, message);
}
export function QH3_DEBUG_WITH_PID(native_server: serversdk.qh3server_ptr, pid: string, tag: string, message: string) {
    LOG_FILE_WITH_PID(native_server, serversdk.elog_type.DEBUG_LOG, tag, pid, message);
}
export function QH3_WARN_WITH_PID(native_server: serversdk.qh3server_ptr, pid: string, tag: string, message: string) {
    LOG_FILE_WITH_PID(native_server, serversdk.elog_type.WARN_LOG, tag, pid, message);
}
export function QH3_ERROR_WITH_PID(native_server: serversdk.qh3server_ptr, pid: string, tag: string, message: string) {
    LOG_FILE_WITH_PID(native_server, serversdk.elog_type.ERROR_LOG, tag, pid, message);
}

// //q server log
// export function Q_INFO(native_server: serversdk.qserver_ptr, tag: string, message: string) {
//     LOG_FILE(native_server, serversdk.elog_type.INFO_LOG, tag, message);
// }
// export function Q_DEBUG(native_server: serversdk.qserver_ptr, tag: string, message: string) {
//     LOG_FILE(native_server, serversdk.elog_type.DEBUG_LOG, tag, message);
// }
// export function Q_WARN(native_server: serversdk.qserver_ptr, tag: string, message: string) {
//     LOG_FILE(native_server, serversdk.elog_type.WARN_LOG, tag, message);
// }
// export function Q_ERROR(native_server: serversdk.qserver_ptr, tag: string, message: string) {
//     LOG_FILE(native_server, serversdk.elog_type.ERROR_LOG, tag, message);
// }

// export function Q_INFO_WITH_PID(native_server: serversdk.qserver_ptr, pid: string, tag: string, message: string) {
//     LOG_FILE_WITH_PID(native_server, serversdk.elog_type.INFO_LOG, tag, pid, message);
// }
// export function Q_DEBUG_WITH_PID(native_server: serversdk.qserver_ptr, pid: string, tag: string, message: string) {
//     LOG_FILE_WITH_PID(native_server, serversdk.elog_type.DEBUG_LOG, tag, pid, message);
// }
// export function Q_WARN_WITH_PID(native_server: serversdk.qserver_ptr, pid: string, tag: string, message: string) {
//     LOG_FILE_WITH_PID(native_server, serversdk.elog_type.WARN_LOG, tag, pid, message);
// }
// export function Q_ERROR_WITH_PID(native_server: serversdk.qserver_ptr, pid: string, tag: string, message: string) {
//     LOG_FILE_WITH_PID(native_server, serversdk.elog_type.ERROR_LOG, tag, pid, message);
// }