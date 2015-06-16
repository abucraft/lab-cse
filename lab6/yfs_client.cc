// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <fstream>
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
  curversion=0;
  recovermod=true;
  recovery(0);
}


void yfs_client::logcreate(inum parent, const char *name, mode_t mode){
	if(recovermod==false){
		lc->acquire(0);
		std::ofstream ofs("log",std::ios::app);
		if(!ofs){
			printf("Open Log Error\n\n");
			return;
		}
		unsigned int length;
		std::string buf;
		buf+="create ";
		length=sizeof(name);
		buf+=i2n(parent);
		buf+=' ';
		buf+=i2n(length);
		buf+=' ';
		for(size_t i=0;i<length;i++){
			buf+=name[i];
		}
		buf+=' ';
		buf+=i2n(mode);
		buf+='\n';
		ofs<<buf;
		lc->release(0);
	}
}


void yfs_client::logmkdir(inum parent, const char *name, mode_t mode){
	if(recovermod==false){
		lc->acquire(0);
		std::ofstream ofs("log",std::ios::app);
		if(!ofs){
			printf("Open Log Error\n\n");
			return;
		}
		unsigned int length;
		std::string buf;
		buf+="mkdir ";
		length=strlen(name);
		buf+=i2n(parent);
		buf+=' ';
		buf+=i2n(length);
		buf+=' ';
		for(size_t i=0;i<length;i++){
			buf+=name[i];
		}
		buf+=' ';
		buf+=i2n(mode);
		buf+='\n';
		ofs<<buf;
		lc->release(0);
	}
}

void yfs_client::logwrite(inum ino, size_t size, off_t off, const char *data){
	if(recovermod==false){
		lc->acquire(0);
		std::ofstream ofs("log",std::ios::app);
		if(!ofs){
			printf("Open Log Error\n\n");
			return;
		}
		std::string buf;
		buf+="write ";
		buf+=i2n(ino);
		buf+=' ';
		buf+=i2n(size);
		buf+=' ';
		buf+=i2n(off);
		buf+=' ';
		for(size_t i=0;i<size;i++){
			buf+=data[i];
		}
		buf+='\n';
		ofs<<buf;
		lc->release(0);
	}
}

void yfs_client::logunlink(inum parent,const char* name){
	if(recovermod==false){
		lc->acquire(0);
		std::ofstream ofs("log",std::ios::app);
		if(!ofs){
			printf("Open Log Error\n\n");
			return;
		}
		std::string buf;
		unsigned int length;
		buf+="unlink ";
		length=sizeof(name);
		buf+=i2n(parent);
		buf+=' ';
		buf+=i2n(length);
		buf+=' ';
		for(size_t i=0;i<length;i++){
			buf+=name[i];
		}
		buf+='\n';
		ofs<<buf;
		lc->release(0);
	}
}

void yfs_client::logsymlink(const char*link,inum parent,const char*name){
	if(recovermod==false){
		lc->acquire(0);
		std::ofstream ofs("log",std::ios::app);
		if(!ofs){
			printf("Open Log Error\n\n");
			return;
		}
		unsigned int length;
		std::string buf;
		buf+="symlink ";
		length=sizeof(link);
		buf+=i2n(length);
		buf+=' ';
		buf+=link;
		buf+=' ';
		buf+=i2n(parent);
		buf+=' ';
		length=strlen(name);
		buf+=i2n(length);
		buf+=' ';
		buf+=name;
		buf+='\n';
		ofs<<buf;
		lc->release(0);
	}
}

void yfs_client::logsetattr(inum ino,int size){
	if(recovermod==false){
		lc->acquire(0);
		std::ofstream ofs("log",std::ios::app);
		if(!ofs){
			printf("Open Log Error\n\n");
			return;
		}
		std::string buf;
		buf+="setattr ";
		buf+=i2n(ino);
		buf+=' ';
		buf+=i2n(size);
		buf+='\n';
		ofs<<buf;
		lc->release(0);
	}
}

void yfs_client::recovery(unsigned int pos){
	std::ifstream ifs("log");
	recovermod=true;
	if(!ifs){
		recovermod=false;
		return;
	}
	ec->clear(0);
	unsigned int cline=0;
	while(!ifs.eof()){
		std::string buf;
		ifs>>buf;
		printf("\n\nrecovery...\n\n");
		if(buf=="setattr")
		{
			inum inode;
			ifs>>inode;
			int size;
			ifs>>size;
			setattr(inode,size);
		}
		if(buf=="write"){
			inum inode;
			size_t size;
			off_t off;
			size_t size_out=0;
			ifs>>inode;
			ifs>>size;
			ifs>>off;
			char space;
			std::string data;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				data+=space;
			}
			write(inode,size,off,data.c_str(),size_out);
			printf("\n\nredo write:%d %d %d %d\n\n",inode,size,off,size_out);
		}
		if(buf=="create"){
			inum parent,ino_out;
			size_t size;
			int mode;
			char space;
			std::string name;
			ifs>>parent;
			ifs>>size;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
			ifs>>mode;
			create(parent,name.c_str(),mode,ino_out);
			printf("\n\nredo create:%d %s %d\n\n",parent,name.c_str(),ino_out);
		}
		if(buf=="unlink"){
			inum parent;
			size_t size;
			std::string name;
			char space;
			ifs>>parent;
			ifs>>size;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
			printf("\n\nredo unlink:%d %s\n\n",parent,name.c_str());
			unlink(parent,name.c_str());
		}
		if(buf=="symlink"){
			inum parent,ino_out;
			size_t size;
			std::string link,name;
			char space;
			ifs>>size;
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				link+=space;
			}
			ifs.get(space);
			ifs>>parent;
			ifs>>size;
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
			symlink(link.c_str(),parent,name.c_str(),ino_out);
		}
		if(buf=="mkdir"){
			inum parent,ino_out;
			size_t size;
			int mode;
			char space;
			std::string name;
			ifs>>parent;
			ifs>>size;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
			ifs>>mode;
			printf("\n\nredo mkdir:%d %s\n\n",parent,name.c_str());
			mkdir(parent,name.c_str(),mode,ino_out);
		}
		if(buf=="version"){
			ifs>>curversion;
		}
		cline++;
		printf("\n\nrecover line %d\n",cline);
		if(cline==pos)
			break;
	}
	recovermod=false;
}

void yfs_client::commit(){
	lc->acquire(0);
	std::ofstream ofs("log",std::ios::app);
	if(!ofs){
		printf("Open Log Error\n\n");
		return;
	}
	std::string buf;
	buf+="version ";
	buf+=i2n(curversion);
	buf+='\n';
	ofs<<buf;
	curversion++;
	lc->release(0);
}

void yfs_client::preversion(){
	recovermod=true;
	if(curversion==0)
		return;
	std::ifstream ifs("log");
	if(!ifs){
		printf("Open Log Error\n\n");
		return;
	}
	printf("\n\npreversion...\n\n");
	unsigned int pos=0;
	unsigned int line=0;
	while(!ifs.eof()){
		std::string head;
		line++;
		ifs>>head;
		if(head=="version"){
			unsigned int version;
			ifs>>version;
			if(version==curversion-1){
				pos=line;
			}
		}
		if(head=="setattr"){
			inum inode;
			ifs>>inode;
			int size;
			ifs>>size;
		}
		if(head=="write"){
			inum inode;
			size_t size;
			off_t off;
			size_t size_out;
			ifs>>inode;
			ifs>>size;
			ifs>>off;
			char space;
			std::string data;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				data+=space;
			}
		}
		if(head=="create"){
			inum parent,ino_out;
			size_t size;
			int mode;
			char space;
			std::string name;
			ifs>>parent;
			ifs>>size;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
			ifs>>mode;
		}
		if(head=="unlink"){
			inum parent;
			size_t size;
			std::string name;
			char space;
			ifs>>parent;
			ifs>>size;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
		}
		if(head=="symlink"){
			inum parent,ino_out;
			size_t size;
			std::string link,name;
			char space;
			ifs>>size;
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				link+=space;
			}
			ifs.get(space);
			ifs>>parent;
			ifs>>size;
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
		}
		if(head=="mkdir"){
			inum parent,ino_out;
			size_t size;
			int mode;
			char space;
			std::string name;
			ifs>>parent;
			ifs>>size;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
			ifs>>mode;
		}
	}
	printf("\n\nturn to pos:%d,curversion:%d\n\n",pos,curversion);
	if(pos==0)
		return;
	recovery(pos);
}

void yfs_client::nextversion(){
	recovermod=true;
	std::ifstream ifs("log");
	if(!ifs){
		printf("Open Log Error\n\n");
		return;
	}
	unsigned int pos=0;
	unsigned int line=0;
	printf("\n\nnextversion...\n\n");
	while(!ifs.eof()){
		std::string head;
		line++;
		ifs>>head;
		if(head=="version"){
			unsigned int version;
			ifs>>version;
			if(version==curversion+1){
				pos=line;
			}
		}
		if(head=="setattr"){
			inum inode;
			ifs>>inode;
			int size;
			ifs>>size;
		}
		if(head=="write"){
			inum inode;
			size_t size;
			off_t off;
			size_t size_out;
			ifs>>inode;
			ifs>>size;
			ifs>>off;
			char space;
			std::string data;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				data+=space;
			}
		}
		if(head=="create"){
			inum parent,ino_out;
			size_t size;
			int mode;
			char space;
			std::string name;
			ifs>>parent;
			ifs>>size;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
			ifs>>mode;
		}
		if(head=="unlink"){
			inum parent;
			size_t size;
			std::string name;
			char space;
			ifs>>parent;
			ifs>>size;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
		}
		if(head=="symlink"){
			inum parent,ino_out;
			size_t size;
			std::string link,name;
			char space;
			ifs>>size;
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				link+=space;
			}
			ifs.get(space);
			ifs>>parent;
			ifs>>size;
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
		}
		if(head=="mkdir"){
			inum parent,ino_out;
			size_t size;
			int mode;
			char space;
			std::string name;
			ifs>>parent;
			ifs>>size;
			ifs.get(space);
			for(size_t i=0;i<size;i++){
				ifs.get(space);
				name+=space;
			}
			ifs>>mode;
		}
	}
	if(pos==0){
		return;
	}
	recovery(pos);
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
	int r;
	bool found=false;
	inum fresult=0;
	
	//write log
	logsymlink(link,parent,name);

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

	int oldsize=buf.size();
	buf+=name;
	buf+=MIDDELIM;
	buf+=i2n(ino_out);
	buf+=MIDDELIM;

	if(ec->put(parent,buf)!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}
	std::string linkcontent(link);

	lc->acquire(ino_out);
	if(ec->put(ino_out,linkcontent)!=extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		lc->release(ino_out);
		return r;
	}
	lc->release(ino_out);
	lc->release(parent);

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
	extent_protocol::attr a;
    	if (ec->getattr(ino, a) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(ino);
        	return r;
    	}


	//write log before
	logsetattr(ino,size);


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
	


	std::string newbuf(changed);
	if(ec->put(ino,newbuf)!=extent_protocol::OK){
		r = IOERR;
		delete [] changed;
		lc->release(ino);
		return r;
	}
	delete [] changed;
	lc->release(ino);



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

	logcreate(parent,name,mode);

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


	int oldsize=buf.size();
	buf+=name;
	buf+=MIDDELIM;
	buf+=i2n(ino_out);
	buf+=MIDDELIM;


	if(ec->put(parent,buf)!=extent_protocol::OK){
		r = IOERR;
	}
	lc->release(parent);

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
	
	logmkdir(parent,name,mode);
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
	int oldsize=buf.size();
	buf+=name;
	buf+=MIDDELIM;
	buf+=i2n(ino_out);
	buf+=MIDDELIM;

	if(ec->put(parent,buf) != extent_protocol::OK){
		r = IOERR;
		lc->release(parent);
		return r;
	}
	lc->release(parent);
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
	logwrite(ino,size,off,data);

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

	if (ec->put(ino, buf) != extent_protocol::OK) {
        	r = IOERR;
		lc->release(ino);
        	return r;
    	}
	lc->release(ino);


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
	
	logunlink(parent,name);

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
    return r;
}


