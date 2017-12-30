# dhcp

Client:

./clt -n -1 [for apply for a ip address and enter normally renew period (t1 expires and successful lease renew).] 

./clt -n -2 [for apply for a ip address and enter normally renew period (t2 expires).]

./clt -u <Lease Time> [for apply for a ip address and enter normally renew period (t2 expires).]
 
 ./clt -r <serverIP> [for release a ip addr.]
 
 ./clt -inform <serverIP> [for inform.]
 
 Server:

./svr [For normal server.]

./svr -n [When client applies to renew its lease, send NAK]
