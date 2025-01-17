#include <riscv.h>
#include <os1.h>
#include <uart.h>
#include <proc.h>
#include <printk.h>
#include <vm.h>
#include <panic.h>
#include <sched.h>
#include <stddef.h>
#include <types.h>

void usertrap(void) {
	asm volatile("nop");
}

int syscall(struct proc *rp);
ssize_t write(int fd, const void *buf, size_t count);

void usertrapret(void) {
	struct proc *p;

	w_stvec(TRAMPOLINE);
	p = cpus[r_tp()].rp;
	p->tf->satp = r_satp();
	p->tf->ksp = (u64)(p->kstack + PAGE_SIZE);
    w_sstatus(((r_sstatus() & ~SSTATUS_SPP) | SSTATUS_SPIE));
	w_sepc(p->tf->sepc);
	
	((void(*)(u64))(TRAMPOLINE + (((u64) userret) - ((u64) trampoline))))((u64)SATP(p->pgtbl));
}

void kerneltrap(void) {
	struct proc *rp;
	rp = cpus[r_tp()].rp;
	u64 scause = r_scause();
	
	switch(scause) {
		case LOAD_PAGE_FAULT: {
			printk("Load page fault: epc:%x, tval:%x\n", r_sepc(), r_stval());
			break;
		}
		case STORE_AMO_PAGE_FAULT: {
			printk("store/amo page fault: epc:%x, tval:%x\n", r_sepc(), r_stval());
			break;
		}
		case INSTRUCTION_PAGE_FAULT: {
			printk("instruction page fault: epc:%x, tval:%x\n", r_sepc(), r_stval());
			break;
		}
		case LOAD_ACCESS_FAULT: {
			uart_puts("load access fault\n");
			break;
		}
#define SMODE_SOFTWARE_INTERRUPT 0x8000000000000001
		case SMODE_SOFTWARE_INTERRUPT: {
			w_sip(0x0);
			rp->stat = RUNNABLE;
			sched(rp);
			break;
		}
#define ECALL_FROM_U_MODE 8
		case ECALL_FROM_U_MODE: {
			rp->tf->sepc+=4;
			rp->tf->a0 = syscall(rp);
			sched(rp);
			break;
		}
		default: {
			printk("trap: fault: cause: %x, epc:%x, tval:%x\n", r_scause(), r_sepc(), r_stval());
			panic("trap");
			break;
		}
	}
	usertrapret();
}

#define SYS_WRITE 64
#define SYS_EXECEV 221
int syscall(struct proc *rp) {
	u64 syscall_num = rp->tf->a7;
	u64 a0 = rp->tf->a0;
	u64 a1 = rp->tf->a1;
	u64 a2 = rp->tf->a2;
	int ret = -1;

	switch(syscall_num) {
		case SYS_WRITE: {
			ret = write(a0, (void *)va2pa(rp->pgtbl, a1), a2);
			break;
		}
		case SYS_EXECEV: {
			ret = exec(((const char *)va2pa(rp->pgtbl, a0)), ((char const **)va2pa(rp->pgtbl, a1)));
			break;
		}
		default: {
			panic("invalid syscall\n");
			break;
		}
	}

	return ret;
}
ssize_t write(int fd, const void *buf, size_t count) {
	u64 i = 0;
	if(fd != 1) {
		return -1;
	}
	for(i = 0; i < count; i++) {
		uart_putchar(((u8 *)buf)[i]);
	}
	return 0;
}
