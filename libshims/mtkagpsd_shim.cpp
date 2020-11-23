#define LOG_TAG "SSL_ctrl_stub"

long SSL_ctrl() {return -1;}
long SSL_CTX_ctrl() {return -1;}
void CRYPTO_free();
void *CRYPTO_malloc();
void CRYPTO_lock();

