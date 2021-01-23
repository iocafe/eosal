# STEP B: Create request to sign server certificate.
# RUN BY LOCAL NETWORK SERVER ADMIN
from OpenSSL import crypto, SSL
import os

CN = "myhome" # Common name of the certificate you want (if you leave this as is, matches to examples)
reqfile = "%s-certificate-request.crt" % CN 
privkey = "secret/%s.key" % CN

targetdir = "/coderoot/eosal/extensions/tls/keys-and-certs"
reqfile = os.path.join(targetdir, reqfile)
privkey = os.path.join(targetdir, privkey)

k = crypto.PKey()
k.generate_key(crypto.TYPE_RSA, 2048)

req = crypto.X509Req()
req.get_subject().C = "US" # Change you country here (2 characters max)
req.get_subject().ST = "GA" # I live in Georgia state
req.get_subject().L = "JohnsCreek" # City name
req.get_subject().O = "myhome" # Organization
req.get_subject().OU = "cafenet" # Organizational Unit, IO network name
req.get_subject().CN = CN
req.set_pubkey(k)
req.sign(k, 'sha256')
pub=crypto.dump_certificate_request(crypto.FILETYPE_PEM, req)
priv=crypto.dump_privatekey(crypto.FILETYPE_PEM, k)

if os.path.exists(reqfile):
    print("Certificate request " + reqfile + " exists, aborting. Delete the file to recreate.")
    exit()

open(reqfile,"wt").write(pub.decode("utf-8"))
open(privkey, "wt").write(priv.decode("utf-8") )
