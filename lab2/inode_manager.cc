#include "inode_manager.h"
char get_bit(char* buf,int addr){
	char bar=buf[addr/8];
	char ret=bar&(1<<(addr%8));
	return ret;
}
void set_bit(char* buf,int addr){
	char bar=buf[addr/8];
	char result=bar|(1<<(addr%8));
	buf[addr/8]=result;
}
void cancel_bit(char* buf,int addr){
	char bar=buf[addr/8];
	char result=bar&(~(1<<(addr%8)));
	buf[addr/8]=result;
}
#define CBITADDR(mbid,bid) (bid-(mbid-2)*BPB) 
// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
  /*
   *your lab1 code goes here.
   *if id is smaller than 0 or larger than BLOCK_NUM 
   *or buf is null, just return.
   *put the content of target block into buf.
   *hint: use memcpy
  */
	if(id<0||id>=BLOCK_NUM)
		return;
	if(buf==NULL)
		return;
	memcpy(buf,blocks[id],BLOCK_SIZE);
	
}

void
disk::write_block(blockid_t id, const char *buf)
{
  /*
   *your lab1 code goes here.
   *hint: just like read_block
  */
	if(id<0||id>=BLOCK_NUM)
		return;
	if(buf==NULL)
		return;
	memcpy(blocks[id],buf,BLOCK_SIZE);
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your lab1 code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.

   *hint: use macro IBLOCK and BBLOCK.
          use bit operation.
          remind yourself of the layout of disk.
   */
	char buf[BLOCK_SIZE];
	blockid_t bid,curmbid;
	bid=IBLOCK(sb.ninodes,sb.nblocks);
	curmbid=BBLOCK(bid);
	read_block(curmbid,buf);
	for(;bid<sb.nblocks;bid++){
		blockid_t next=BBLOCK(bid);
		if(curmbid<next){
			curmbid=next;
			read_block(curmbid,buf);
		}
		if(!get_bit(buf,CBITADDR(curmbid,bid))){
			set_bit(buf,CBITADDR(curmbid,bid));
			write_block(curmbid,buf);
			return bid;
		}
	}
	return BLOCK_NUM;
}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your lab1 code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
	char buf[BLOCK_SIZE];
	char newbuf[BLOCK_SIZE];
	blockid_t mbid;
	mbid=BBLOCK(id);
	bzero(newbuf,BLOCK_SIZE);
	write_block(id,newbuf);
	read_block(mbid,buf);
	cancel_bit(buf,CBITADDR(mbid,id));
	write_block(mbid,buf);
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your lab1 code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
    
   * if you get some heap memory, do not forget to free it.
   */
	char buf[BLOCK_SIZE];
	char nodebuf[BLOCK_SIZE];
	inode_t tmpnode;
	bm->read_block(2,buf);
	for(int i=1;i<INODE_NUM;i++){
		if(!get_bit(buf,IBLOCK(i, bm->sb.nblocks))){
			tmpnode.type=type;
			tmpnode.size=0;
			memcpy(nodebuf,(char*)(&tmpnode),sizeof(tmpnode));
			bm->write_block(IBLOCK(i, bm->sb.nblocks),nodebuf);
			set_bit(buf,IBLOCK(i, bm->sb.nblocks));
			bm->write_block(2,buf);
			return i;
		}
	}
	return NULL;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your lab1 code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   * do not forget to free memory if necessary.
   */
	char buf[BLOCK_SIZE];
	char nodebuf[BLOCK_SIZE];
	bm->read_block(2,buf);
	if(get_bit(buf,IBLOCK(inum, bm->sb.nblocks))){
		bm->read_block(IBLOCK(inum, bm->sb.nblocks),nodebuf);
		memset(nodebuf,0,BLOCK_SIZE);
		bm->write_block(IBLOCK(inum, bm->sb.nblocks),nodebuf);
		cancel_bit(buf,IBLOCK(inum, bm->sb.nblocks));
		bm->write_block(2,buf);
	}
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino, *ino_disk;
  char buf[BLOCK_SIZE];

  printf("\tim: get_inode %d\n", inum);

  if (inum < 0 || inum >= INODE_NUM) {
    printf("\tim: inum out of range\n");
    return NULL;
  }

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  // printf("%s:%d\n", __FILE__, __LINE__);

  ino_disk = (struct inode*)buf + inum%IPB;
  if (ino_disk->type == 0) {
    printf("\tim: inode not exist\n");
    return NULL;
  }

  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
  /*
   * your lab1 code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_out
   */
	struct inode *ino = get_inode(inum);
	ino->atime = time(0);
	put_inode(inum, ino);
	*size = ino->size;
	unsigned int nBlock = 0;
	if(ino->size != 0) nBlock = (ino->size-1)/BLOCK_SIZE + 1;
	*buf_out = (char*)malloc(nBlock * BLOCK_SIZE * sizeof(char*));

	if(nBlock <= NDIRECT){
		for(unsigned int i= 0; i< nBlock; ++i){
			bm->read_block(ino->blocks[i], *buf_out + i*BLOCK_SIZE);
		}
	}else{
		for(unsigned int i= 0; i< NDIRECT; ++i){
			bm->read_block(ino->blocks[i], *buf_out + i*BLOCK_SIZE);
		}
		blockid_t tmpBlock[NINDIRECT];
		bm->read_block(ino->blocks[NDIRECT], (char*)&tmpBlock);
		for(unsigned int i= NDIRECT; i< nBlock; ++i){
			bm->read_block(tmpBlock[i-NDIRECT], *buf_out +i*BLOCK_SIZE);
		}
	}
	free(ino);
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your lab1 code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode.
   * you should free some blocks if necessary.
   */
	struct inode *ino = get_inode(inum);
	ino->atime = time(0);
	ino->mtime = time(0);
	ino->ctime = time(0);
	unsigned int nOldBlock = 0;
	unsigned int nNewBlock = 0;
	if(ino->size != 0) nOldBlock = (ino->size - 1)/BLOCK_SIZE + 1;
	if(size != 0) nNewBlock = (size - 1)/BLOCK_SIZE + 1;

	ino->size = size;

	if(nNewBlock > MAXFILE) return;
	//alloc new memery if necessary
	if(nNewBlock > nOldBlock){
		if(nNewBlock <= NDIRECT){
			for(unsigned int i= nOldBlock; i< nNewBlock; ++i){
				ino->blocks[i] = bm->alloc_block();
			}
		}else if(nOldBlock<= NDIRECT){
			for(unsigned int i= nOldBlock; i< NDIRECT; ++i){
				ino->blocks[i] = bm->alloc_block();
			}
			ino->blocks[NDIRECT] = bm->alloc_block();
			blockid_t tmpBlock[NINDIRECT];
			for(unsigned int i= NDIRECT; i< nNewBlock; ++i){
				tmpBlock[i-NDIRECT] = bm->alloc_block();
			}
			bm->write_block(ino->blocks[NDIRECT], (const char*)tmpBlock);
		}else{
			blockid_t tmpBlock[NINDIRECT];
			bm->read_block(ino->blocks[NDIRECT], (char*)tmpBlock);
			for(unsigned int i= nOldBlock; i< nNewBlock; ++i){
				tmpBlock[i-NDIRECT] = bm->alloc_block();
			}
			bm->write_block(ino->blocks[NDIRECT], (const char*)tmpBlock);
		}
	}
	//copy
	if(nNewBlock <= NDIRECT){
		for(unsigned int i= 0; i< nNewBlock; ++i){
			bm->write_block(ino->blocks[i], buf+BLOCK_SIZE*i);
		}
	}else{
		for(unsigned int i= 0; i< NDIRECT; ++i){
			bm->write_block(ino->blocks[i], buf+BLOCK_SIZE*i);
		}
		blockid_t tmpBlock[NINDIRECT];
		bm->read_block(ino->blocks[NDIRECT], (char*)tmpBlock);
		for(unsigned int i= NDIRECT; i< nNewBlock; ++i){
			bm->write_block(tmpBlock[i-NDIRECT], buf+BLOCK_SIZE*i);
		}
	}
	//free if necessary
	if(nNewBlock < nOldBlock){
		if(nOldBlock <= NDIRECT){
			for(unsigned int i= nNewBlock; i< nOldBlock; ++i){
				bm->free_block(ino->blocks[i]);
			}
		}else if(nNewBlock <= NDIRECT){
			for(unsigned int i= nNewBlock; i< NDIRECT; ++i){
				bm->free_block(ino->blocks[i]);
			}
			blockid_t tmpBlock[NINDIRECT];
			bm->read_block(ino->blocks[NDIRECT], (char*)tmpBlock);
			for(unsigned int i= NDIRECT; i< nOldBlock; ++i){
				bm->free_block(tmpBlock[i-NDIRECT]);
			}
			bm->free_block(ino->blocks[NDIRECT]);
		}else{
			blockid_t tmpBlock[NINDIRECT];
			bm->read_block(ino->blocks[NDIRECT], (char*)tmpBlock);
			for(unsigned int i= nNewBlock; i< nOldBlock; ++i){
				bm->free_block(tmpBlock[i-NDIRECT]);
			}
			bm->write_block(ino->blocks[NDIRECT], (const char*)tmpBlock);
		}
	}
	put_inode(inum, ino);
	free(ino);
}

void
inode_manager::getattr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your lab1 code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
	struct inode* ino=get_inode(inum);
	if(ino==NULL)
		return;
	a.type=ino->type;
	a.size=ino->size;
	a.atime=ino->atime;
	a.mtime=ino->mtime;
	a.ctime=ino->ctime;
	free(ino);
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your lab1 code goes here
   * note: you need to consider about both the data block and inode of the file
   * do not forget to free memory if necessary.
   */
	struct inode *ino = get_inode(inum);
	unsigned int nBlock = 0;
	if(ino->size != 0) nBlock = (ino->size-1)/BLOCK_SIZE+1;
	if(nBlock <= NDIRECT){
		for(unsigned int i= 0; i< nBlock; ++i){
			bm->free_block(ino->blocks[i]);
		}
	}else{
		for(unsigned int i= 0; i< NDIRECT; ++i){
			bm->free_block(ino->blocks[i]);
		}
		blockid_t tmpBlock[NINDIRECT];
		bm->read_block(ino->blocks[NDIRECT], (char*)&tmpBlock);
		for(unsigned int i= NDIRECT; i< nBlock; ++i){
			bm->free_block(tmpBlock[i-NDIRECT]);
		}
		bm->free_block(ino->blocks[NDIRECT]);
	}
	free_inode(inum);
	free(ino);
}
