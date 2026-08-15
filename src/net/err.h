#pragma once

typedef enum {
    NET_OK = 0,
    NET_EARGS,
    NET_ENOMEM,
    NET_ESOCK,
    NET_ERESOLVE,
    NET_ECONNECT,
    NET_ECTX,
    NET_ESSL,
    NET_EHANDSHAKE,
    NET_EVERIFY,
    NET_EWRITE,
    NET_EREAD,
    NET_EHDRS,
    NET_ECLEN,
    NET_ETIMEOUT,
} net_err_t;

extern _Thread_local net_err_t net_errno;

const char *net_strerror(net_err_t err);
