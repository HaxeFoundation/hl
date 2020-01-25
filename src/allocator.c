/*
 * Copyright (C)2005-2020 Haxe Foundation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include "hl.h"

#ifdef HL_WIN
#	include <intrin.h>
static unsigned int __inline TRAILING_ONES( unsigned int x ) {
	DWORD msb = 0;
	if( _BitScanForward( &msb, ~x ) )
		return msb;
	return 32;
}
static unsigned int __inline TRAILING_ZEROES( unsigned int x ) {
	DWORD msb = 0;
	if( _BitScanForward( &msb, x ) )
		return msb;
	return 32;
}
#else
static inline unsigned int TRAILING_ONES( unsigned int x ) {
	return (~x) ? __builtin_ctz(~x) : 32;
}
static inline unsigned int TRAILING_ZEROES( unsigned int x ) {
	return x ? __builtin_ctz(x) : 32;
}
#endif

#define GC_PARTITIONS	9
#define GC_PART_BITS	4
#define GC_FIXED_PARTS	5
static const int GC_SBITS[GC_PARTITIONS] = {0,0,0,0,0,		3,6,14,22};

#ifdef HL_64
static const int GC_SIZES[GC_PARTITIONS] = {8,16,24,32,40,	8,64,1<<14,1<<22};
#	define GC_ALIGN_BITS		3
#else
static const int GC_SIZES[GC_PARTITIONS] = {4,8,12,16,20,	8,64,1<<14,1<<22};
#	define GC_ALIGN_BITS		2
#endif


#define GC_ALL_PAGES	(GC_PARTITIONS << PAGE_KIND_BITS)
#define	GC_ALIGN		(1 << GC_ALIGN_BITS)

static gc_pheader *gc_pages[GC_ALL_PAGES] = {NULL};
static gc_pheader *gc_free_pages[GC_ALL_PAGES] = {NULL};

#define GC_FREELIST_MAX 16
static void* gc_free_list[1 << PAGE_KIND_BITS][GC_FREELIST_MAX] = {NULL};


static gc_pheader *gc_allocator_new_page( int pid, int block, int size, int kind, bool varsize ) {
	// increase size based on previously allocated pages
	if( block < 256 ) {
		int num_pages = 0;
		gc_pheader *ph = gc_pages[pid];
		while( ph ) {
			num_pages++;
			ph = ph->next_page;
		}
		while( num_pages > 8 && (size<<1) / block <= GC_PAGE_SIZE ) {
			size <<= 1;
			num_pages /= 3;
		}
	}

	int start_pos = 0;
	int max_blocks = size / block;

	gc_pheader *ph = gc_alloc_page(size, kind, max_blocks);
	gc_allocator_page_data *p = &ph->alloc;

	p->block_size = block;
	p->max_blocks = max_blocks;
	p->sizes = NULL;
	if( p->max_blocks > GC_PAGE_SIZE )
		hl_fatal("Too many blocks for this page");
	if( varsize ) {
		if( p->max_blocks <= 8 )
			p->sizes = (unsigned char*)&p->sizes_ref;
		else {
			p->sizes = ph->base + start_pos;
			start_pos += p->max_blocks;
		}
		MZERO(p->sizes,p->max_blocks);
	}
	int m = start_pos % block;
	if( m ) start_pos += block - m;
	p->first_block = start_pos / block;
	p->next_block = p->first_block;

	ph->next_page = gc_pages[pid];
	gc_pages[pid] = ph;

	return ph;
}

static void *gc_alloc_fixed( int part, int kind ) {
	int pid = (part << PAGE_KIND_BITS) | kind;
	gc_pheader *ph = gc_free_pages[pid];
	gc_allocator_page_data *p;
	unsigned char *ptr;
	while( ph ) {
		p = &ph->alloc;
		if( ph->bmp ) {
			int next = p->next_block;
			while( true ) {
				unsigned int fetch_bits = ((unsigned int*)ph->bmp)[next >> 5];
				int ones = TRAILING_ONES(fetch_bits >> (next&31));
				next += ones;
				if( (next&31) == 0 && ones ) {
					if( next >= p->max_blocks ) {
						p->next_block = next;
						break;
					}
					continue;
				}
				p->next_block = next;
				if( next >= p->max_blocks )
					break;
				goto alloc_fixed;
			}
		} else if( p->next_block < p->max_blocks )
			break;
		ph = ph->next_page;
	}
	if( ph == NULL )
		ph = gc_allocator_new_page(pid, GC_SIZES[part], GC_PAGE_SIZE, kind, false);
alloc_fixed:
	p = &ph->alloc;
	ptr = ph->base + p->next_block * p->block_size;
#	ifdef GC_DEBUG
	{
		int i;
		if( p->next_block < p->first_block || p->next_block >= p->max_blocks )
			hl_fatal("assert");
		if( ph->bmp && (ph->bmp[p->next_block>>3]&(1<<(p->next_block&7))) != 0 )
			hl_fatal("Alloc on marked bit");
		for(i=0;i<p->block_size;i++)
			if( ptr[i] != 0xDD )
				hl_fatal("assert");
	}
#	endif
	p->next_block++;
	gc_free_pages[pid] = ph;
	return ptr;
}

static void *gc_alloc_var( int part, int size, int kind ) {
	int pid = (part << PAGE_KIND_BITS) | kind;
	gc_pheader *ph = gc_free_pages[pid];
	gc_allocator_page_data *p;
	unsigned char *ptr;
	int nblocks = size >> GC_SBITS[part];
loop:
	while( ph ) {
		p = &ph->alloc;
		if( ph->bmp ) {
			int next, avail = 0;
			next = p->next_block;
			if( next + nblocks > p->max_blocks )
				goto skip;
			while( true ) {
				int fid = next >> 5;
				unsigned int fetch_bits = ((unsigned int*)ph->bmp)[fid];
				int bits;
resume:
				bits = TRAILING_ONES(fetch_bits >> (next&31));
				if( bits ) {
					if (avail && part == GC_FIXED_PARTS && avail <= GC_FREELIST_MAX) {
						void** head = gc_free_list[kind];
						ptr = ph->base + ((next - avail) << GC_SBITS[part]);
						*(void**)ptr = head[avail - 1];  // ptr.next = *head
						head[avail - 1] = ptr;           // *head = ptr;
					}
					avail = 0;
					next += bits - 1;
					if( next >= p->max_blocks ) {
						p->next_block = next;
						ph = ph->next_page;
						goto loop;
					}
					if( p->sizes[next] == 0 ) hl_fatal("assert");
					next += p->sizes[next];
					if( next + nblocks > p->max_blocks ) {
						p->next_block = next;
						ph = ph->next_page;
						goto loop;
					}
					if( (next>>5) != fid )
						continue;
					goto resume;
				}
				bits = TRAILING_ZEROES( (next & 31) ? (fetch_bits >> (next&31)) | (1<<(32-(next&31))) : fetch_bits );
				avail += bits;
				next += bits;
				if( next > p->max_blocks ) {
					avail -= next - p->max_blocks;
					next = p->max_blocks;
					if( avail < nblocks ) break;
				}
				if( avail >= nblocks ) {
					p->next_block = next - avail;
					goto alloc_var;
				}
				if( next & 31 ) goto resume;
			}
			p->next_block = next;
		} else if( p->next_block + nblocks <= p->max_blocks )
			break;
skip:
		ph = ph->next_page;
	}
	if( ph == NULL ) {
		int psize = GC_PAGE_SIZE;
		while( psize < size + 1024 )
			psize <<= 1;
		ph = gc_allocator_new_page(pid, GC_SIZES[part], psize, kind, true);
	}
alloc_var:
	p = &ph->alloc;
	ptr = ph->base + p->next_block * p->block_size;
#	ifdef GC_DEBUG
	{
		int i;
		if( p->next_block < p->first_block || p->next_block + nblocks > p->max_blocks )
			hl_fatal("assert");
		for(i=0;i<size;i++)
			if( ptr[i] != 0xDD )
				hl_fatal("assert");
	}
#	endif
	if( ph->bmp ) {
		int bid = p->next_block;
#		ifdef GC_DEBUG
		int i;
		for(i=0;i<nblocks;i++) {
			if( (ph->bmp[bid>>3]&(1<<(bid&7))) != 0 ) hl_fatal("Alloc on marked block");
			bid++;
		}
		bid = p->next_block;
#		endif
		ph->bmp[bid>>3] |= 1<<(bid&7);
	}
	if( nblocks > 1 ) MZERO(p->sizes + p->next_block, nblocks);
	p->sizes[p->next_block] = (unsigned char)nblocks;
	p->next_block += nblocks;
	gc_free_pages[pid] = ph;
	return ptr;
}

static void* gc_freelist_pickup(int size, int part, int kind) {
	if (part == GC_FIXED_PARTS) {           // Currently only for 8-byte blocks
		int nblocks = size >> GC_SBITS[part];
		int index = nblocks - 1;
		if (index < GC_FREELIST_MAX) {
			void** head = gc_free_list[kind];
			void* cur = head[index];
			if (cur) {
				head[index] = *(void**)cur; // *head = cur.next
				gc_pheader *page = GC_GET_PAGE(cur);
				int bid = ((unsigned char*)cur - page->base) >> GC_SBITS[part];
				MZERO(page->alloc.sizes + bid, nblocks);
				page->alloc.sizes[bid] = (unsigned char)nblocks;
				page->bmp[bid >> 3] |= 1 << (bid & 7);
				return cur;
			}
		}
	}
	return NULL;
}

static void gc_allocator_sizes(int *size, int *part, int page_kind) {
	int sz = *size;
	sz += (-sz) & (GC_ALIGN - 1);
	if (sz <= GC_SIZES[GC_FIXED_PARTS - 1] && page_kind != MEM_KIND_FINALIZER) {
		*part = (sz >> GC_ALIGN_BITS) - 1;
		*size = GC_SIZES[*part];
	} else {
		int p;
		for (p = GC_FIXED_PARTS; p < GC_PARTITIONS; p++) {
			int block = GC_SIZES[p];
			int query = sz + ((-sz) & (block - 1));
			if (query < block * 255) {
				*part = p;
				*size = query;
				return;
			}
		}
		hl_error("Required memory allocation too big");
	}
}

static void *gc_allocator_alloc( int size, int part, int page_kind ) {
	return part < GC_FIXED_PARTS ? gc_alloc_fixed(part, page_kind) : gc_alloc_var(part, size, page_kind);
}

static bool is_zero( void *ptr, int size ) {
	static char ZEROMEM[256] = {0};
	unsigned char *p = (unsigned char*)ptr;
	while( size>>8 ) {
		if( memcmp(p,ZEROMEM,256) ) return false;
		p += 256;
		size -= 256;
	}
	return memcmp(p,ZEROMEM,size) == 0;
}

static void gc_flush_empty_pages() {
	int i;
	for(i=0;i<GC_ALL_PAGES;i++) {
		// if page_kind is MEM_KIND_FINALIZER or sizeof(page_block) >= (1<<22)
		if (!((i & MEM_KIND_FINALIZER) == MEM_KIND_FINALIZER || i >= ((GC_PARTITIONS - 1) << PAGE_KIND_BITS)))
			continue;
		gc_pheader *ph = gc_pages[i];
		gc_pheader *prev = NULL;
		while( ph ) {
			gc_allocator_page_data *p = &ph->alloc;
			gc_pheader *next = ph->next_page;
			if( ph->bmp && is_zero(ph->bmp+(p->first_block>>3),((p->max_blocks+7)>>3) - (p->first_block>>3)) ) {
				if( prev )
					prev->next_page = next;
				else
					gc_pages[i] = next;
				if( gc_free_pages[i] == ph )
					gc_free_pages[i] = next;
				gc_free_page(ph, p->max_blocks);
			} else
				prev = ph;
			ph = next;
		}
	}
}

#ifdef GC_DEBUG
static void gc_clear_unmarked_mem() {
	int i;
	for(i=0;i<GC_ALL_PAGES;i++) {
		gc_pheader *ph = gc_pages[i];
		while( ph ) {
			int bid;
			gc_allocator_page_data *p = &ph->alloc;
			for(bid=p->first_block;bid<p->max_blocks;bid++) {
				if( p->sizes && !p->sizes[bid] ) continue;
				int size = p->sizes ? p->sizes[bid] * p->block_size : p->block_size;
				unsigned char *ptr = ph->base + bid * p->block_size;
				if( bid * p->block_size + size > ph->page_size ) hl_fatal("invalid block size");
#				ifdef GC_MEMCHK
				int_val eob = *(int_val*)(ptr + size - HL_WSIZE);
#				ifdef HL_64
				if( eob != 0xEEEEEEEEEEEEEEEE && eob != 0xDDDDDDDDDDDDDDDD )
#				else
				if( eob != 0xEEEEEEEE && eob != 0xDDDDDDDD )
#				endif
					hl_fatal("Block written out of bounds");
#				endif
				if( (ph->bmp[bid>>3] & (1<<(bid&7))) == 0 ) {
					memset(ptr,0xDD,size);
					if( p->sizes ) p->sizes[bid] = 0;
				}
			}
			ph = ph->next_page;
		}
	}
}
#endif

static void gc_call_finalizers(){
	int i;
	for(i=MEM_KIND_FINALIZER;i<GC_ALL_PAGES;i+=1<<PAGE_KIND_BITS) {
		gc_pheader *ph = gc_pages[i];
		while( ph ) {
			int bid;
			gc_allocator_page_data *p = &ph->alloc;
			for(bid=p->first_block;bid<p->max_blocks;bid++) {
				int size = p->sizes[bid];
				if( !size ) continue;
				if( (ph->bmp[bid>>3] & (1<<(bid&7))) == 0 ) {
					unsigned char *ptr = ph->base + bid * p->block_size;
					void *finalizer = *(void**)ptr;
					p->sizes[bid] = 0;
					if( finalizer )
						((void(*)(void *))finalizer)(ptr);
#					ifdef GC_DEBUG
					memset(ptr,0xDD,size*p->block_size);
#					endif
				}
			}
			ph = ph->next_page;
		}
	}
}

static void gc_allocator_before_mark( unsigned char *mark_cur ) {
	int pid;
	for(pid=0;pid<GC_ALL_PAGES;pid++) {
		gc_pheader *p = gc_pages[pid];
		gc_free_pages[pid] = p;
		while( p ) {
			p->bmp = mark_cur;
			p->alloc.next_block = p->alloc.first_block;
			mark_cur += (p->alloc.max_blocks + 7) >> 3;
			p = p->next_page;
		}
	}
	MZERO(gc_free_list, sizeof(gc_free_list));
}

#define gc_allocator_fast_block_size(page,block) \
	(page->alloc.sizes ? page->alloc.sizes[(int)(((unsigned char*)(block)) - page->base) / page->alloc.block_size] * page->alloc.block_size : page->alloc.block_size)

static void gc_allocator_init() {
	if( TRAILING_ONES(0x080003FF) != 10 || TRAILING_ONES(0) != 0 || TRAILING_ONES(0xFFFFFFFF) != 32 )
		hl_fatal("Invalid builtin tl1");
	if( TRAILING_ZEROES((unsigned)~0x080003FF) != 10 || TRAILING_ZEROES(0) != 32 || TRAILING_ZEROES(0xFFFFFFFF) != 0 )
		hl_fatal("Invalid builtin tl0");
}

static int gc_allocator_get_block_id( gc_pheader *page, void *block ) {
	int offset = (int)((unsigned char*)block - page->base);
	if( offset%page->alloc.block_size != 0 )
		return -1;
	int bid = offset / page->alloc.block_size;
	if( page->alloc.sizes && page->alloc.sizes[bid] == 0 ) return -1;
	return bid;
}

#ifdef GC_INTERIOR_POINTERS
static int gc_allocator_get_block_interior( gc_pheader *page, void **block ) {
	int offset = (int)((unsigned char*)*block - page->base);
	int bid = offset / page->alloc.block_size;
	if( page->alloc.sizes ) {
		while( page->alloc.sizes[bid] == 0 ) {
			if( bid == page->alloc.first_block ) return -1;
			bid--;
		}
	}
	*block = page->base + bid * page->alloc.block_size;
	return bid;
}
#endif

static void gc_allocator_after_mark() {
	gc_call_finalizers();
#	ifdef GC_DEBUG
	gc_clear_unmarked_mem();
#	endif
	gc_flush_empty_pages();
}



