#include <openssl/ssl.h>
#include <log/log.h>

long SSL_ctrl(SSL *ssl, int cmd, long larg, void *parg) {
    ALOGD("SSL_ctrl: ssl=%p cmd=%d larg=%ld parg=%p", ssl, cmd, larg, parg);
    return SSL_set_mode(ssl, larg);
}
