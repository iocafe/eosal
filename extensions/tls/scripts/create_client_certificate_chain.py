# STEP D: Create client certificate chain. 
# RUN BY LOCAL NETWORK SERVER ADMIN
import os

CN = "myhome" # Common name of the certificate you want to create
certfile = "%s.crt" % CN 
chainfile = "%s-bundle.crt" % CN 

ROOTCN = "rootca" # Name of the CA root certificate used to sign the server sertificate
root_pubkey = "%s.crt" % ROOTCN

targetdir = "/coderoot/eosal/extensions/tls/keys-and-certs"
certfile = os.path.join(targetdir, certfile)
chainfile = os.path.join(targetdir, chainfile)
root_pubkey = os.path.join(targetdir, root_pubkey)

# Get device CSR obj (Certificate Signing Request)
with open(certfile, "rt") as file:  
    a = file.read()

with open(root_pubkey, "rt") as file:  
    b = file.read()

pub = a + b
open(chainfile,"wt").write(pub)
