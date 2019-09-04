@echo off
call set-ssl-env
cd %E_SSL_ROOT_CA_DIR%
echo Enter pass phrase "eosal3" when asked.
%E_SSL_PATH%\openssl genrsa -aes256 -out intermediate/private/www.example.com.key.pem 2048
cd %E_SSL_SCRIPT_DIR%