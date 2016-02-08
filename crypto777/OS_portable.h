/******************************************************************************
 * Copyright © 2014-2016 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/
#ifndef OS_PORTABLEH
#define OS_PORTABLEH

// iguana_OS has functions that invoke system calls. Whenever possible stdio and similar functions are use and most functions are fully portable and in this file. For things that require OS specific, the call is routed to iguana_OS_portable_*  Usually, all but one OS can be handled with the same code, so iguana_OS_portable.c has most of this shared logic and an #ifdef iguana_OS_nonportable.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#ifdef __MINGW
#define sleep(x) Sleep(1000*(x))
#include "../win/mingw.h"
#include "../win/mman.h"

#else
#include <poll.h>
#include <netdb.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/socket.h>
#define closesocket close
#endif
#ifndef MIN
#define MIN(x, y) ( ((x)<(y))?(x):(y) )
#endif

#include "../includes/libgfshare.h"
#include "../includes/utlist.h"
#include "../includes/uthash.h"
#include "../includes/curve25519.h"
#include "../includes/cJSON.h"
#include "../includes/tweetnacl.h"

#ifndef MAP_FILE
#define MAP_FILE        0
#endif

struct huffstream { uint8_t *ptr,*buf; uint32_t bitoffset,maski,endpos; uint32_t allocsize:31,allocated:1; };
typedef struct huffstream HUFF;

struct ramcoder
{
    uint32_t cumulativeProb;
    uint16_t lower,upper,code,underflowBits,lastsymbol,upper_lastsymbol,counter;
    uint64_t *histo;
    uint16_t ranges[];
};

#define hrewind(hp) hseek(hp,0,SEEK_SET)
int32_t ramcoder_decoder(struct ramcoder *coder,int32_t updateprobs,uint8_t *buf,int32_t maxlen,HUFF *hp,bits256 *seed);
int32_t ramcoder_encoder(struct ramcoder *coder,int32_t updateprobs,uint8_t *buf,int32_t len,HUFF *hp,uint64_t *histo,bits256 *seed);
//int32_t init_ramcoder(struct ramcoder *coder,HUFF *hp,bits256 *seed);
int32_t ramcoder_decompress(uint8_t *data,int32_t maxlen,uint8_t *bits,uint32_t numbits,bits256 seed);
int32_t ramcoder_compress(uint8_t *bits,int32_t maxlen,uint8_t *data,int32_t datalen,bits256 seed);
uint64_t hconv_bitlen(uint64_t bitlen);
void _init_HUFF(HUFF *hp,int32_t allocsize,void *buf);
int32_t hgetbit(HUFF *hp);
int32_t hputbit(HUFF *hp,int32_t bit);
uint64_t hconv_bitlen(uint64_t bitlen);
int32_t hseek(HUFF *hp,int32_t offset,int32_t mode);

#define SCRIPT_OPRETURN 0x6a
#define GENESIS_ACCT "1739068987193023818"  // NXT-MRCC-2YLS-8M54-3CMAJ
#define GENESIS_PUBKEYSTR "1259ec21d31a30898d7cd1609f80d9668b4778e3d97e941044b39f0c44d2e51b"
#define GENESIS_PRIVKEYSTR "88a71671a6edd987ad9e9097428fc3f169decba3ac8f10da7b24e0ca16803b70"
#define GENESIS_SECRET "It was a bright cold day in April, and the clocks were striking thirteen."

#define SATOSHIDEN ((uint64_t)100000000L)
#define dstr(x) ((double)(x) / SATOSHIDEN)

#define SMALLVAL 0.000000000000001

#define SETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] |= (1 << ((bitoffset) & 7)))
#define GETBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] & (1 << ((bitoffset) & 7)))
#define CLEARBIT(bits,bitoffset) (((uint8_t *)bits)[(bitoffset) >> 3] &= ~(1 << ((bitoffset) & 7)))

#define portable_mutex_t pthread_mutex_t
#define portable_mutex_init(ptr) pthread_mutex_init(ptr,NULL)
#define portable_mutex_lock pthread_mutex_lock
#define portable_mutex_unlock pthread_mutex_unlock
#define OS_thread_create pthread_create

#define issue_curl(cmdstr) bitcoind_RPC(0,"curl",cmdstr,0,0,0)

struct allocitem { uint32_t allocsize,type; } __attribute__((packed));
struct queueitem { struct queueitem *next,*prev; uint32_t allocsize,type;  } __attribute__((packed));
typedef struct queue
{
	struct queueitem *list;
	portable_mutex_t mutex;
    char name[64],initflag;
} queue_t;

struct OS_mappedptr
{
	char fname[512];
	void *fileptr,*pending;
	long allocsize,changedsize;
	int32_t rwflag,dirty,actually_allocated;
    uint32_t closetime,opentime;
};

struct OS_memspace
{
    portable_mutex_t mutex; long used,totalsize; struct OS_mappedptr M; char name[64]; void *ptr;
    int32_t alignflag,counter,maxheight,openfiles,lastcounter,threadsafe,allocated:1,mapped:1,external:1;
#ifdef IGUANA_PEERALLOC
    int32_t outofptrs,numptrs,availptrs;
    void *ptrs[4096]; int32_t allocsizes[4096],maxsizes[4096];
#endif
};

struct tai { uint64_t x; double millis; };
struct taidate { int32_t year,month,day; };
struct taitime { struct taidate date; int32_t hour,minute,second; uint32_t offset; double millis; };
int32_t leapsecs_sub(struct tai *);

struct tai tai_now();
uint32_t tai2utc(struct tai t);
struct taidate taidate_frommjd(int32_t day,int32_t *pwday,int32_t *pyday);
struct taitime tai2time(struct tai t,int32_t *pwday,int32_t *pyday);
struct taidate tai2date(struct tai t);
int32_t taidate_str(char *s,struct taidate cd);
char *taitime_str(char *s,struct taitime ct);
int32_t taidate_mjd(struct taidate cd);
uint64_t tai2utime(struct tai t);
struct tai taitime2tai(struct taitime ct);
char *tai_str(char *str,struct tai t);
char *utc_str(char *str,uint32_t utc);
double tai_diff(struct tai reftai,struct tai cmptai);
uint32_t OS_conv_utime(char *utime);

//int32_t msync(void *addr,size_t len,int32_t flags);

#ifdef __PNACL
int32_t OS_nonportable_syncmap(struct OS_mappedptr *mp,long len);
void *OS_nonportable_tmpalloc(char *dirname,char *name,struct OS_memspace *mem,long origsize);

#elif _WIN32
char *OS_portable_path(char *str);
int32_t OS_nonportable_renamefile(char *fname,char *newfname);
int32_t  OS_nonportable_launch(char *args[]);
void OS_nonportable_randombytes(unsigned char *x,long xlen);
int32_t OS_nonportable_init();
#endif

void OS_portable_init();
void OS_init();

double OS_portable_milliseconds();
void OS_portable_randombytes(unsigned char *x,long xlen);
int32_t OS_portable_truncate(char *fname,long filesize);
char *OS_portable_path(char *str);
int32_t OS_portable_renamefile(char *fname,char *newfname);
int32_t OS_portable_removefile(char *fname);
void *OS_portable_mapfile(char *fname,long *filesizep,int32_t enablewrite);
int32_t OS_portable_syncmap(struct OS_mappedptr *mp,long len);
void *OS_portable_tmpalloc(char *dirname,char *name,struct OS_memspace *mem,long origsize);

int32_t is_DST(int32_t datenum);
int32_t extract_datenum(int32_t *yearp,int32_t *monthp,int32_t *dayp,int32_t datenum);
int32_t expand_datenum(char *date,int32_t datenum);
int32_t calc_datenum(int32_t year,int32_t month,int32_t day);
int32_t ecb_decrdate(int32_t *yearp,int32_t *monthp,int32_t *dayp,char *date,int32_t datenum);
int32_t conv_date(int32_t *secondsp,char *buf);
uint32_t OS_conv_datenum(int32_t datenum,int32_t hour,int32_t minute,int32_t second);
int32_t OS_conv_unixtime(struct tai *t,int32_t *secondsp,time_t timestamp);
double OS_milliseconds();

void OS_randombytes(unsigned char *x,long xlen);

int32_t OS_truncate(char *fname,long filesize);
char *OS_compatible_path(char *str);
int32_t OS_renamefile(char *fname,char *newfname);
int32_t OS_removefile(char *fname,int32_t scrubflag);
void OS_ensure_directory(char *dirname);
uint64_t OS_filesize(char *fname);
int32_t OS_compare_files(char *fname,char *fname2);
int64_t OS_copyfile(char *src,char *dest,int32_t cmpflag);
int32_t OS_releasemap(void *ptr,uint64_t filesize);
void _OS_closemap(struct OS_mappedptr *mp);
void OS_closemap(struct OS_mappedptr *mp);
long OS_ensurefilesize(char *fname,long filesize,int32_t truncateflag);
int32_t OS_openmap(struct OS_mappedptr *mp);
void *OS_mappedptr(void **ptrp,struct OS_mappedptr *mp,uint64_t allocsize,int32_t rwflag,char *fname);
void *OS_filealloc(struct OS_mappedptr *M,char *fname,struct OS_memspace *mem,long size);
void *OS_mapfile(char *fname,long *filesizep,int32_t enablewrite);
void *OS_loadfile(char *fname,char **bufp,long *lenp,long *allocsizep);
void *OS_filestr(long *allocsizep,char *fname);

int32_t OS_syncmap(struct OS_mappedptr *mp,long len);
void *OS_tmpalloc(char *dirname,char *name,struct OS_memspace *mem,long origsize);

long myallocated(uint8_t type,long change);
void *mycalloc(uint8_t type,int32_t n,long itemsize);
void myfree(void *_ptr,long allocsize);
void free_queueitem(void *itemdata);
void *myrealloc(uint8_t type,void *oldptr,long oldsize,long newsize);
void *myaligned_alloc(uint64_t allocsize);
int32_t myaligned_free(void *ptr,long size);

void *queueitem(char *str);
void queue_enqueue(char *name,queue_t *queue,struct queueitem *origitem,int32_t offsetflag);
void *queue_dequeue(queue_t *queue,int32_t offsetflag);
void *queue_delete(queue_t *queue,struct queueitem *copy,int32_t copysize,int32_t freeitem);
void *queue_free(queue_t *queue);
void *queue_clone(queue_t *clone,queue_t *queue,int32_t size);
int32_t queue_size(queue_t *queue);

void iguana_memreset(struct OS_memspace *mem);
void iguana_mempurge(struct OS_memspace *mem);
void *iguana_meminit(struct OS_memspace *mem,char *name,void *ptr,int64_t totalsize,int32_t threadsafe);
void *iguana_memalloc(struct OS_memspace *mem,long size,int32_t clearflag);
int64_t iguana_memfree(struct OS_memspace *mem,void *ptr,int32_t size);

// generic functions
int32_t unhex(char c);
void touppercase(char *str);
uint32_t is_ipaddr(char *str);
void iguana_bitmap(char *space,int32_t max,char *name);
double _pairaved(double valA,double valB);
int32_t unstringbits(char *buf,uint64_t bits);
uint64_t stringbits(char *str);
int32_t is_decimalstr(char *str);
void tolowercase(char *str);
char *clonestr(char *str);
int32_t is_hexstr(char *str,int32_t n);
int32_t decode_hex(unsigned char *bytes,int32_t n,char *hex);
void reverse_hexstr(char *str);
int32_t init_hexbytes_noT(char *hexbytes,uint8_t *message,long len);
uint16_t parse_ipaddr(char *ipaddr,char *ip_port);
int32_t bitweight(uint64_t x);
unsigned char _decode_hex(char *hex);
char *uppercase_str(char *buf,char *str);
char *lowercase_str(char *buf,char *str);
int32_t strsearch(char *strs[],int32_t num,char *name);
int32_t OS_getline(int32_t waitflag,char *line,int32_t max,char *dispstr);
int32_t sort64s(uint64_t *buf,uint32_t num,int32_t size);
int32_t revsort64s(uint64_t *buf,uint32_t num,int32_t size);
int decode_base32(uint8_t *token,uint8_t *tokenstr,int32_t len);
int init_base32(char *tokenstr,uint8_t *token,int32_t len);
char *OS_mvstr();

long _stripwhite(char *buf,int accept);
int32_t is_DST(int32_t datenum);
int32_t extract_datenum(int32_t *yearp,int32_t *monthp,int32_t *dayp,int32_t datenum);
int32_t expand_datenum(char *date,int32_t datenum);
int32_t calc_datenum(int32_t year,int32_t month,int32_t day);
int32_t ecb_decrdate(int32_t *yearp,int32_t *monthp,int32_t *dayp,char *date,int32_t datenum);
int32_t conv_date(int32_t *secondsp,char *buf);
uint32_t OS_conv_datenum(int32_t datenum,int32_t hour,int32_t minute,int32_t second);
int32_t OS_conv_unixtime(struct tai *t,int32_t *secondsp,time_t timestamp);
int32_t btc_coinaddr(char *coinaddr,uint8_t addrtype,char *pubkeystr);
int32_t btc_convaddr(char *hexaddr,char *addr58);

uint64_t RS_decode(char *rs);
int32_t RS_encode(char *rsaddr,uint64_t id);

void calc_sha1(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_md2(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_md4(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_md4str(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_md2str(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_md5str(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_sha224(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_sha384(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_sha512(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_sha224(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_rmd160(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_rmd128(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_rmd256(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_rmd320(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_tiger(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_whirlpool(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);

char *hmac_sha1_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_md2_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_md4_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_md5_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_sha224_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_sha256_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_sha384_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_sha512_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_rmd128_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_rmd160_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_rmd256_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_rmd320_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_tiger_str(char *dest,char *key,int32_t key_size,char *message);
char *hmac_whirlpool_str(char *dest,char *key,int32_t key_size,char *message);
int nn_base64_encode(const uint8_t *in,size_t in_len,char *out,size_t out_len);
int nn_base64_decode(const char *in,size_t in_len,uint8_t *out,size_t out_len);

void sha256_sha256(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void rmd160ofsha256(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_md5str(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_crc32str(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_NXTaddr(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_curve25519_str(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_base64_encodestr(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_base64_decodestr(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_hexstr(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);
void calc_unhexstr(char *hexstr,uint8_t *buf,uint8_t *msg,int32_t len);

uint64_t calc_ipbits(char *ip_port);
void expand_ipbits(char *ipaddr,uint64_t ipbits);
void escape_code(char *escaped,char *str);
void SaM_PrepareIndices();

// iguana_serdes.c
#define IGUANA_LOG2PACKETSIZE 21
#define IGUANA_MAXPACKETSIZE (1 << IGUANA_LOG2PACKETSIZE)
struct iguana_msghdr { uint8_t netmagic[4]; char command[12]; uint8_t serdatalen[4],hash[4]; } __attribute__((packed));

int32_t iguana_rwnum(int32_t rwflag,uint8_t *serialized,int32_t len,void *endianedp);
int32_t iguana_validatehdr(char *symbol,struct iguana_msghdr *H);
int32_t iguana_rwbignum(int32_t rwflag,uint8_t *serialized,int32_t len,uint8_t *endianedp);
int32_t iguana_sethdr(struct iguana_msghdr *H,const uint8_t netmagic[4],char *command,uint8_t *data,int32_t datalen);
uint8_t *iguana_varint16(int32_t rwflag,uint8_t *serialized,uint16_t *varint16p);
uint8_t *iguana_varint32(int32_t rwflag,uint8_t *serialized,uint16_t *varint16p);
uint8_t *iguana_varint64(int32_t rwflag,uint8_t *serialized,uint32_t *varint32p);
int32_t iguana_rwvarint(int32_t rwflag,uint8_t *serialized,uint64_t *varint64p);
int32_t iguana_rwvarint32(int32_t rwflag,uint8_t *serialized,uint32_t *int32p);
int32_t iguana_rwstr(int32_t rwflag,uint8_t *serialized,int32_t maxlen,char *endianedp);
int32_t iguana_rwmem(int32_t rwflag,uint8_t *serialized,int32_t len,void *endianedp);

bits256 bits256_doublesha256(char *hashstr,uint8_t *data,int32_t datalen);
char *bits256_str(char hexstr[65],bits256 x);
char *bits256_lstr(char hexstr[65],bits256 x);
bits256 bits256_add(bits256 a,bits256 b);
int32_t bits256_cmp(bits256 a,bits256 b);
bits256 bits256_lshift(bits256 x);
bits256 bits256_from_compact(uint32_t c);
bits256 bits256_conv(char *hexstr);
int32_t btc_priv2pub(uint8_t pubkey[33],uint8_t privkey[32]);
void calc_shares(unsigned char *shares,unsigned char *secret,int32_t size,int32_t width,int32_t M,int32_t N,unsigned char *sharenrs);

extern char *Iguana_validcommands[];
extern bits256 GENESIS_PUBKEY,GENESIS_PRIVKEY;
extern char NXTAPIURL[];
extern int32_t smallprimes[168],Debuglevel;

#endif

