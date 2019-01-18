#include <openssl/ssl.h>

long SSL_ctrl(SSL *s, int cmd, long larg, void *parg) {
    switch (cmd) {
        case 8: { /* SSL_CTRL_GET_SESSION_REUSED */
            return SSL_session_reused(s);
        }
        default: break;
    }
    return -1;
}

long SSL_CTX_ctrl(SSL_CTX *ctx, int cmd, long larg, void *parg) {
    switch (cmd) {
        case 32: { /* SSL_CTRL_OPTIONS */
            return SSL_CTX_set_options(ctx, larg);
        }
        case 44: { /* SSL_CTRL_SET_SESS_CACHE_MODE */
            return SSL_CTX_set_session_cache_mode(ctx, larg);
        }
        default: break;
    }
    return -1;
}

void CRYPTO_free(void *str) {
    free(str);
}

void *CRYPTO_malloc(int num, const char *file, int line) {
    return malloc(num);
}

void CRYPTO_lock(int mode, int type, const char *file, int line) {
    CRYPTO_lock(mode, type, file, line);
}
