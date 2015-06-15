// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

static pthread_mutex_t mtx=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
void lock_server::clientcheck(){
	while(true){
		usleep(10000);
		std::map<int,int>::iterator clientit=clients.begin();
		for(;clientit!=clients.end();clientit++){
			(clientit->second)+=10000;
			if((clientit->second)>300000){
				pthread_mutex_lock(&mtx);
				std::map<lock_protocol::lockid_t,lock_stat>::iterator lockit=locks.begin();
				for(;lockit!=locks.end();lockit++){
					if((lockit->second).clt_id==(clientit->first)){
						(lockit->second).stat=FREE;
					}
				}
				pthread_mutex_unlock(&mtx);
				pthread_cond_broadcast(&cond);
			}
		}
	}
}
void* lock_server::thread(void* sr){
  ((lock_server*)sr)->clientcheck();
}

lock_protocol::status 
lock_server::heartbeat(int clt, int &r){
  lock_protocol::status ret = lock_protocol::OK;
  clients[clt] = 0;
  r = nacquire;
  return ret;
}


lock_server::lock_server():
  nacquire (0)
{
	pthread_create(&checkthreadid,NULL,thread,this);
}

lock_server::~lock_server(){
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab4 code goes here
  pthread_mutex_lock(&mtx);
  r = nacquire;
  if(!locks.count(lid)){
	locks[lid].stat=LOCKED;
	locks[lid].clt_id=clt;
  }else{
	while(locks[lid].stat==LOCKED){
		pthread_cond_wait(&cond,&mtx);
	}
	locks[lid].stat=LOCKED;
	locks[lid].clt_id=clt;
  }
  pthread_mutex_unlock(&mtx);
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab4 code goes here
  pthread_mutex_lock(&mtx);
  r = nacquire;
  locks[lid].stat=FREE;
  locks[lid].clt_id=0;
  pthread_mutex_unlock(&mtx);
  pthread_cond_broadcast(&cond);
  return ret;
}
