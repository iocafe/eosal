@echo off
call set-ssl-env
set OPENSSL_CONF=%E_SSL_ROOT_CA_DIR%\openssl.cnf
cd %E_SSL_ROOT_CA_DIR%
echo Enter pass phrase "eosal1" when asked. Press enter to use defaults for all other information.
%E_SSL_PATH%\openssl req -config %E_SSL_ROOT_CA_DIR%\openssl.cnf -key %E_SSL_ROOT_CA_DIR%\private\ca.key.pem -new -x509 -days 36500 -sha256 -extensions v3_ca -out %E_SSL_ROOT_CA_DIR%\certs\ca.cert.pem
cd %E_SSL_SCRIPT_DIR%
echo Private key for root cetrificate authority: ssl-scripts/root/ca/private/ca.key.pem.
echo Self signed root sertificate: ssl-scripts/root/ca/certs/ca.cert.pem.