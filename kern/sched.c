#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>

void sched_halt(void);

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *idle;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// LAB 4: Your code here.
    cprintf("sched_yield start\n");
    idle = NULL;
    struct Env *e = NULL;
    e = thiscpu->cpu_env;
    cprintf("curenv = %x, thiscpu->cpu_env = %x\n", curenv, thiscpu->cpu_env);

    if(e == NULL)
    {
        cprintf("e == NULL\n");
        e = envs;
        cprintf("e = %x\n", e);
        for(uint32_t i = 0; i < NENV; i++)
        {
            e = &envs[i];
            if(e->env_status == ENV_RUNNABLE)
            {
                idle = e;
                cprintf("Find env = %x runnable\n", e);
                break;

            }
            if(e->env_link == NULL)
            {
                cprintf("current has no env runnable\n");
                e = NULL;
                break;
            }
        }
    }
    else
    {
        uint8_t flag = 0;
        cprintf("e = %x\n", e);
        for(uint16_t i = 0; i < NENV; i++)
        {
            struct Env *temp = &envs[i];
            if(temp <= e && flag == 0)
                continue;

            if(temp->env_status == ENV_RUNNABLE)
            {
                idle = temp;
                cprintf("Find env = %x runnable\n", e);
                break;
            }
            
            if(temp == e && flag != 0)
            {
                if(temp->env_status == ENV_RUNNING)
                {
                    cprintf("thiscpu->cpu_env->env_status == ENV_RUNNING\n");
                    idle = temp;
                }
                else
                     cprintf("Have no runnable env\n");

                break;
            }

            if(i + 1 >= NENV)
            {
                flag = 1;
                i = 0xffff;
                cprintf("rest of envs has no runnable, start at the beginning of envs\n");
                continue;
            }
            
//            i++;
        }
/*        while(1)
        {
            if(e == pre && e != NULL)
            {
                cprintf("e == pre && e != NULL\n");
                if(thiscpu->cpu_env->env_status == ENV_RUNNING)
                {
                    cprintf("thiscpu->cpu_env->env_status == ENV_RUNNING\n");
                    idle = pre;
                }
                else
            }

            if(e->env_status == ENV_RUNNABLE)
            {
                idle = e;
                cprintf("Find env = %x runnable\n", e);
                break;
            }
    
            if(e->env_link == NULL)
            {
                e = envs;
                pre = thiscpu->cpu_env;
                cprintf("pre = %x, e = %x, envs = %x\n", pre, e, envs);
                continue;
            }
            e = e->env_link;

        }
*/
    }

    if(e != NULL)
    {
        cprintf("start env_run\n");
        env_run(idle);
    }

    cprintf("sched_halt start\n");
	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		// Uncomment the following line after completing exercise 13
		//"sti\n"
		"1:\n"
		"hlt\n"
		"jmp 1b\n"
	: : "a" (thiscpu->cpu_ts.ts_esp0));
}

