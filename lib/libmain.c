// Called from entry.S to get us going.
// entry.S already took care of defining envs, pages, uvpd, and uvpt.

#include <inc/lib.h>

extern void umain(int argc, char **argv);

const volatile struct Env *thisenv;
const char *binaryname = "<unknown>";

void
libmain(int argc, char **argv)
{
	// set thisenv to point at our Env structure in envs[].
	// LAB 3: Your code here.

    volatile uint32_t envid = sys_getenvid();
    cprintf("envid = %x\n", envid);

    thisenv = &envs[0];

//    for(; thisenv->env_link != NULL ; thisenv = thisenv->env_link)
    for(uint32_t i = 0; i < NENV ; i++)
    {
        thisenv = &envs[i];
        cprintf("thisenv = %x, thisenv->env_id = %d\n", thisenv, thisenv->env_id);
        if(thisenv->env_id == envid)
        {
            break;
        }
    }

	// save the name of the program so that panic() can use it
	if (argc > 0)
		binaryname = argv[0];

	// call user main routine
	umain(argc, argv);

	// exit gracefully
	exit();
}

