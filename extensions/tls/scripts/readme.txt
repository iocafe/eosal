notes 29.1.2020/pekka
This is short and simple:
- A top-level CA runs a cloud server.
- A server in local net requests top-level CA for certificate and received it.

Chain of trust can be as complex as needed.

-----------------------------------------------------------------------------------------------------------------------------------------------------------
A. RUN BY ROOT CERTIFICATE AUTHORITY
This generates a self-signed root CA certificate, the top level of the trust.  
- If you are the top level of the trust and use your own root, only devices that you certify (or authenticate others to certify by issuing them certificate)  can connect your IO network. 
- You may choose not to be the top level of the network but instead request certificate intermediate to certify your devices. This is useful when the cloud server is operated by someone else and you want your devices to connect to your network through the cloud server.

1. *** make sure that pyopenssl (OpenSSL for python) is installed 
sudo python3 -m pip install pyopenssl

2. *** python scripts are in this directory
cd /coderoot/eosal/extensions/tls/scripts

3. Modify file "create-root-CA-certificate.py" to have your information
gedit create-root-CA-certificate.py

4. Create the CA certificate
python3 create-root-CA-certificate.py
=> The private key is created in /coderoot/eosal/extensions/tls/keys-and-certs/secret/rootca.key. This is the SECRET, do not give this to anyone.
=> The root CA certificate is created in /coderoot/eosal/extensions/tls/keys-and-certs/rootca.crt. This can be shared freely.

-----------------------------------------------------------------------------------------------------------------------------------------------------------
B. RUN BY LOCAL NETWORK SERVER ADMIN
To certify a server in the local network you need to create a certificate request for it. This also creates a private key for the server on the local net.
This can be run by a separate person at a different location as root CA certificate was prepared. The certificate request must be emailed to root CA and ready certificate back. Steps 1 and 2 are the same as above.

3. Modify file "create-certificate-request.py" to have your information
gedit create-certificate-request.py

4. Create the certificate request
python3 create-certificate-request.py
=> The private key is created in /coderoot/eosal/extensions/tls/keys-and-certs/secret/myhome.key. This is the SECRET, do not give this to anyone.
=> The certificate request is created in /coderoot/eosal/extensions/tls/keys-and-certs/myhome-certificate-request.crt. This needs to be sent to root CA.

-----------------------------------------------------------------------------------------------------------------------------------------------------------
C. RUN BY ROOT CERTIFICATE AUTHORITY
This generates a certificate for server in local network Steps 1 and 2 are the same as above.

3. Modify file "create-server-certificate.py" to have the right information (if needed)
gedit create-server-certificate.py

4. Create the server certificate
python3 create-server-certificate.py
=> The certificate is created in /coderoot/eosal/extensions/tls/keys-and-certs/myhome.crt. This needs to be sent to the server admin.

-----------------------------------------------------------------------------------------------------------------------------------------------------------
D. RUN BY LOCAL NETWORK SERVER ADMIN
Create a client certificate chain. The certificate chain file (or bundle) simply a file containing both server certificate and root CA
certificate:  myhome-bundle.crt: rootca.crt appended to the end of myhome.crt -> certificate chain. This is public information used by the client to verify the server. Since local server and cloud server are both based on root CA cert, both of those are accepted by IO device when "myhome-bundle.crt" is chosen to be trusted by the IO device.

cd /coderoot/eosal/extensions/tls/scripts
python3 create-client-certificate-chain.py