# Create request to sign cloud server certificate.
from OpenSSL import crypto, SSL
from os.path import join
import os

CN = "cloud-certificate-request" # Common name of the certificate you want (if you leave this as is, matches to examples)
reqfile = "%s.crt" % CN 
privkey = "secret/%s.key" % CN

targetdir = "/coderoot/eosal/extensions/tls/keys-and-certs"
reqfile = join(targetdir, reqfile)
privkey = join(targetdir, privkey)

k = crypto.PKey()
k.generate_key(crypto.TYPE_RSA, 2048)
# serialnumber=random.getrandbits(64)

req = crypto.X509Req()
req.get_subject().C = "US" # Change you country here (2 characters max)
req.get_subject().ST = "GA" # I live in Georgia state
req.get_subject().L = "Cloud" # City name
req.get_subject().O = "cloud-public" # Organization
req.get_subject().OU = "transit" # Organizational Unit
req.get_subject().CN = CN
req.set_pubkey(k)
req.sign(k, 'sha256')
pub=crypto.dump_certificate_request(crypto.FILETYPE_PEM, req)
priv=crypto.dump_privatekey(crypto.FILETYPE_PEM, k)

if os.path.exists(reqfile):
    print("Certificate file " + reqfile + " exists, aborting. Delete the file to recreate.")
    exit()

if os.path.exists(reqfile):
    print("Private key file " + privkey + " exists, aborting. Delete the file to recreate.")
    exit()

open(reqfile,"wt").write(pub.decode("utf-8"))
open(privkey, "wt").write(priv.decode("utf-8") )
