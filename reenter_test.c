/* Use thread_cond and list_link.
 * Reenter test: when 2 threads put element to list_link at same time, 
 * check if element is put to list_link sort by time.
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "link_list.h"

struct test_msg {
    LINK_LIST link_list;
    char tid_time[128];
};

LINK_LIST test_msg_link_list;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int put_message(void)
{
    struct timeval tv;
    struct test_msg *msg;

    while (1) {
        if (NULL != (msg =malloc(sizeof(struct test_msg)))) {
            memset(msg, 0, sizeof(*msg));
            gettimeofday(&tv, NULL);
            snprintf(msg->tid_time, sizeof(msg->tid_time), "tid: %lu, time: %ld.%ld", pthread_self(), tv.tv_sec, tv.tv_usec);

            pthread_mutex_lock(&mut);
            link_list_add_tail(&test_msg_link_list, &msg->link_list);
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mut);

            printf("put_message: %s\n", msg->tid_time);
        }

        usleep(1000);
    }
    return 0;
}

int get_message(void)
{
    struct test_msg *msg;

    while (1) {
        pthread_mutex_lock(&mut);
        pthread_cond_wait(&cond, &mut);
        while (NULL != (msg = link_list_get_head(&test_msg_link_list, struct test_msg, link_list))) {
            link_list_del(&test_msg_link_list, &msg->link_list);

            printf("get_message: %s\n", msg->tid_time);

            free(msg);
        }
        pthread_mutex_unlock(&mut);

    }
    return 0;
}

int main(void)
{
    pthread_t tid_put1;
    pthread_t tid_put2;
    pthread_t tid_get1;
    pthread_attr_t thread_attr;

    link_list_init(&test_msg_link_list);

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    if (0 != pthread_create(&tid_get1, &thread_attr, (void *) get_message, NULL)) {
        perror("Create get thread1 failed");
        return -1;
    }

    if ((0 != pthread_create(&tid_put1, &thread_attr, (void *) put_message, NULL))) {
        perror("Create put thread1 failed");
        return -1;
    }
    if (0 != pthread_create(&tid_put2, &thread_attr, (void *) put_message, NULL)) {
        perror("Create put thread2 failed");
        return -1;
    }

    getchar();

    return 0;
}
