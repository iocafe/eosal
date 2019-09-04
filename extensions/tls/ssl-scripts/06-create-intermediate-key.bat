@echo off
call set-ssl-env
set OPENSSL_CONF=%E_SSL_ROOT_CA_DIR%\intermediate\openssl.cnf
echo Enter pass phrase "eosal2" when asked.
%E_SSL_PATH%\openssl genrsa -aes256 -out %E_SSL_ROOT_CA_DIR%\intermediate\private\intermediate.key.pem 4096
