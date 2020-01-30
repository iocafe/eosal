@echo off
call set-ssl-env
set OPENSSL_CONF=%E_SSL_ROOT_CA_DIR%\openssl.cnf
cd %E_SSL_ROOT_CA_DIR%
echo Enter pass phrase "eosal1" when asked.
%E_SSL_PATH%\openssl ca -config %E_SSL_ROOT_CA_DIR%\openssl.cnf -extensions v3_intermediate_ca -days 3650 -notext -md sha256 -in intermediate/csr/intermediate.csr.pem -out %E_SSL_ROOT_CA_DIR%\intermediate\certs\intermediate.cert.pem
cd %E_SSL_SCRIPT_DIR%