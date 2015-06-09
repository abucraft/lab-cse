// RPC stubs for clients to talk to lock_server

#include "lock_client.h"
#include "rpc.h"
#include <arpa/inet.h>

#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
void lock_client::heartbeat(){
  int r;
  while(true){
  lock_protocol::status ret = cl->call(lock_protocol::heartbeat, cl->id(),r);
  VERIFY (ret == lock_protocol::OK);
  usleep(10000);
  }
}

void* lock_client::thread(void* cl){
  ((lock_client*)cl)->heartbeat();
}

lock_client::lock_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() < 0) {
    printf("lock_client: call bind\n");
    return;
  }
  pthread_create(&heartbeatid,NULL,thread,this);
}

lock_client::~lock_client(){
}


int
lock_client::stat(lock_protocol::lockid_t lid)
{
  int r;
  lock_protocol::status ret = cl->call(lock_protocol::stat, cl->id(), lid, r);
  VERIFY (ret == lock_protocol::OK);
  return r;
}

lock_protocol::status
lock_client::acquire(lock_protocol::lockid_t lid)
{
	// Your lab4 code goes here
  int r;
  lock_protocol::status ret = cl->call(lock_protocol::acquire, cl->id(), lid, r);
  VERIFY (ret == lock_protocol::OK);
  return r;
}

lock_protocol::status
lock_client::release(lock_protocol::lockid_t lid)
{
	// Your lab4 code goes here
  int r;
  lock_protocol::status ret = cl->call(lock_protocol::release, cl->id(), lid, r);
  VERIFY (ret == lock_protocol::OK);
  return r;
}

