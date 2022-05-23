#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/soundcard.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>

#define espeakpath "/mnt/sdcard/navit/bin/espeak"
#define IBUFFERLEN 1024
#define MAXARGC 30


int  main(int argc, char *argv[],char *envp[]) {
    int pipefd[2];
    pid_t cpid;
    char buf;
    int co,wp,l,fh;
    short bufi[IBUFFERLEN],bufo[IBUFFERLEN*2];
    int rate=22050;

    char *newargv[MAXARGC+2];

    for(co=0; co<argc; co++) {
        if(co>=MAXARGC)break;
        newargv[co]=argv[co];
    }
    newargv[co++]="--stdout";
    newargv[co++]=NULL;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if(setpriority(PRIO_PROCESS,0,-10))
        perror ("setpriority");

    cpid = fork();
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (cpid == 0) {
        /* Child writes to pipe */

        close(pipefd[0]);          /* Close unused read end */
        dup2(pipefd[1],1);
        execve(espeakpath,newargv,envp);
        perror(espeakpath);
        close(pipefd[1]);          /* Reader will see EOF */
        wait(NULL);                /* Wait for child */
        exit(EXIT_SUCCESS);

    } else { /* Parent read from  pipe */

        close(pipefd[1]);          /* Close unused write end */

        l=read(pipefd[0],bufi,64);
        if(memcmp(bufi,"RIFF",4)) {
            while(l>0) {
                write(1,bufi,l);
                l=read(pipefd[0],bufi,IBUFFERLEN);
            }
            exit(EXIT_SUCCESS);
        }
        l=read(pipefd[0],bufi,IBUFFERLEN);

        fh=open("/dev/dsp",O_WRONLY);
        if(fh<0) {
            perror("open /dev/dsp");
            exit(EXIT_FAILURE);
        }
        ioctl(fh, SNDCTL_DSP_SPEED, &rate);
        ioctl(fh, SNDCTL_DSP_SYNC, 0);
        while(l) {
            for(co=0,wp=0; (co<IBUFFERLEN)&&(co<l); co++) {
                bufo[wp++]=bufi[co]; /* mono->stereo */
                bufo[wp++]=bufi[co];
            }
            write (fh,bufo,wp);
            l=read(pipefd[0],bufi,IBUFFERLEN);
        }
        ioctl(fh, SNDCTL_DSP_SYNC, 0);
        close(pipefd[0]);
        exit(EXIT_SUCCESS);
    }
}
