#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/ptrace.h>

#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <asm/processor.h>
#include <asm/csr.h>
#include <asm/string.h>
#include <asm/switch_to.h>

extern asmlinkage void ret_from_fork(void);
extern asmlinkage void ret_from_kernel_thread(void);

void arch_cpu_idle(void)
{
	wait_for_interrupt();
	local_irq_enable();
}

void show_regs(struct pt_regs *regs)
{
	show_regs_print_info(KERN_DEFAULT);

	printk("sepc: " REG_FMT " ra : " REG_FMT " sp : " REG_FMT "\n",
		regs->sepc, regs->ra, regs->sp);
	printk(" gp : " REG_FMT " tp : " REG_FMT " t0 : " REG_FMT "\n",
		regs->gp, regs->tp, regs->t0);
	printk(" t1 : " REG_FMT " t2 : " REG_FMT " s0 : " REG_FMT "\n",
		regs->t1, regs->t2, regs->s0);
	printk(" s1 : " REG_FMT " a0 : " REG_FMT " a1 : " REG_FMT "\n",
		regs->s1, regs->a0, regs->a1);
	printk(" a2 : " REG_FMT " a3 : " REG_FMT " a4 : " REG_FMT "\n",
		regs->a2, regs->a3, regs->a4);
	printk(" a5 : " REG_FMT " a6 : " REG_FMT " a7 : " REG_FMT "\n",
		regs->a5, regs->a6, regs->a7);
	printk(" s2 : " REG_FMT " s3 : " REG_FMT " s4 : " REG_FMT "\n",
		regs->s2, regs->s3, regs->s4);
	printk(" s5 : " REG_FMT " s6 : " REG_FMT " s7 : " REG_FMT "\n",
		regs->s5, regs->s6, regs->s7);
	printk(" s8 : " REG_FMT " s9 : " REG_FMT " s10: " REG_FMT "\n",
		regs->s8, regs->s9, regs->s10);
	printk(" s11: " REG_FMT " t3 : " REG_FMT " t4 : " REG_FMT "\n",
		regs->s11, regs->t3, regs->t4);
	printk(" t5 : " REG_FMT " t6 : " REG_FMT "\n",
		regs->t5, regs->t6);

	printk("sstatus: " REG_FMT " sbadaddr: " REG_FMT " scause: " REG_FMT "\n",
		regs->sstatus, regs->sbadaddr, regs->scause);
}

void start_thread(struct pt_regs *regs, unsigned long pc, 
	unsigned long sp)
{
	regs->sstatus = SR_PIE /* User mode, irqs on */ | SR_FS_INITIAL;
	regs->sepc = pc;
	regs->sp = sp;
	set_fs(USER_DS);
}

void flush_thread(void)
{
	/* Reset FPU context
	 *	frm: round to nearest, ties to even (IEEE default)
	 *	fflags: accrued exceptions cleared
	 */
	memset(&current->thread.fstate, 0,
		sizeof(struct user_fpregs_struct));
}

int arch_dup_task_struct(struct task_struct *dst, struct task_struct *src)
{
	fstate_save(src, task_pt_regs(src));
	*dst = *src;
	return 0;
}

int copy_thread(unsigned long clone_flags, unsigned long usp,
	unsigned long arg, struct task_struct *p)
{
	struct pt_regs *childregs = task_pt_regs(p);

	/* p->thread holds context to be restored by __switch_to() */
	if (unlikely(p->flags & PF_KTHREAD)) {
		/* Kernel thread */
		const register unsigned long gp __asm__ ("gp");
		memset(childregs, 0, sizeof(struct pt_regs));
		childregs->gp = gp;
		childregs->sstatus = SR_PS | SR_PIE; /* Supervisor, irqs on */

		p->thread.ra = (unsigned long)ret_from_kernel_thread;
		asm volatile ("stag %0, 0(%1)" ::"r"(2), "r"(&(p->thread.ra)));
		p->thread.s[0] = usp; /* fn */
		p->thread.s[1] = arg;
	} else {
		*childregs = *(current_pt_regs());
		if (usp) /* User fork */
			childregs->sp = usp;
		if (clone_flags & CLONE_SETTLS)
			childregs->tp = childregs->a5;
		childregs->a0 = 0; /* Return value of fork() */
		p->thread.ra = (unsigned long)ret_from_fork;
		asm volatile ("stag %0, 0(%1)" ::"r"(2), "r"(&(p->thread.ra)));
	}
	p->thread.sp = (unsigned long)childregs; /* kernel sp */
	return 0;
}
