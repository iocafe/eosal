8.1.2020/pekka

alice.crt: Certificate for Alice in PEM format. This is public information.
alice.key: Key for Alice in PEM format. This is seacret and must be known only to Alice.
bob.crt, bob.key, carol.crt and caarol.key: Same as alice.*, but for Bob and Carol (peers of Alice).
Server loads for example alice.crt and alice.key to identify itself to clients. 
ca.crt: Self signed root certificate, PEM format. This is public information.

bob-bundle.crt: ca.crt appended to end of bob.crt -> certificate chain. This is public information used by client to verify server. Since all alice.crt, bob.crt and carol.crt are signed using same ca.crt, all of them will be accepted when "bob-bundle.crt" is chosen to be trusted by client.

Summary:
Server loads for example alice.crt and alice.key.
Client loads bob-bundle.crt.
Now these can connect

