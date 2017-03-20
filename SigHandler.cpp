

#include "SigHandler.hpp"

#include <stdio.h>
#include <signal.h>
#include <stdio.h>
#include <signal.h>
#include <execinfo.h>
#include <cstdlib>
#include <zconf.h>
#include <ucontext.h>

//http://stackoverflow.com/questions/14014669/system-includes-moved-on-ubuntu-g-cannot-resolve-reg-eip
#ifdef __x86_64__
#define REG_EIP REG_RIP
#endif

// http://stackoverflow.com/questions/3151779/how-its-better-to-invoke-gdb-from-program-to-print-its-stacktrace/4611112#4611112
char* exe = 0;

void initialiseExecutableName()
{
    char link[1024];
    exe = new char[1024];
    snprintf(link,sizeof link,"/proc/%d/exe",getpid());
    if(readlink(link,exe,sizeof link)==-1) {
        fprintf(stderr,"ERRORRRRR\n");
        exit(1);
    }
    printf("Executable name initialised: %s\n",exe);
}

const char* getExecutableName()
{
    if (exe == 0)
        initialiseExecutableName();
    return exe;
}


void bt_sighandler(int sig, siginfo_t *info, void *secret) {

    void *trace[16];
    char **messages = (char **)NULL;
    int i, trace_size = 0;
    ucontext_t *uc = (ucontext_t *)secret;

    /* Do something useful with siginfo_t */
    if (sig == SIGSEGV) {
        printf("Got signal %d, faulty address is %p, from 0x%llx\n", sig, info->si_addr, uc->uc_mcontext.gregs[REG_EIP]);
    } else {
        printf("Got signal %d\n", sig);
    }

    trace_size = backtrace(trace, 16);
    /* overwrite sigaction with caller's address */
    trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];

    messages = backtrace_symbols(trace, trace_size);
    /* skip first stack frame (points here) */
    printf("Execution path:\n");
    for (i=1; i<trace_size; ++i)
    {
        printf("%s\n", messages[i]);

        /* find first occurence of '(' or ' ' in message[i] and assume
         * everything before that is the file name. (Don't go beyond 0 though
         * (string terminator)*/
        size_t p = 0;
        while(messages[i][p] != '(' && messages[i][p] != ' ' && messages[i][p] != 0) {
            ++p;
        }

        char syscom[256];
        sprintf(syscom,"addr2line %p -e %.*s", trace[i], (int)p, messages[i] );
        //last parameter is the filename of the symbol
        system(syscom);

    }
    exit(-1);
}

void initSigSegvHandler()
{
    /* Install our signal handler */
    struct sigaction sa;

    sa.sa_sigaction = bt_sighandler;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_SIGINFO;

    sigaction(SIGSEGV, &sa, NULL);

}
