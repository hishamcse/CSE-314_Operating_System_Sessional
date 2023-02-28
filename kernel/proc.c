#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

struct page {
  uint64 va;      // virtual page number of the page
  int procId;     // id of the process that owns the page
  int age;        // age of the page (used for LRU page replacement policy)
};

struct swap_page {
  uint64 va;      // virtual page number of the page
  int procId;     // id of the process that owns the page
  int age;        // age of the page (used for LRU page replacement policy)
  struct swap *swapRecord; // swap struct of the page
};

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

struct page phys_mem[MAX_PSYC_PAGES]; // array of live pages in physical memory
struct swap_page swap_mem_pages[MAX_TOTAL_PAGES]; // array of pages in swap memory

int num_of_live_pages = 0; // number of live pages in physical memory
int num_of_swap_pages = 0; // number of pages in swap memory

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void
proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    char *pa = kalloc();
    if(pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int) (p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void
procinit(void)
{
  struct proc *p;

  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for(p = proc; p < &proc[NPROC]; p++) {
      initlock(&p->lock, "proc");
      p->state = UNUSED;
      p->kstack = KSTACK((int) (p - proc));
  }
}

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int
cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu*
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc*
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int
allocpid()
{
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) {
      goto found;
    } else {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // Allocate a trapframe page.
  if((p->trapframe = (struct trapframe *)kalloc()) == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if(p->pagetable == 0){
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;

  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if(p->trapframe)
    kfree((void*)p->trapframe);
  p->trapframe = 0;
  if(p->pagetable)
    proc_freepagetable(p->pagetable, p->sz, p->pid);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if(pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if(mappages(pagetable, TRAMPOLINE, PGSIZE,
              (uint64)trampoline, PTE_R | PTE_X, 0) < 0){
    uvmfree(pagetable, 0, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if(mappages(pagetable, TRAPFRAME, PGSIZE,
              (uint64)(p->trapframe), PTE_R | PTE_W, 0) < 0){
    uvmunmap(pagetable, TRAMPOLINE, 1, 0, 0);
    uvmfree(pagetable, 0, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void
proc_freepagetable(pagetable_t pagetable, uint64 sz, int pid)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0, 0);
  uvmfree(pagetable, sz, pid);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
  0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
  0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
  0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
  0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
  0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
  0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Set up first user process.
void
userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;

  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode), p -> pid);
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;      // user program counter
  p->trapframe->sp = PGSIZE;  // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if(n > 0){
    if((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W, p->pid)) == 0) {
      return -1;
    }
  } else if(n < 0){
    sz = uvmdealloc(p->pagetable, sz, sz + n, p -> pid);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  release(&np->lock);

  // Copy user memory from parent to child.
  if(uvmcopy(p->pagetable, np->pagetable, p->sz, np->pid) < 0){
    freeproc(np);
    return -1;
  }

  acquire(&np->lock);
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for(i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void
reparent(struct proc *p)
{
  struct proc *pp;

  for(pp = proc; pp < &proc[NPROC]; pp++){
    if(pp->parent == p){
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void
exit(int status)
{
  struct proc *p = myproc();

  if(p == initproc)
    panic("init exiting");

  // Close all open files.
  for(int fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd]){
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(pp = proc; pp < &proc[NPROC]; pp++){
      if(pp->parent == p){
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if(pp->state == ZOMBIE){
          // Found one.
          pid = pp->pid;
          if(addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                  sizeof(pp->xstate)) < 0) {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || killed(p)){
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock);  //DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&p->lock))
    panic("sched p->lock");
  if(mycpu()->noff != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void
forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first) {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void
wakeup(void *chan)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++) {
    if(p != myproc()){
      acquire(&p->lock);
      if(p->state == SLEEPING && p->chan == chan) {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int
kill(int pid)
{
  struct proc *p;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->killed = 1;
      if(p->state == SLEEPING){
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void
setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int
killed(struct proc *p)
{
  int k;

  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int
either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if(user_dst){
    return copyout(p->pagetable, dst, src, len);
  } else {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int
either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if(user_src){
    return copyin(p->pagetable, dst, src, len);
  } else {
    memmove(dst, (char*)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [USED]      "used",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s", p->pid, state, p->name);
    printf("\n");
  }
}

// swapout a page from the physical memory
void
swapout_page(uint64 va, int pid)
{
  struct page *pg = find_page_to_swap_out_fifo();
  if(pg == 0) {
    printf("swapout_page: no page to swap out found\n");
    return;
  }

  struct proc *p;
  pte_t *pte;
  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->pid == pg -> procId) {
      printf("swapout_page: found pid: %d\n", p->pid);
      pte = walk(p->pagetable, pg->va, 0);
      printf("swapout_page: pte: %p\n", pte);
      if (pte == 0) {
        printf("swapout_page: no physical address found for va: %p\n", pg->va);
        return;
      }

      struct swap *sw = swapalloc();
      printf("swapout_page: sw: %p\n", sw);
      if (sw == 0) {
        printf("swapout_page: no swap space found\n");
        return;
      }

      uint64 pa = PTE2PA(*pte);
      printf("swapout_page: pa: %p\n", pa);
      swapout(sw, (char *)pa);
      printf("swapout_page: swapped out page: %p\n", pa);

      *pte &= ~PTE_V;
      *pte |= PTE_SWAP;

      kfree((void*)pa);

      add_page_to_swap_mem(pg->va, pg->procId, sw);

      remove_page_from_phys_mem(pg->va, pg->procId);
      if(pid != 0) add_page_to_phys_mem(va, pid);

      printf("num_of_live_pages: %d, num_of_swap_pages: %d\n", num_of_live_pages, num_of_swap_pages);

      break;
    }
  }
}

// swap in a page from swap memory to physical memory
void
swapin_page(struct proc *p, uint64 va, pte_t *pte)
{
    printf("swapin_page: va: %p, pid: %d\n", va, p->pid);
    struct swap_page *sw_pg = find_page_from_swap_mem(va, p->pid);
    if(sw_pg == 0) {
      printf("swapin_page: no page to swap in found\n");
      return;
    }

    if(num_of_live_pages >= MAX_PSYC_PAGES) {
      printf("swapin_page: too many live pages in physical memory. so, first swap out\n");
      swapout_page(0, 0);
    }

    uint64 pa = (uint64)kalloc();
    if(pa == 0) {
      printf("swapin_page: no physical memory found\n");
      return;
    }

    swapin((char *)pa, sw_pg->swapRecord);

    // printf("swapin_page: swapped in page: %p\n", pa);
    swapfree(sw_pg->swapRecord);      // free the swap struct

    mappages(p->pagetable, va, PGSIZE, pa, PTE_W|PTE_X|PTE_R|PTE_U, 0);

    // printf("swapin_page: page mapped\n");
    remove_page_from_swap_mem(sw_pg->va, sw_pg->procId);
    add_page_to_phys_mem(sw_pg->va, sw_pg->procId);

    *pte &= ~PTE_SWAP;
    *pte |= PTE_V;
    printf("num_of_live_pages: %d, num_of_swap_pages: %d\n", num_of_live_pages, num_of_swap_pages);
}

// swap in a page from swap memory to physical memory
uint64
swapin_page_pid(int pid, uint64 va, pte_t *pte)
{
    printf("swapin_page_pid: va: %p, pid: %d\n", va, pid);
    struct swap_page *sw_pg = find_page_from_swap_mem(va, pid);
    if(sw_pg == 0) {
      printf("swapin_page_pid: no page to swap in found\n");
      return -1;
    }

    if(num_of_live_pages >= MAX_PSYC_PAGES) {
      printf("swapin_page_pid: too many live pages in physical memory. so, first swap out\n");
      swapout_page(0, 0);
    }

    uint64 pa = (uint64)kalloc();
    if(pa == 0) {
      printf("swapin_page_pid: no physical memory found\n");
      return -1;
    }

    swapin((char *)pa, sw_pg->swapRecord);

    swapfree(sw_pg->swapRecord);      // free the swap struct

    add_page_to_phys_mem(sw_pg->va, sw_pg->procId);
    remove_page_from_swap_mem(sw_pg->va, sw_pg->procId);

    *pte &= ~PTE_SWAP;
    *pte |= PTE_V;

    return pa;
}

// add a live page to the physical memory
void
add_page_to_phys_mem(uint64 va, int pid)
{
  if(num_of_live_pages >= MAX_PSYC_PAGES) {
    printf("add_page_to_phys_mem: too many live pages in physical memory\n");
    swapout_page(va, pid);
    return;
  }

  phys_mem[num_of_live_pages].va = va;
  phys_mem[num_of_live_pages].procId = pid;
  phys_mem[num_of_live_pages].age = 0;

  // printf("add_page_to_phys_mem: va: %p, pid: %d\n", va, pid);

  num_of_live_pages++;
}

// remove a live page from the physical memory
void
remove_page_from_phys_mem(uint64 va, int pid)
{
  int i;
  for(i = 0; i < num_of_live_pages; i++)
  {
    if(phys_mem[i].va == va && phys_mem[i].procId == pid)
    {
      // printf("remove found va: %p, pid: %d\n", va, pid);
      phys_mem[i] = phys_mem[num_of_live_pages - 1];
      num_of_live_pages--;
      return;
    }
  }
}

// add a swap page to the swap memory
void
add_page_to_swap_mem(uint64 va, int pid, struct swap *sw)
{
  printf("add_page_to_swap_mem: va: %p, pid: %d\n", va, pid);
  swap_mem_pages[num_of_swap_pages].va = va;
  swap_mem_pages[num_of_swap_pages].procId = pid;
  swap_mem_pages[num_of_swap_pages].age = 0;
  swap_mem_pages[num_of_swap_pages].swapRecord = sw;

  num_of_swap_pages++;
}

// remove a swap page from the swap memory
void
remove_page_from_swap_mem(uint64 va, int pid)
{
  int i;
  for(i = 0; i < num_of_swap_pages; i++)
  {
    if(swap_mem_pages[i].va == va && swap_mem_pages[i].procId == pid)
    {
      swap_mem_pages[i] = swap_mem_pages[num_of_swap_pages - 1];
      num_of_swap_pages--;
      return;
    }
  }
}

// remove a swap page from the swap memory and free the swap struct
void remove_swap_pg(uint64 va, int pid, pte_t *pte) {
  int i;
  for(i = 0; i < num_of_swap_pages; i++)
  {
    if(swap_mem_pages[i].va == va && swap_mem_pages[i].procId == pid)
    {
      swapfree(swap_mem_pages[i].swapRecord);
      *pte &= ~PTE_SWAP;
      *pte |= PTE_V;
      swap_mem_pages[i] = swap_mem_pages[num_of_swap_pages - 1];
      num_of_swap_pages--;
      return;
    }
  }
}

// find a live page in the physical memory according to LRU algorithm
struct page*
find_page_to_swap_out(void)
{
  int i;
  int min_age = phys_mem[0].age;
  int min_age_index = 0;
  for(i = 1; i < num_of_live_pages; i++)
  {
    if(phys_mem[i].age < min_age)
    {
      min_age = phys_mem[i].age;
      min_age_index = i;
    }
  }
  return &phys_mem[min_age_index];
}

// find a live page in the physical memory according to FIFO(First In First Out) algorithm
struct page*
find_page_to_swap_out_fifo(void)
{
  return &phys_mem[0];
}

// find a page in the swap memory
struct swap_page*
find_page_from_swap_mem(uint64 va, int pid)
{
  int i;
  for(i = 0; i < num_of_swap_pages; i++)
  {
    if(swap_mem_pages[i].va == va && swap_mem_pages[i].procId == pid)
    {
      return &swap_mem_pages[i];
    }
  }
  return 0;
}

// return the number of live pages in the physical memory
int
stats(void)
{
  struct proc *p = myproc();
  uint64 count = 0;

  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->state != UNUSED && p->state != ZOMBIE) {
      int n = count_used_pages(p->pagetable);
      printf("PID: %d, Pages Used: %d\n", p->pid, n);
      count += n;
    }
  }

  printf("num_of_live_pages: %d\n", num_of_live_pages);
  printf("actual count: %d\n", count);

  printf("num_of_swap_pages: %d\n", num_of_swap_pages);

  return count;
}