@echo off
call set-ssl-env
set OPENSSL_CONF=%E_SSL_ROOT_CA_DIR%\intermediate\openssl.cnf
%E_SSL_PATH%\openssl x509 -noout -text -in %E_SSL_ROOT_CA_DIR%/intermediate/certs/intermediate.cert.pem
%E_SSL_PATH%\openssl verify -CAfile %E_SSL_ROOT_CA_DIR%/certs/ca.cert.pem %E_SSL_ROOT_CA_DIR%/intermediate/certs/intermediate.cert.pem
