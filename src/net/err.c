#include "err.h"

_Thread_local net_err_t net_errno = NET_OK;

const char *net_strerror(net_err_t err) {
    switch (err) {
    case NET_OK:
        return "ok";
    case NET_EARGS:
        return "invalid arguments";
    case NET_ENOMEM:
        return "out of memory";
    case NET_ESOCK:
        return "socket creation failed";
    case NET_ERESOLVE:
        return "hostname resolution failed";
    case NET_ECONNECT:
        return "connection failed";
    case NET_ECTX:
        return "ssl context creation failed";
    case NET_ESSL:
        return "ssl object creation failed";
    case NET_EHANDSHAKE:
        return "ssl handshake failed";
    case NET_EVERIFY:
        return "ssl certificate verification failed";
    case NET_EWRITE:
        return "ssl write failed";
    case NET_EREAD:
        return "ssl read failed";
    case NET_EHDRS:
        return "response headers too large";
    case NET_ECLEN:
        return "no content-length in response";
    case NET_ETIMEOUT:
        return "timed out";
    default:
        return "unknown error";
    }
}
