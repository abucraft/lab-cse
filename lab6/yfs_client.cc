// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define MIDDELIM '/'
#define LOGNODE 2
yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  lc = new lock_client(lock_dst);
  logcount=0;
  recovermod=true;
}

int yfs_client::logtransaction(int transcount,std::string type){
	int r=extent_protocol::OK;
	if(recovermod==false){
	std::string logbuf;
	lc->acquire(LOGNODE);
	if(ec->get(LOGNODE,logbuf)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(LOGNODE);
        	return r;
    	}
	logbuf+="Transaction ";
	logbuf+=i2n(transcount);
	logbuf+=' '+type+'\n';
	if(ec->put(LOGNODE,logbuf)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(LOGNODE);
        	return r;
    	}
	lc->release(LOGNODE);
	}
	return r;
} 

int yfs_client::logcreate(int transcount,inum inode){
	int r=extent_protocol::OK;
	if(recovermod==false){
	std::string logbuf;
	lc->acquire(LOGNODE);
	if(ec->get(LOGNODE,logbuf)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(LOGNODE);
        	return r;
    	}
	logbuf+="T ";
	logbuf+=i2n(transcount);
	logbuf+=' ';
	logbuf+="create ";
	logbuf+=i2n(inode);
	logbuf+='\n';
	if(ec->put(LOGNODE,logbuf)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(LOGNODE);
        	return r;
    	}
	lc->release(LOGNODE);
	}
	return r;
}

int yfs_client::logwrite(int transcount,inum inode,int oldsize,int offset,int contlength,int oldcontlength,std::string content){
	int r=extent_protocol::OK;
	if(recovermod==false){
	std::string logbuf;
	lc->acquire(LOGNODE);
	if(ec->get(LOGNODE,logbuf)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(LOGNODE);
        	return r;
    	}
	logbuf+="T ";
	logbuf+=i2n(transcount);
	logbuf+=' ';
	logbuf+="write ";
	logbuf+=i2n(inode);//inode
	logbuf+=' ';
	logbuf+=i2n(oldsize);//oldsize
	logbuf+=' ';
	logbuf+=i2n(offset);//offset
	logbuf+=' ';
	logbuf+=i2n(contlength);//content length
	logbuf+=' ';
	logbuf+=i2n(oldcontlength);//old content length
	if(oldcontlength!=0){
		logbuf+=' ';
		logbuf+=content;
	}
	logbuf+='\n';
	if(ec->put(LOGNODE,logbuf)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(LOGNODE);
        	return r;
    	}
	lc->release(LOGNODE);
	}
	return r;
}

int yfs_client::logremove(int transcount,inum parent,int oldsize,inum inoderemov,int type,std::string filename,int oldcontlength,std::string oldcontent){
	int r=extent_protocol::OK;
	if(recovermod==false){
	std::string logbuf;
	lc->acquire(LOGNODE);
	if(ec->get(LOGNODE,logbuf)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(LOGNODE);
        	return r;
    	}
	logbuf+="T ";
	logbuf+=i2n(transcount);
	logbuf+=' ';
	logbuf+=i2n(parent);
	logbuf+=' ';
	logbuf+=i2n(oldsize);
	logbuf+=' ';
	logbuf+=i2n(inoderemov);
	logbuf+=' ';
	logbuf+=i2n(type);
	logbuf+=' ';
	logbuf+=filename;
	logbuf+=' ';
	logbuf+=i2n(oldcontlength);
	logbuf+=' ';
	if(oldcontlength!=0){
		logbuf+=' ';
		logbuf+=oldcontent;
	}
	logbuf+='\n';
	if(ec->put(LOGNODE,logbuf)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(LOGNODE);
        	return r;
    	}
	lc->release(LOGNODE);
	}
	return r;
}

void yfs_client::recovery(){
	std::string logbuf;
	recovermod=true;
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string yfs_client::i2n(yfs_client::inum inum){
	std::stringstream st;
	std::string result;
	st<<inum;
	st>>result;
	return result;
}

std::vector<std::string> yfs_client::split(const std::string &ssource,char delim){
	size_t last=0;
	size_t index=ssource.find_first_of(delim,last);
	std::vector<std::string> result;
	while(index!=std::string::npos){
		result.push_back(ssource.substr(last,index-last));
		last=index+1;
		index=ssource.find_first_of(delim,last);
	}
	if(last<ssource.size()){
		result.push_back(ssource.substr(last,ssource.size()-last));
	}
	return result;
}

std::vector<yfs_client::dirent> yfs_client::str2dir(const std::string &ssource){
	std::vector<yfs_client::dirent> result;
	std::vector<std::string> strvec=split(ssource,MIDDELIM);
	if(strvec.size()%2!=0){
		std::cout<<"sth wrong";
	}
	std::cout<<strvec.size()<<'\n';
	for(int i=0;i<strvec.size();i+=2){
		yfs_client::dirent tmpdir;
		if(i+1==strvec.size()){
			break;
		}
		tmpdir.name=strvec[i];
		tmpdir.inum=n2i(strvec[i+1]);
		result.push_back(tmpdir);
	}
	return result;
}


std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

int 
yfs_client::symlink(const char*link, inum parent, const char*name, inum& ino_out){
	lc->acquire(parent);
	int transcount= logcount ;
	logcount++;
	int r;
	bool found=false;
	inum fresult=0;

	//write log before transaction
	if(logtransaction(transcount,"begin")!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}
	//write log OK


	if(lookup(parent,name,found,fresult)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	if(found){
		r = EXIST;
		lc->release(parent);
		return r;
	}
	std::string buf;
	if(ec->get(parent, buf) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	if(ec->create(extent_protocol::T_SYM,ino_out)!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}


	//after create inode write log
	if(logcreate(transcount,ino_out)!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}
	//write log OK


	int oldsize=buf.size();
	buf+=name;
	buf+=MIDDELIM;
	buf+=i2n(ino_out);
	buf+=MIDDELIM;


	//write log before write inode
	if(logwrite(transcount,parent,oldsize,oldsize,buf.size()-oldsize,0,"")!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}
	//write log OK


	if(ec->put(parent,buf)!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}
	std::string linkcontent(link);


	//write log before write inode
	if(logwrite(transcount,ino_out,0,0,strlen(link),0,"")!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}
	//write log OK


	lc->acquire(ino_out);
	if(ec->put(ino_out,linkcontent)!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		lc->release(ino_out);
		return r;
	}
	lc->release(ino_out);
	lc->release(parent);


	//write log after transaction
	if(logtransaction(transcount,"end")!=extent_protocol::OK){
		r = IOERR;
		return r;
	}
	//write log OK


	return r;
}


int 
yfs_client::readlink(inum ino, std::string&data){
	int r;
	r=ec->get(ino, data) ;
        return r;
}


int yfs_client::getsym(inum inum, syminfo &sin){
	int r = OK;

    printf("getsym %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    sin.atime = a.atime;
    sin.mtime = a.mtime;
    sin.ctime = a.ctime;
    sin.size = a.size;
    printf("getsim %016llx -> sz %llu\n", inum, sin.size);

release:
    return r;
}

bool
yfs_client::isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    } 
    return false;
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
	//std::cout<<"SETATTR\n";
	lc->acquire(ino);
	int transcount=logcount;
	logcount++;
	extent_protocol::attr a;
    	if (ec->getattr(ino, a) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(ino);
        	return r;
    	}


	//write log before transaction
	if(logtransaction(transcount,"begin")!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(ino);
        	return r;
    	}
	//write log OK


	std::string buf;
	const char *origin;
	if(ec->get(ino, buf) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(ino);
        	return r;
    	}
	int oldsize=buf.size();
	char *changed=new char[size];
	bzero(changed,size);
	origin=buf.c_str();
	memcpy(changed,origin,size<a.size?size:a.size);

	//write log before write inode
	std::string logcontbuf;
	int offset=0;
	int contlength=0;
	int oldcontlength=0;
	if(size<a.size){
		offset=size;
		oldcontlength=a.size-size;
		for(int i=size;i<a.size;i++){
			logcontbuf+=buf[i];
		}
	}
	else{
		offset=a.size;
		contlength=size-a.size;
	}
	if(logwrite(transcount,ino,oldsize,offset,contlength,oldcontlength,logcontbuf)!=extent_protocol::OK){
		r = IOERR;
		lc->release(ino);
		return r;
	}
	//write log OK


	std::string newbuf(changed);
	if(ec->put(ino,newbuf)!=extent_protocol::OK){
		r = IOERR;
		delete [] changed;
		lc->release(ino);
		return r;
	}
	delete [] changed;
	lc->release(ino);


	//write log after transaction
	if(logtransaction(transcount,"end")!=extent_protocol::OK){
		r = IOERR;
		return r;
	}
	//write log OK


    return r;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
	//std::cout<<"ON CREATE!\n"; 
	lc->acquire(parent);
	int transcount=logcount;
	logcount++;

	//write transaction to log
	if(logtransaction(transcount,"begin")!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}


	bool found=false;
	inum fresult=0;
	if(lookup(parent,name,found,fresult)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	if(found){
		r = EXIST;
		lc->release(parent);
		return r;
	}
	std::string buf;
	if(ec->get(parent, buf) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	if(ec->create(extent_protocol::T_FILE,ino_out)!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}

	//write log after create
	if(logcreate(transcount,ino_out)!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}
	
	int oldsize=buf.size();
	buf+=name;
	buf+=MIDDELIM;
	buf+=i2n(ino_out);
	buf+=MIDDELIM;
	

	//write log before write inode
	if(logwrite(transcount,parent,oldsize,oldsize,buf.size()-oldsize,0,"")!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}


	if(ec->put(parent,buf)!=extent_protocol::OK){
		r = IOERR;
	}
	lc->release(parent);

	//write transaction end
	if(logtransaction(transcount,"end")!=extent_protocol::OK){
		r = IOERR;
		return r;
	}
	return r;
}
int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
	//printf("\n\nMKDIR\n\n");
	//printf("%s\n\n",name);
	lc->acquire(parent);
	int transcount=logcount;
	logcount++;


	//write transaction
	if(logtransaction(transcount,"begin")!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}


	bool found=false;
	inum fresult=0;
	if(lookup(parent,name,found,fresult)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	if(found){
		r = EXIST;
		lc->release(parent);
		return r;
	}
	std::string buf;
	if(ec->get(parent, buf) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	if(ec->create(extent_protocol::T_DIR,ino_out)!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}

	//write log after create
	if(logcreate(transcount,ino_out)!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}


	int oldsize=buf.size();
	buf+=name;
	buf+=MIDDELIM;
	buf+=i2n(ino_out);
	buf+=MIDDELIM;
	

	//write log before write inode
	if(logwrite(transcount,parent,oldsize,oldsize,buf.size()-oldsize,0,"")!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}


	if(ec->put(parent,buf) != extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}
	lc->release(parent);

	//write transaction
	if(logtransaction(transcount,"end")!=extent_protocol::OK){
		r = IOERR;
		return r;
	}

	return r;
}


	

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
	//std::cout<<"ON LOOKUP!\n"; 
	found=false;
	std::string buf;
	if(ec->get(parent, buf) != extent_protocol::OK) {
        	r = IOERR;
        	return r;
    	}
	std::vector<dirent> dirs=str2dir(buf);
	for(int i=0;i<dirs.size();i++){
		if(dirs[i].name==name){
			found=true;
			ino_out=dirs[i].inum;
			return r;
		}
	}
    return r;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
	std::string buf;
	if (ec->get(dir, buf) != extent_protocol::OK) {
        	r = IOERR;
        	return r;
    	}
	std::vector<dirent> dirvec=str2dir(buf);
	for(int i=0;i<dirvec.size();i++){
		list.push_back(dirvec[i]);
	}
    return r;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
   int r = OK;

    /*
     * your lab2 code goes here.
     * note: read using ec->get().
     */     
	//std::cout<<"\nREAD\n\n\n\n";
	std::string buf;
	data = "";
	if (ec->get(ino, buf) != extent_protocol::OK) {
        	r = IOERR;
        	return r;
    	}
	if(off<buf.size())
		data=buf.substr(off,off+size<buf.size()?size:buf.size()-off);
	std::cout<<buf<<'\n';
    return r;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
	//std::cout<<"WRITE\n";
	lc->acquire(ino);
	int transcount=logcount;
	logcount++;

	//write transaction
	if(logtransaction(transcount,"begin")!=extent_protocol::OK){
		r = IOERR;
		lc->release(ino);
		return r;
	}

	std::string buf;
	if (ec->get(ino, buf) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(ino);
        	return r;
    	}
	int oldsize=buf.size();
	std::string oldcontbuf;
	int oldcontlength=0;
	int contlength=0;
	int offset=0;
	for(int i=off;i<oldsize;i++){
		oldcontbuf+=buf[i];
		oldcontlength++;
	}

	if(off+size>buf.size()){
		buf.resize(off+size,'\0');
	}
	buf.replace(off,size,data,size);
	if(off<=oldsize){
		bytes_written=size;
		offset=off;
		contlength=size;
	}
	else{
		bytes_written=size;
		offset=oldsize;
		contlength=off-oldsize+size;
	}

	//write log before write inode
	if(logwrite(transcount,ino,oldsize,offset,contlength,oldcontlength,oldcontbuf)!=extent_protocol::OK){
		r = IOERR;
		lc->release(ino);
		return r;
	}


	if (ec->put(ino, buf) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(ino);
        	return r;
    	}
	lc->release(ino);

	//write transaction
	if(logtransaction(transcount,"end")!=extent_protocol::OK){
		r = IOERR;
		return r;
	}


    return r;
}

int yfs_client::unlink(inum parent,const char *name)
{
    int r = OK;

    /*
     * your lab2 code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
	//printf("\n\nUNLINK\n");
	lc->acquire(parent);
	int transcount=logcount;
	logcount++;
	//write transaction
	if(logtransaction(transcount,"begin")!= extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}

	std::string oldcontent;
	std::string buf;
	if(ec->get(parent, buf) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	int oldsize=buf.size();
	int i;
	std::vector<dirent> dirvec=str2dir(buf);
	for(i=0;i<dirvec.size();i++){
		if(std::string(name)==dirvec[i].name)
			break;
	}
	
	if(i==dirvec.size()){
		lc->release(parent);
		return r;
	}

	//write log before remove
	extent_protocol::attr a;
	if(ec->getattr(dirvec[i].inum,a)!= extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	if(ec->get(dirvec[i].inum,oldcontent)!= extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	std::string filename(name);
	if(logremove(transcount,parent,oldsize,dirvec[i].inum,a.type,filename,oldcontent.size(),oldcontent)!= extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}

	inum inodetoremove=dirvec[i].inum;
	dirvec.erase(dirvec.begin()+i);
	buf.clear();
	for(int j=0;j<dirvec.size();j++){
		//printf("rest:%s\t%d",dirvec[i].name.c_str(),dirvec[i].inum);
		buf+=dirvec[j].name;
		buf+=MIDDELIM;
		buf+=i2n(dirvec[j].inum);
		buf+=MIDDELIM;
	}
	if(ec->put(parent, buf) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	//printf("\n\nremove:%s\t%d\n",dirvec[i].name.c_str(),dirvec[i].inum);
	if(ec->remove(inodetoremove)!=extent_protocol::OK) {
        	r = IOERR;
		lc->release(parent);
        	return r;
    	}
	lc->release(parent);

	//write transaction
	if(logtransaction(transcount,"end")!= extent_protocol::OK) {
        	r = IOERR;
        	return r;
    	}
    return r;
}


