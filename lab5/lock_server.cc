// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

static pthread_mutex_t mtx=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
/*void lock_server::clientcheck(){
	while(true){
		sleep(1);
		std::map<int,int>::iterator clientit=clients.begin();
		for(;clientit!=clients.end();clientit++){
			(clientit->second)++;
			if((clientit->second)>3){
				pthread_mutex_lock(&mtx);
				std::map<lock_inst,int>::iterator lockit=locks.begin();
				for(;lockit!=locks.end();lockit++){
					if((lockit->first).clt_id==(clientit->first)){
						(lockit->second)=FREE;
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
  pthread_mutex_lock(&mtx);
  clients[clt] = 0;
  pthread_mutex_unlock(&mtx);
  r = nacquire;
  return ret;
}*/


lock_server::lock_server():
  nacquire (0)
{
	//pthread_create(&checkthreadid,NULL,thread,this);
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
  timeval now;
  timespec outtime;
	// Your lab4 code goes here
  pthread_mutex_lock(&mtx);
  r = nacquire;
  if(!locks.count(lid)){
	locks[lid]=LOCKED;
	lock_clocks[lid]=clock();
	pthread_mutex_unlock(&mtx);
	return ret;	
  }else{
	while(locks[lid]==LOCKED&&((double)(clock()-lock_clocks[lid])/(double)CLOCKS_PER_SEC<0.001)){
		gettimeofday(&now, NULL);
		outtime.tv_sec = now.tv_sec;
      		outtime.tv_nsec = now.tv_usec*1000+5000000;
		if(outtime.tv_nsec>=1000000000){
			outtime.tv_nsec-=1000000000;
			outtime.tv_sec+=1;
		}
		pthread_cond_timedwait(&cond,&mtx,&outtime);
	}
	locks[lid]=LOCKED;
	lock_clocks[lid]=clock();
	pthread_mutex_unlock(&mtx);
	return ret;
  }
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
	// Your lab4 code goes here
  pthread_mutex_lock(&mtx);
  r = nacquire;
  locks[lid]=FREE;
  pthread_mutex_unlock(&mtx);
  pthread_cond_broadcast(&cond);
  return ret;
}
