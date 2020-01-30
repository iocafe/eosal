@echo off
call set-ssl-env
if not exist %E_SSL_SCRIPT_DIR% mkdir %E_SSL_SCRIPT_DIR%
cd %E_SSL_SCRIPT_DIR%
if not exist root mkdir root
cd root
if not exist ca mkdir ca
cd ca
if not exist intermediate mkdir intermediate
cd intermediate
if not exist certs mkdir certs
if not exist crl mkdir crl
if not exist csr mkdir csr
if not exist newcerts mkdir newcerts
if not exist private mkdir private
if not exist index.txt echo. 2>index.txt
if not exist serial echo 1000 >serial
if not exist crlnumber echo 1000 >crlnumber
cd %E_SSL_SCRIPT_DIR%
echo Done
