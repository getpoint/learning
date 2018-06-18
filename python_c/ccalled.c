#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>


typedef struct {
    int c_int;
    char c_list[32];
} C_A;

typedef struct {
    C_A c_a;
    int c_int;
    char c_list[32];
} C_B;

int registe_sighandler(void *handler)
{
    printf("c registe sighandler\n");
    printf("c process id: %d\n", getpid());

    signal(SIGINT, handler);

    return 0;
}

int c_func(int *i, char *c_list, C_B *c_b)
{
    *i = 1;
    c_b->c_int = 2;
    c_b->c_a.c_int = 4;
    strcpy(c_list, "fill words to c_list");
    strcpy(c_b->c_list, "fill words to c_b->c_list");
    strcpy(c_b->c_a.c_list, "fill words to c_b->c_a.c_list");
    return 0;
}

/*
void c_sighandler(int sig_no)
{
    C_B c_b;
    char c_list[32] = {0};
    int i = 0;

    if(sig_no == SIGINT) {
        printf("c get SIGINT\n");
    } else {
        printf("c get other sig_no: %d\n", sig_no);
    }

    memset(&c_b, 0, sizeof(C_B));
    printf("before:\n");
    printf("i: %d\nc_b.c_int:%d\nc_b.c_a.c_int:%d\n",
            i, c_b.c_int, c_b.c_a.c_int);
    printf("c_list: %s\nc_b.c_list: %s\nc_b.c_a.c_list: %s\n",
            c_list, c_b.c_list, c_b.c_a.c_list);
    c_func(&i, c_list, &c_b);
    printf("after:\n");
    printf("i: %d\nc_b.c_int:%d\nc_b.c_a.c_int:%d\n",
            i, c_b.c_int, c_b.c_a.c_int);
    printf("c_list: %s\nc_b.c_list: %s\nc_b.c_a.c_list: %s\n",
            c_list, c_b.c_list, c_b.c_a.c_list);
}

int main(void)
{
    registe_sighandler(c_sighandler);

    while(1) {
        sleep(5);
    }

    return 0;
}
*/
