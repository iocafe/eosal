# STEP A: Create a self signed root ca cert.
# RUN BY ROOT CERTIFICATE AUTHORITY
from OpenSSL import crypto, SSL
import os
import random

CN = "rootca" # Common name of the certificate you want (if you leave this as is, matches to examples)
pubkey = "%s.crt" % CN 
privkey = "secret/%s.key" % CN

targetdir = "/coderoot/eosal/extensions/tls/keys-and-certs"
pubkey = os.path.join(targetdir, pubkey)
privkey = os.path.join(targetdir, privkey)

k = crypto.PKey()
k.generate_key(crypto.TYPE_RSA, 2048)
serialnumber=random.getrandbits(64)

# create a self-signed cert
cert = crypto.X509()
cert.get_subject().C = "US" # Change you country here (2 characters max)
cert.get_subject().ST = "GA" # I live in Georgia state
cert.get_subject().L = "Artemis" # City name
cert.get_subject().O = "iocafe" # Organization
cert.get_subject().OU = "cloudnet" # Organizational Unit, IO network name
cert.get_subject().CN = CN
cert.set_serial_number(serialnumber)
cert.gmtime_adj_notBefore(0)
cert.gmtime_adj_notAfter(31536000) # 315360000 is in seconds.
cert.set_issuer(cert.get_subject())
cert.set_pubkey(k)
cert.sign(k, 'sha256')
pub=crypto.dump_certificate(crypto.FILETYPE_PEM, cert)
priv=crypto.dump_privatekey(crypto.FILETYPE_PEM, k)

if os.path.exists(pubkey):
    print("Root CA certificate " + pubkey + " exists, aborting. Delete the file to recreate.")
    exit()

open(pubkey,"wt").write(pub.decode("utf-8"))
open(privkey, "wt").write(priv.decode("utf-8") )
