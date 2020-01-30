@echo off
call set-ssl-env
set OPENSSL_CONF=%E_SSL_ROOT_CA_DIR%\intermediate\openssl.cnf
cd %E_SSL_ROOT_CA_DIR%
echo Enter pass phrase "eosal3" when asked and type in client/server name "myprocess" as common name.
%E_SSL_PATH%\openssl req -config intermediate/openssl.cnf -key intermediate/private/www.example.com.key.pem -new -sha256 -out intermediate/csr/www.example.com.csr.pem
cd %E_SSL_SCRIPT_DIR%