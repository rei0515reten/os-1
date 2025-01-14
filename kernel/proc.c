#include <riscv.h>
#include <os1.h>
#include <types.h>
#include <proc.h>
#include <vm.h>
#include <trap.h>
#include <string.h>
#include <sched.h>
#include <fs.h>
#include <printk.h>
#include <elf.h>
#include <panic.h>

struct proc procs[NPROCS];
u64 mpid = 1;
struct cpu cpus[NCPUS];

void initcpu(void) {
	for(int i = 0; i < NCPUS; i++) {
		cpus[i].rp = NULL;
		memset(&cpus[i].ctx, 0, sizeof(context_t));
	}
}

void initproc(void) {
	mpid = 1;
	for(int i = 0; i < NPROCS; i++) {
		procs[i].stat = UNUSED;
	}
}

extern void init(void);
void userinit(void) {
	u64 pid = newproc();
	kvmmap(procs[pid].pgtbl, 0x0, (u64)init, PAGE_SIZE, PTE_V|PTE_W|PTE_R|PTE_X|PTE_U);
	kvmdump(procs[pid].pgtbl, TRAPFRAME);
	kvmdump(procs[pid].pgtbl, TRAMPOLINE);
	kvmdump(procs[pid].pgtbl, 0x0);
}

int newproc(void) {
	struct proc *p;

	p = &procs[mpid];
	p->stat = RUNNABLE;
	p->pid = mpid;
	p->tf = kalloc();
	p->pgtbl = kalloc();
	p->kstack = kalloc();
	memset(p->tf, 0, sizeof(trapframe_t));
	p->ctx.ra = (u64)usertrapret;
	p->ctx.sp = (u64)(p->kstack+PAGE_SIZE);
	p->tf->satp = SATP(p->pgtbl);
	p->tf->ksp = (u64)(p->kstack + PAGE_SIZE);
	p->tf->trap_handler = (u64)(kerneltrap);
	kvmmap(p->pgtbl, TRAPFRAME, (u64)p->tf, PAGE_SIZE, PTE_V|PTE_W|PTE_R);
	kvmmap(p->pgtbl, TRAMPOLINE, (u64)trampoline, PAGE_SIZE, PTE_V|PTE_X|PTE_R);
	
	return mpid++;
}

int exec(const char *file, char const **argv) {
	struct inode *ip;
	char path[128];
	Elf64_Ehdr ehdr;
	struct proc *rp;
	char *seg;
	Elf64_Phdr *phdr = NULL;

	strcpy(path, file);
	ip = namei((char *)path);
	// TODO: check permission for executable file (rx)
	if(ip == NULL) {
		return -1;
	}
	readi(ip, (char *)&ehdr, 0, sizeof(ehdr));

	if(IS_RISCV_ELF(ehdr) && ehdr.e_type == ET_EXEC) {
		printk("Valid ELF\n");
	} else {
		panic("Invalid ELF\n");
	}

	if(ehdr.e_phnum > 4) {
		panic("exec: load failed\n");
	}
	phdr = kalloc();

	rp = cpus[r_tp()].rp;
	readi(ip, (char *)phdr, ehdr.e_phoff, sizeof(Elf64_Phdr)*ehdr.e_phnum);
	for(int i = 0; i < ehdr.e_phnum; i++) {
		if(phdr[i].p_type == PT_LOAD) {
			printk("PT_LOAD: p_offset: %x, p_vaddr: %x, p_paddr: %x, p_filesz: %x,p_memsz: %x, p_align: %x\n",
					phdr[i].p_offset, phdr[i].p_vaddr, phdr[i].p_paddr, phdr[i].p_filesz, phdr[i].p_memsz, phdr[i].p_align);
			seg = (char *)va2pa(rp->pgtbl, 0x0);
			memset(seg, 0, PAGE_SIZE);
			readi(ip, (char *)seg, phdr[i].p_offset, PAGE_SIZE);
			sfence_vma(); // required?
			break; // TODO: only load first page for segment
		}
	}

	kfree(phdr);

	rp->tf->sepc = ehdr.e_entry;
	rp->tf->sp = PAGE_SIZE;

	return 0;
}

