/* 100 threads cost one number at same time.
 * Mutex objects are intended to serve as a low-level primitive from which
 * other thread synchronization functions can be built.
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define MONEY 100
#define THREAD_NUM MONEY
#define STOP 0
#define START 1

unsigned int g_total_money = MONEY;
unsigned int g_cost_flag = STOP;
pthread_mutex_t cost_mut = PTHREAD_MUTEX_INITIALIZER;

void cost_money()
{
    unsigned int money_tmp = 0;

    while (START != g_cost_flag) {
        continue;
    }

    money_tmp = g_total_money;

    pthread_mutex_lock(&cost_mut);

    g_total_money--;
    printf("Before cost is %u, after cost is %u\n", money_tmp, g_total_money);

    pthread_mutex_unlock(&cost_mut);
}

int main(void)
{
    int i = 0;
    pthread_t tid_cost;
    pthread_attr_t thread_attr;

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    for (i = 0; i < THREAD_NUM; i++) {
        if (0 != pthread_create(&tid_cost, &thread_attr, (void *) cost_money, NULL)) {
            perror("Create cost_money thread failed");
            return -1;
        }
    }

    printf("Wait cost_flag start, press any key\n");
    getchar();
    g_cost_flag = START;

    printf("There is %u money left\n", g_total_money);

    return 0;
}
