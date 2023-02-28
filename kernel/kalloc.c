// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// structure for the reference count of the page
struct {
  struct spinlock lock;
  int ref_count[PGROUNDUP(PHYSTOP) / PGSIZE];
} ref_page;

// increase the reference count of the page
void increase_ref_count(void *pa)
{
  acquire(&ref_page.lock);
  ref_page.ref_count[(uint64)pa / PGSIZE]++;
  release(&ref_page.lock);
}

// decrease the reference count of the page
void decrease_ref_count(void *pa)
{
  acquire(&ref_page.lock);
  ref_page.ref_count[(uint64)pa / PGSIZE]--;
  release(&ref_page.lock);
}

// get the reference count of the page
int get_ref_count(void *pa)
{
  acquire(&ref_page.lock);
  int ref_count = ref_page.ref_count[(uint64)pa / PGSIZE];
  release(&ref_page.lock);
  return ref_count;
}

void
kinit()
{
  // initializes all the reference count of the pages to 0
  initlock(&ref_page.lock, "ref_page");
  acquire(&ref_page.lock);
  memset(ref_page.ref_count, 0, sizeof(ref_page.ref_count));
  release(&ref_page.lock);

  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
  {
    // update the reference count of the page to 1
    increase_ref_count((void *)p);

    // now kfree will decrease the reference count of the page to 0 again
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // decrease the reference count of the page
  decrease_ref_count(pa);

  // if the reference count of the page is not 0, then do not free the page
  if (get_ref_count(pa) > 0)
    return;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r)
  {
    memset((char *)r, 5, PGSIZE); // fill with junk
    // increase the reference count of the page
    increase_ref_count((void *)r);
  }

  return (void *)r;
}

// calculate number of free pages
uint64
freePages_count(void)
{
  struct run *r;
  uint64 pgCount = 0;

  acquire(&kmem.lock);
  for(r = kmem.freelist; r; r = r->next)
    pgCount++;
  release(&kmem.lock);

  return pgCount;
}