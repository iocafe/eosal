notes 29.1.2020/pekka

-----------------------------------------------------------------------------------------------------------------------------------------------------------
This generates self signed root CA certificate. This means the top level of the trust.  
- If you are top level of the trust and use your own root, only devices which you certify (or authenticate otherts to certify by issuing them certificate) 
  can connect your IO network. 
- You may choose not to be top level of the network, but instead request certificate intermediate to certify your devices. This is useful when cloud
  server is operated by someone else and you want your devices to connect to your network through the cloud server.

1. *** make sure that pyopenssl (OpenSSL for python) is installed 
sudo python3 -m pip install pyopenssl

2. *** python scripts are in this directory
cd /coderoot/eosal/extensions/tls/scripts

3. Modify file "create-root-CA-certificate.py" to have your information
gedit create-root-CA-certificate.py

4. Run the "create-root-CA-certificate.py"
python3 create-root-CA-certificate.py
=> The private key is created in /coderoot/eosal/extensions/tls/keys-and-certs/secret/rootca.key. This is the SECRET, do not give this to anyone.
=> The root CA certificate is created in /coderoot/eosal/extensions/tls/keys-and-certs/rootca.crt. This can be shared freely.


