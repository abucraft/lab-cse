// this is the lock server
// the lock client has a similar interface

#ifndef lock_server_h
#define lock_server_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
#include "rpc.h"
#include <map>
#define FREE 0
#define LOCKED 1
class lock_server {
 protected:
	struct lock_stat{
		int clt_id;
		int stat;
	};
  int nacquire;
  std::map<lock_protocol::lockid_t,lock_stat> locks;
  std::map<int,int> clients;
  pthread_t checkthreadid;
  static void* thread(void*);
  void clientcheck();
 public:
  lock_server();
  ~lock_server();
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status heartbeat(int clt, int &);
};

#endif 







