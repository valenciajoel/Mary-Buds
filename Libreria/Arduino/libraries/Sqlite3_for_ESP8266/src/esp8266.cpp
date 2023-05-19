/* From: https://chromium.googlesource.com/chromium/src.git/+/4.1.249.1050/third_party/sqlite/src/os_symbian.cc
 * https://github.com/spsoft/spmemvfs/tree/master/spmemvfs
 * http://www.sqlite.org/src/doc/trunk/src/test_demovfs.c
 * http://www.sqlite.org/src/doc/trunk/src/test_vfstrace.c
 * http://www.sqlite.org/src/doc/trunk/src/test_onefile.c
 * http://www.sqlite.org/src/doc/trunk/src/test_vfs.c
 * https://github.com/nodemcu/nodemcu-firmware/blob/master/app/sqlite3/esp8266.c
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <c_types.h>
#include <osapi.h>
#include <time.h>
#include <spi_flash.h>
#include <sqlite3.h>
#include <Arduino.h>
#include "vfs.h"
extern "C" {
#include "user_interface.h"
}

#include "shox96_0_2.h"

// From https://stackoverflow.com/questions/19758270/read-varint-from-linux-sockets#19760246
// Encode an unsigned 64-bit varint.  Returns number of encoded bytes.
// 'buffer' must have room for up to 10 bytes.
int encode_unsigned_varint(uint8_t *buffer, uint64_t value) {
	int encoded = 0;
	do {
		uint8_t next_byte = value & 0x7F;
		value >>= 7;
		if (value)
			next_byte |= 0x80;
		buffer[encoded++] = next_byte;
	} while (value);
	return encoded;
}

uint64_t decode_unsigned_varint(const uint8_t *data, int &decoded_bytes) {
	int i = 0;
	uint64_t decoded_value = 0;
	int shift_amount = 0;
	do {
		decoded_value |= (uint64_t)(data[i] & 0x7F) << shift_amount;     
		shift_amount += 7;
	} while ((data[i++] & 0x80) != 0);
	decoded_bytes = i;
	return decoded_value;
}

#undef dbg_printf
//#define dbg_printf(...) Serial.printf(__VA_ARGS__)
#define dbg_printf(...) 0
#define CACHEBLOCKSZ 64
#define ESP8266_DEFAULT_MAXNAMESIZE 32

extern "C" {
    void SerialPrintln(const char *str) {
        //Serial.println(str);
    }
}

int esp8266_Close(sqlite3_file*);
int esp8266_Lock(sqlite3_file *, int);
int esp8266_Unlock(sqlite3_file*, int);
int esp8266_Sync(sqlite3_file*, int);
int esp8266_Open(sqlite3_vfs*, const char *, sqlite3_file *, int, int*);
int esp8266_Read(sqlite3_file*, void*, int, sqlite3_int64);
int esp8266_Write(sqlite3_file*, const void*, int, sqlite3_int64);
int esp8266_Truncate(sqlite3_file*, sqlite3_int64);
int esp8266_Delete(sqlite3_vfs*, const char *, int);
int esp8266_FileSize(sqlite3_file*, sqlite3_int64*);
int esp8266_Access(sqlite3_vfs*, const char*, int, int*);
int esp8266_FullPathname( sqlite3_vfs*, const char *, int, char*);
int esp8266_CheckReservedLock(sqlite3_file*, int *);
int esp8266_FileControl(sqlite3_file *, int, void*);
int esp8266_SectorSize(sqlite3_file*);
int esp8266_DeviceCharacteristics(sqlite3_file*);
void* esp8266_DlOpen(sqlite3_vfs*, const char *);
void esp8266_DlError(sqlite3_vfs*, int, char*);
void (*esp8266_DlSym (sqlite3_vfs*, void*, const char*))(void);
void esp8266_DlClose(sqlite3_vfs*, void*);
int esp8266_Randomness(sqlite3_vfs*, int, char*);
int esp8266_Sleep(sqlite3_vfs*, int);
int esp8266_CurrentTime(sqlite3_vfs*, double*);

int esp8266mem_Close(sqlite3_file*);
int esp8266mem_Read(sqlite3_file*, void*, int, sqlite3_int64);
int esp8266mem_Write(sqlite3_file*, const void*, int, sqlite3_int64);
int esp8266mem_FileSize(sqlite3_file*, sqlite3_int64*);
int esp8266mem_Sync(sqlite3_file*, int);

typedef struct st_linkedlist {
	uint16_t blockid;
	struct st_linkedlist *next;
	uint8_t data[CACHEBLOCKSZ];
} linkedlist_t, *pLinkedList_t;

typedef struct st_filecache {
	uint32_t size;
	linkedlist_t *list;
} filecache_t, *pFileCache_t;

typedef struct esp8266_file {
	sqlite3_file base;
	vfs_file *fd;
	filecache_t *cache;
	char name[ESP8266_DEFAULT_MAXNAMESIZE];
} esp8266_file;

sqlite3_vfs  esp8266Vfs = {
	1,			// iVersion
	sizeof(esp8266_file),	// szOsFile
	FS_OBJ_NAME_LEN,	// mxPathname
	NULL,			// pNext
	"esp8266",		// name
	0,			// pAppData
	esp8266_Open,		// xOpen
	esp8266_Delete,		// xDelete
	esp8266_Access,		// xAccess
	esp8266_FullPathname,	// xFullPathname
	esp8266_DlOpen,		// xDlOpen
	esp8266_DlError,	// xDlError
	esp8266_DlSym,		// xDlSym
	esp8266_DlClose,	// xDlClose
	esp8266_Randomness,	// xRandomness
	esp8266_Sleep,		// xSleep
	esp8266_CurrentTime,	// xCurrentTime
	0			// xGetLastError
};

const sqlite3_io_methods esp8266IoMethods = {
	1,
	esp8266_Close,
	esp8266_Read,
	esp8266_Write,
	esp8266_Truncate,
	esp8266_Sync,
	esp8266_FileSize,
	esp8266_Lock,
	esp8266_Unlock,
	esp8266_CheckReservedLock,
	esp8266_FileControl,
	esp8266_SectorSize,
	esp8266_DeviceCharacteristics
};

const sqlite3_io_methods esp8266MemMethods = {
	1,
	esp8266mem_Close,
	esp8266mem_Read,
	esp8266mem_Write,
	esp8266_Truncate,
	esp8266mem_Sync,
	esp8266mem_FileSize,
	esp8266_Lock,
	esp8266_Unlock,
	esp8266_CheckReservedLock,
	esp8266_FileControl,
	esp8266_SectorSize,
	esp8266_DeviceCharacteristics
};

uint32_t linkedlist_store (linkedlist_t **leaf, uint32_t offset, uint32_t len, const uint8_t *data) {
	const uint8_t blank[CACHEBLOCKSZ] = { 0 };
	uint16_t blockid = offset/CACHEBLOCKSZ;
	linkedlist_t *block;

	if (!memcmp(data, blank, CACHEBLOCKSZ))
		return len;

	block = *leaf;
	if (!block || ( block->blockid != blockid ) ) {
		block = (linkedlist_t *) sqlite3_malloc ( sizeof( linkedlist_t ) );
		if (!block)
			return SQLITE_NOMEM;

		memset (block->data, 0, CACHEBLOCKSZ);
		block->blockid = blockid;
	}

	if (!*leaf) {
		*leaf = block;
		block->next = NULL;
	} else if (block != *leaf) {
		if (block->blockid > (*leaf)->blockid) {
			block->next = (*leaf)->next;
			(*leaf)->next = block;
		} else {
			block->next = (*leaf);
			(*leaf) = block;
		}
	}

	memcpy (block->data + offset%CACHEBLOCKSZ, data, len);

	return len;
}

uint32_t filecache_pull (pFileCache_t cache, uint32_t offset, uint32_t len, uint8_t *data) {
	uint16_t i;
	float blocks;
	uint32_t r = 0;

	blocks = ( offset % CACHEBLOCKSZ + len ) / (float) CACHEBLOCKSZ;
	if (blocks == 0.0)
		return 0;
	if (!cache->list)
		return 0;

	if (( blocks - (int) blocks) > 0.0)
		blocks = blocks + 1.0;

	for (i = 0; i < (uint16_t) blocks; i++) {
		uint16_t round;
		float relablock;
		linkedlist_t *leaf;
		uint32_t relaoffset, relalen;
		uint8_t * reladata = (uint8_t*) data;

		relalen = len - r;

		reladata = reladata + r;
		relaoffset = offset + r;

		round = CACHEBLOCKSZ - relaoffset%CACHEBLOCKSZ;
		if (relalen > round) relalen = round;

		for (leaf = cache->list; leaf && leaf->next; leaf = leaf->next) {
			if ( ( leaf->next->blockid * CACHEBLOCKSZ ) > relaoffset )
				break;
		}

		relablock = relaoffset/((float)CACHEBLOCKSZ) - leaf->blockid;

		if ( ( relablock >= 0 ) && ( relablock < 1 ) )
			memcpy (data + r, leaf->data + (relaoffset % CACHEBLOCKSZ), relalen);

		r = r + relalen;
	}
SerialPrintln("fcp3");

	return 0;
}

uint32_t filecache_push (pFileCache_t cache, uint32_t offset, uint32_t len, const uint8_t *data) {
	uint16_t i;
	float blocks;
	uint32_t r = 0;
	uint8_t updateroot = 0x1;

	blocks = ( offset % CACHEBLOCKSZ + len ) / (float) CACHEBLOCKSZ;

	if (blocks == 0.0)
		return 0;

	if (( blocks - (int) blocks) > 0.0)
		blocks = blocks + 1.0;

	for (i = 0; i < (uint16_t) blocks; i++) {
		uint16_t round;
		uint32_t localr;
		linkedlist_t *leaf;
		uint32_t relaoffset, relalen;
		uint8_t * reladata = (uint8_t*) data;

		relalen = len - r;

		reladata = reladata + r;
		relaoffset = offset + r;

		round = CACHEBLOCKSZ - relaoffset%CACHEBLOCKSZ;
		if (relalen > round) relalen = round;

		for (leaf = cache->list; leaf && leaf->next; leaf = leaf->next) {
			if ( ( leaf->next->blockid * CACHEBLOCKSZ ) > relaoffset )
				break;
			updateroot = 0x0;
		}

		localr = linkedlist_store(&leaf, relaoffset, (relalen > CACHEBLOCKSZ) ? CACHEBLOCKSZ : relalen, reladata);
		if (localr == SQLITE_NOMEM)
			return SQLITE_NOMEM;

		r = r + localr;

		if (updateroot & 0x1)
			cache->list = leaf;
	}

	if (offset + len > cache->size)
		cache->size = offset + len;

	return r;
}

void filecache_free (pFileCache_t cache) {
	pLinkedList_t ll = cache->list, next;

	while (ll != NULL) {
		next = ll->next;
		sqlite3_free (ll);
		ll = next;
	}
}

int esp8266mem_Close(sqlite3_file *id)
{
	esp8266_file *file = (esp8266_file*) id;

	filecache_free(file->cache);
	sqlite3_free (file->cache);

	dbg_printf("esp8266mem_Close: %s OK\n", file->name);
	return SQLITE_OK;
}

int esp8266mem_Read(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset)
{
	sint32_t ofst;
	esp8266_file *file = (esp8266_file*) id;
dbg_printf("memread\n");
	ofst = (sint32_t)(offset & 0x7FFFFFFF);

	filecache_pull (file->cache, ofst, amount, (uint8_t *) buffer);

	dbg_printf("esp8266mem_Read: %s [%ld] [%d] OK\n", file->name, ofst, amount);
	Serial.flush();
	return SQLITE_OK;
}

int esp8266mem_Write(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset)
{
	sint32_t ofst;
	esp8266_file *file = (esp8266_file*) id;

	ofst = (sint32_t)(offset & 0x7FFFFFFF);

	filecache_push (file->cache, ofst, amount, (const uint8_t *) buffer);

	dbg_printf("esp8266mem_Write: %s [%ld] [%d] OK\n", file->name, ofst, amount);
	return SQLITE_OK;
}

int esp8266mem_Sync(sqlite3_file *id, int flags)
{
	esp8266_file *file = (esp8266_file*) id;
	dbg_printf("esp8266mem_Sync: %s OK\n", file->name);
	return  SQLITE_OK;
}

int esp8266mem_FileSize(sqlite3_file *id, sqlite3_int64 *size)
{
	esp8266_file *file = (esp8266_file*) id;

	*size = 0LL | file->cache->size;
	dbg_printf("esp8266mem_FileSize: %s [%d] OK\n", file->name, file->cache->size);
	return SQLITE_OK;
}

const char *MODE_READONLY = "r";
const char *MODE_READPLUS = "r+";
const char *MODE_WRITEPLUS = "w+";
int esp8266_Open( sqlite3_vfs * vfs, const char * path, sqlite3_file * file, int flags, int * outflags )
{
	int rc;
	const char *mode = MODE_READONLY;
	esp8266_file *p = (esp8266_file*) file;

	if ( path == NULL ) return SQLITE_IOERR;
	if( flags&SQLITE_OPEN_READONLY )  mode = MODE_READONLY;
	if( flags&SQLITE_OPEN_READWRITE || flags&SQLITE_OPEN_MAIN_JOURNAL ) {
		int result;
		if (SQLITE_OK != esp8266_Access(vfs, path, flags, &result))
			return SQLITE_CANTOPEN;

		if (result == 1)
			mode = MODE_READPLUS;
		else
			mode = MODE_WRITEPLUS;
	}

	dbg_printf("esp8266_Open: 1o %s %s\n", path, mode);
	memset (p, 0, sizeof(esp8266_file));

  strncpy (p->name, path, ESP8266_DEFAULT_MAXNAMESIZE);
	p->name[ESP8266_DEFAULT_MAXNAMESIZE-1] = '\0';

	if( flags&SQLITE_OPEN_MAIN_JOURNAL ) {
		p->fd = 0;
		p->cache = (filecache_t *) sqlite3_malloc(sizeof (filecache_t));
		if (! p->cache )
			return SQLITE_NOMEM;
		memset (p->cache, 0, sizeof(filecache_t));

		p->base.pMethods = &esp8266MemMethods;
		dbg_printf("esp8266_Open: 2o %s %d %ld MEM OK\n", p->name, p->fd, system_get_free_heap_size());
		return SQLITE_OK;
	}

	vfs_file *filep = vfs_open (path, mode);
	p->fd = filep;
	dbg_printf("esp8266_Open: 2o %s %ld OK\n", p->name, filep);
	if ( p->fd <= 0 ) {
		return SQLITE_CANTOPEN;
	}

	p->base.pMethods = &esp8266IoMethods;
	dbg_printf("esp8266_Open: 2o %s %d OK\n", p->name, p->fd);
	return SQLITE_OK;
}

int esp8266_Close(sqlite3_file *id)
{
	esp8266_file *file = (esp8266_file*) id;

	int rc = vfs_close(file->fd);
	dbg_printf("esp8266_Close: %s %d %d\n", file->name, file->fd, rc);
	return rc ? SQLITE_IOERR_CLOSE : SQLITE_OK;
}

int esp8266_Read(sqlite3_file *id, void *buffer, int amount, sqlite3_int64 offset)
{
	size_t nRead;
	sint32_t ofst, iofst;
	esp8266_file *file = (esp8266_file*) id;

	iofst = (sint32_t)(offset & 0x7FFFFFFF);

	dbg_printf("esp8266_Read: 1r %s %d %d %lld[%ld] \n", file->name, file->fd, amount, offset, iofst);
	ofst = vfs_lseek(file->fd, iofst, VFS_SEEK_SET);
	if (ofst != iofst) {
	    dbg_printf("esp8266_Read: 2r %ld != %ld FAIL\n", ofst, iofst);
		return SQLITE_IOERR_SHORT_READ /* SQLITE_IOERR_SEEK */;
	}

	nRead = vfs_read(file->fd, buffer, amount);
	if ( nRead == amount ) {
	    dbg_printf("esp8266_Read: 3r %s %u %d OK\n", file->name, nRead, amount);
		return SQLITE_OK;
	} else if ( nRead >= 0 ) {
	    dbg_printf("esp8266_Read: 3r %s %u %d FAIL\n", file->name, nRead, amount);
		return SQLITE_IOERR_SHORT_READ;
	}

	dbg_printf("esp8266_Read: 4r %s FAIL\n", file->name);
	return SQLITE_IOERR_READ;
}

int esp8266_Write(sqlite3_file *id, const void *buffer, int amount, sqlite3_int64 offset)
{
	size_t nWrite;
	sint32_t ofst, iofst;
	esp8266_file *file = (esp8266_file*) id;

	iofst = (sint32_t)(offset & 0x7FFFFFFF);

	dbg_printf("esp8266_Write: 1w %s %d %d %lld[%ld] \n", file->name, file->fd, amount, offset, iofst);
	ofst = vfs_lseek(file->fd, iofst, VFS_SEEK_SET);
	if (ofst != iofst) {
		return SQLITE_IOERR_SEEK;
	}

	nWrite = vfs_write(file->fd, buffer, amount);
	if ( nWrite != amount ) {
		dbg_printf("esp8266_Write: 2w %s %u %d\n", file->name, nWrite, amount);
		return SQLITE_IOERR_WRITE;
	}

	dbg_printf("esp8266_Write: 3w %s OK\n", file->name);
	return SQLITE_OK;
}

int esp8266_Truncate(sqlite3_file *id, sqlite3_int64 bytes)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_Truncate:\n");
	return 0 ? SQLITE_IOERR_TRUNCATE : SQLITE_OK;
}

int esp8266_Delete( sqlite3_vfs * vfs, const char * path, int syncDir )
{
	sint32_t rc = vfs_remove( path );
	if (rc == VFS_RES_ERR)
		return SQLITE_IOERR_DELETE;

	dbg_printf("esp8266_Delete: %s OK\n", path);
	return SQLITE_OK;
}

int esp8266_FileSize(sqlite3_file *id, sqlite3_int64 *size)
{
	esp8266_file *file = (esp8266_file*) id;
	*size = 0LL | vfs_size( file->fd );
	dbg_printf("esp8266_FileSize: %s %u[%lld]\n", file->name, vfs_size(file->fd), *size);
	return SQLITE_OK;
}

int esp8266_Sync(sqlite3_file *id, int flags)
{
	esp8266_file *file = (esp8266_file*) id;

	int rc = vfs_flush( file->fd );
	dbg_printf("esp8266_Sync: %d\n", rc);

	return rc ? SQLITE_IOERR_FSYNC : SQLITE_OK;
}

int esp8266_Access( sqlite3_vfs * vfs, const char * path, int flags, int * result )
{
	struct vfs_stat st;
	sint32_t rc = vfs_stat( path, &st );
	*result = ( rc != VFS_RES_ERR );

	dbg_printf("esp8266_Access: %d\n", *result);
	return SQLITE_OK;
}

int esp8266_FullPathname( sqlite3_vfs * vfs, const char * path, int len, char * fullpath )
{
	struct vfs_stat st;
	sint32_t rc = vfs_stat( path, &st );
	if ( rc == VFS_RES_OK ){
		strncpy( fullpath, st.name, len );
	} else {
		strncpy( fullpath, path, len );
	}

	fullpath[ len - 1 ] = '\0';

	dbg_printf("esp8266_FullPathname: %s\n", fullpath);
	return SQLITE_OK;
}

int esp8266_Lock(sqlite3_file *id, int lock_type)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_Lock:\n");
	return SQLITE_OK;
}

int esp8266_Unlock(sqlite3_file *id, int lock_type)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_Unlock:\n");
	return SQLITE_OK;
}

int esp8266_CheckReservedLock(sqlite3_file *id, int *result)
{
	esp8266_file *file = (esp8266_file*) id;

	*result = 0;

	dbg_printf("esp8266_CheckReservedLock:\n");
	return SQLITE_OK;
}

int esp8266_FileControl(sqlite3_file *id, int op, void *arg)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_FileControl:\n");
	return SQLITE_OK;
}

int esp8266_SectorSize(sqlite3_file *id)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_SectorSize:\n");
	return SPI_FLASH_SEC_SIZE;
}

int esp8266_DeviceCharacteristics(sqlite3_file *id)
{
	esp8266_file *file = (esp8266_file*) id;

	dbg_printf("esp8266_DeviceCharacteristics:\n");
	return 0;
}

void * esp8266_DlOpen( sqlite3_vfs * vfs, const char * path )
{
	dbg_printf("esp8266_DlOpen:\n");
	return NULL;
}

void esp8266_DlError( sqlite3_vfs * vfs, int len, char * errmsg )
{
	dbg_printf("esp8266_DlError:\n");
	return;
}

void ( * esp8266_DlSym ( sqlite3_vfs * vfs, void * handle, const char * symbol ) ) ( void )
{
	dbg_printf("esp8266_DlSym:\n");
	return NULL;
}

void esp8266_DlClose( sqlite3_vfs * vfs, void * handle )
{
	dbg_printf("esp8266_DlClose:\n");
	return;
}

int esp8266_Randomness( sqlite3_vfs * vfs, int len, char * buffer )
{
	int rc = os_get_random((unsigned char *) buffer, len);
	dbg_printf("esp8266_Randomness: %d\n", rc);
	return SQLITE_OK;
}

int esp8266_Sleep( sqlite3_vfs * vfs, int microseconds )
{
	dbg_printf("esp8266_Sleep:\n");
	return SQLITE_OK;
}

int esp8266_CurrentTime( sqlite3_vfs * vfs, double * result )
{
	// This is stubbed out until we have a working RTCTIME solution;
	// as it stood, this would always have returned the UNIX epoch.
	// time_t t = time(NULL);
	// *result = t / 86400.0 + 2440587.5;
	*result = 2440587.5;
	dbg_printf("esp8266_CurrentTime: %g\n", *result);
	return SQLITE_OK;
}

static void shox96_0_2c(sqlite3_context *context, int argc, sqlite3_value **argv) {
  int nIn, nOut;
  long int nOut2;
  const unsigned char *inBuf;
  unsigned char *outBuf;
	unsigned char vInt[9];
	int vIntLen;

  nIn = sqlite3_value_bytes(argv[0]);
  inBuf = (unsigned char *) sqlite3_value_blob(argv[0]);
  nOut = 13 + nIn + (nIn+999)/1000;
  vIntLen = encode_unsigned_varint(vInt, (uint64_t) nIn);

  outBuf = (unsigned char *) malloc( nOut+vIntLen );
	memcpy(outBuf, vInt, vIntLen);
  nOut2 = shox96_0_2_compress((const char *) inBuf, nIn, (char *) &outBuf[vIntLen], NULL);
  sqlite3_result_blob(context, outBuf, nOut2+vIntLen, free);
}

static void shox96_0_2d(sqlite3_context *context, int argc, sqlite3_value **argv) {
  unsigned int nIn, nOut, rc;
  const unsigned char *inBuf;
  unsigned char *outBuf;
  long int nOut2;
  uint64_t inBufLen64;
	int vIntLen;

  if (sqlite3_value_type(argv[0]) != SQLITE_BLOB)
	  return;

  nIn = sqlite3_value_bytes(argv[0]);
  if (nIn < 2){
    return;
  }
  inBuf = (unsigned char *) sqlite3_value_blob(argv[0]);
  inBufLen64 = decode_unsigned_varint(inBuf, vIntLen);
	nOut = (unsigned int) inBufLen64;
  outBuf = (unsigned char *) malloc( nOut );
  //nOut2 = (long int)nOut;
  nOut2 = shox96_0_2_decompress((const char *) (inBuf + vIntLen), nIn - vIntLen, (char *) outBuf, NULL);
  //if( rc!=Z_OK ){
  //  free(outBuf);
  //}else{
    sqlite3_result_blob(context, outBuf, nOut2, free);
  //}
} 

int registerShox96_0_2(sqlite3 *db, const char **pzErrMsg, const struct sqlite3_api_routines *pThunk) {
  sqlite3_create_function(db, "shox96_0_2c", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0, shox96_0_2c, 0, 0);
  sqlite3_create_function(db, "shox96_0_2d", 1, SQLITE_UTF8 | SQLITE_DETERMINISTIC, 0, shox96_0_2d, 0, 0);
  return SQLITE_OK;
}

int sqlite3_os_init(void){
  sqlite3_vfs_register(&esp8266Vfs, 1);
  sqlite3_auto_extension((void (*)())registerShox96_0_2);
  return SQLITE_OK;
}

int sqlite3_os_end(void){
  return SQLITE_OK;
}
