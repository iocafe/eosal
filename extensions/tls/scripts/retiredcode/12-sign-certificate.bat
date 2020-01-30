@echo off
call set-ssl-env
set OPENSSL_CONF=%E_SSL_ROOT_CA_DIR%\intermediate\openssl.cnf
cd %E_SSL_ROOT_CA_DIR%
echo Enter pass phrase "eosal2" when asked.
%E_SSL_PATH%\openssl ca -config intermediate/openssl.cnf -extensions server_cert -days 375 -notext -md sha256 -in intermediate/csr/www.example.com.csr.pem -out intermediate/certs/www.example.com.cert.pem
cd %E_SSL_SCRIPT_DIR%