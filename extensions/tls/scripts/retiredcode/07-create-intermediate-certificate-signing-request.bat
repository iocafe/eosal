@echo off
call set-ssl-env
set OPENSSL_CONF=%E_SSL_ROOT_CA_DIR%\intermediate\openssl.cnf
cd %E_SSL_ROOT_CA_DIR%
echo Enter pass phrase "eosal2" when asked, type "intca" as common name for intermediate certificate authority.
%E_SSL_PATH%\openssl req -config %E_SSL_ROOT_CA_DIR%\intermediate\openssl.cnf -new -sha256 -key intermediate/private/intermediate.key.pem -out %E_SSL_ROOT_CA_DIR%\intermediate\csr\intermediate.csr.pem
cd %E_SSL_SCRIPT_DIR%