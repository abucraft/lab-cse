#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include "lock_protocol.h"
#include "lock_client.h"
#include <vector>


class yfs_client {
  extent_client *ec;
  lock_client *lc;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct syminfo{
  	unsigned long long size;
  	unsigned long atime;
	unsigned long mtime;
	unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  bool recovermod;
  int logcount;
  static std::string filename(inum);
  static inum n2i(std::string);
  static std::string i2n(inum inum);
  static std::vector<std::string> split(const std::string &ssource,char delim);
  static std::vector<yfs_client::dirent> str2dir(const std::string &ssource);
  int logwrite(int transcount,inum inode,int oldsize,int offset,int contlength,int oldcontlength,std::string content);
  int logcreate(int transcount,inum inode);
  int logremove(int transcount,inum parent,int oldsize,inum inoderemov,int type,std::string filename,int oldcontlength,std::string oldcontent);
  int logtransaction(int transcount,std::string type);
  void recovery();
 public:
  yfs_client(std::string, std::string);
  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);

  int setattr(inum, size_t);
  int lookup(inum, const char *, bool &, inum &);
  int create(inum, const char *, mode_t, inum &);
  int readdir(inum, std::list<dirent> &);
  int write(inum, size_t, off_t, const char *, size_t &);
  int read(inum, size_t, off_t, std::string &);
  int unlink(inum,const char *);
  int mkdir(inum , const char *, mode_t , inum &);
  
  /** you may need to add symbolic link related methods here.*/
  int symlink(const char*, inum, const char*, inum&);
  int readlink(inum, std::string&);
  int getsym(inum, syminfo &);
};

#endif 