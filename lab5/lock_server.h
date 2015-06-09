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
	/*struct lock_inst{
		lock_protocol::lockid_t lock_id;
		int clt_id;
		bool operator <(const lock_inst& i) const{
			return lock_id+clt_id<i.lock_id+i.clt_id;
		}
	};*/
  int nacquire;
  std::map<lock_protocol::lockid_t,int> locks;
  std::map<lock_protocol::lockid_t,clock_t> lock_clocks;
  //std::map<int,int> clients;
  //pthread_t checkthreadid;
  //static void* thread(void*);
  //void clientcheck();
 public:
  lock_server();
  ~lock_server();
  lock_protocol::status stat(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status acquire(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status release(int clt, lock_protocol::lockid_t lid, int &);
  lock_protocol::status heartbeat(int clt, int &);
};

#endif 







