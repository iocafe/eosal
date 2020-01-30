28.1.2020/pekka
Files in eosal/extensions/tls/keys-and-certs directory are created by Python scripts in eosal/extensions/tls/scripts.

rootca.crt: Root CA certificate. 
secret/rootca.key: Root CA private key.
myhome-certificate-request: Request for server (in local net) sertificate from root CA.
secret/myhome.key: Private key for server in local net.
myhome.crt: Local server certiticate.
myhome-bundle.crt: Certificate chain for clients in same IO network.

simply:
Server loads myhome.crt and myhome.key.
Client loads myhome-bundle.crt.
Now client can verify that server is secure, and these can connect. Client device name and password are to be matched after connecting.

