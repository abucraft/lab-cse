// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

static pthread_mutex_t mtx=PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond=PTHREAD_COND_INITIALIZER;

lock_server::lock_server():
  nacquire (0)
{
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
	locks[lid]=LOCKED;
	pthread_mutex_unlock(&mtx);
	return ret;	
  }else{
	while(locks[lid]==LOCKED){
		pthread_cond_wait(&cond,&mtx);
	}
	locks[lid]=LOCKED;
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
