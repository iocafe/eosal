@echo off
call set-ssl-env
set OPENSSL_CONF=%E_SSL_ROOT_CA_DIR%\openssl.cnf
echo Enter pass phrase "eosal1" when asked.
%E_SSL_PATH%\openssl genrsa -aes256 -out %E_SSL_ROOT_CA_DIR%\private\ca.key.pem 4096
