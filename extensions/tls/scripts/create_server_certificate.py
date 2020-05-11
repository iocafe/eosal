# STEP C: Create a server vertificate signed by root CA.
# RUN BY ROOT CERTIFICATE AUTHORITY
from OpenSSL import crypto, SSL
import os
import random

CN = "myhome" # Common name of the certificate you want to create
reqfile = "%s-certificate-request.crt" % CN 
certfile = "%s.crt" % CN 

ROOTCN = "rootca" # Name of the CA root certificate used to sign the server sertificate
root_pubkey = "%s.crt" % ROOTCN
root_privkey = "secret/%s.key" % ROOTCN

targetdir = "/coderoot/eosal/extensions/tls/keys-and-certs"
reqfile = os.path.join(targetdir, reqfile)
certfile = os.path.join(targetdir, certfile)
root_pubkey = os.path.join(targetdir, root_pubkey)
root_privkey = os.path.join(targetdir, root_privkey)

# Get device CSR obj (Certificate Signing Request)
with open(reqfile) as file:  
    req = crypto.load_certificate_request(crypto.FILETYPE_PEM, file.read())

# Get CA root certificate and CA private key
with open(root_privkey) as file:  
    k = crypto.load_privatekey(crypto.FILETYPE_PEM, file.read())
with open(root_pubkey, 'rb') as file:  
    root_cert = crypto.load_certificate(crypto.FILETYPE_PEM, file.read())

serialnumber=random.getrandbits(64)

cert = crypto.X509()
cert.set_serial_number(serialnumber)
cert.gmtime_adj_notBefore(0)
cert.gmtime_adj_notAfter(31536000) # 315360000 is in seconds.
cert.set_issuer(root_cert.get_subject())
cert.set_subject(req.get_subject())
cert.set_pubkey(req.get_pubkey())

cert.sign(k, 'sha256')

if os.path.exists(certfile):
    print("Certificate " + certfile + " exists, aborting. Delete the file to recreate.")
    exit()

pub=crypto.dump_certificate(crypto.FILETYPE_PEM, cert)
open(certfile,"wt").write(pub.decode("utf-8"))
