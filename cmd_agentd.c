/* Use cmd_agent if there is too many cmd need to run.
 * cmd_agent will add cmd to ca_msg_list temporarily, then ca_msg_handle will run cmd in ca_msg_list one by one.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/socket.h>

#include "link_list.h"

#define CMD_AGENT_ADDR "./ca_msg_sock"
#define CMD_AGENT_DEBUG_FILE "./ca_debug"
#define FAIL -1
#define FREE 0
#define BUSY -1

#define ca_debug(fmt, args...) do {\
    if (access(CMD_AGENT_DEBUG_FILE, F_OK) == 0) { \
        printf("[%s():%d] "fmt, __func__, __LINE__, ##args);\
    }\
} while(0)

struct ca_msg {
    struct link_list list;
    char cmd[256];
};

static struct link_list ca_msg_list;

// mut_ca_msg use for operating ca_msg_list.
static pthread_mutex_t mut_ca_msg = PTHREAD_MUTEX_INITIALIZER;

// mut_sig_cmd and cond_sig_cmd use for waiting until other process send message to cmd_agent.
static pthread_mutex_t mut_sig_ca = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_sig_ca = PTHREAD_COND_INITIALIZER;

int ca_sock_init(void)
{
    struct timeval timeout = {10, 0};
    int sock = FAIL;
    struct sockaddr_un addr;

    if (FAIL == (sock = socket(AF_LOCAL, SOCK_STREAM, 0))) {
        perror("socket");
        goto END;
    }
    if (FAIL == setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) &timeout, sizeof(timeout))) {
        perror("setsockopt");
        goto END;
    }

    unlink(CMD_AGENT_ADDR);
    bzero(&addr, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, CMD_AGENT_ADDR, sizeof(addr.sun_path));

    if (FAIL ==  bind(sock, (struct sockaddr*)&addr, (socklen_t)sizeof(addr))) {
        perror("bind");
        goto END;
    }

    if (FAIL == listen(sock, 64)) {
        perror("listen");
        goto END;
    }

    return sock;

END:
    if (FAIL != sock) {
        close(sock);
    }

    return FAIL;
}

static int ca_msg_handle_status = FREE;
void ca_msg_handle() {
    struct ca_msg *msg;

    while (1) {
        ca_msg_handle_status = BUSY;

        while (1) {
            pthread_mutex_lock(&mut_ca_msg);
            if (NULL != (msg = link_list_get_head(&ca_msg_list, struct ca_msg, list))){
                link_list_del(&ca_msg_list, &msg->list);
                pthread_mutex_unlock(&mut_ca_msg);

                ca_debug("run cmd <%s>\n", msg->cmd);
                system(msg->cmd);

                free(msg);
            } else {
                pthread_mutex_unlock(&mut_ca_msg);
                ca_msg_handle_status = FREE;
                break;
            }
        }

        ca_debug("wait until other process send message to cmd_agent\n");
        pthread_mutex_lock(&mut_sig_ca);
        pthread_cond_wait(&cond_sig_ca, &mut_sig_ca);
        pthread_mutex_unlock(&mut_sig_ca);
        ca_debug("cmd_agent receive message, start handle\n");
    }
}

int ca_request_handle(void *arg)
{
    int ret = -1;
    int conn_fd = *((int *)arg);
    struct ca_msg *msg;
    char buffer[256] = {0};

    while (1) {
        ret = recv(conn_fd, buffer, sizeof(buffer), 0);

        if (0 < ret) {
            break;
        } else if (errno != EINTR) {
            perror("recv");
            goto END;
        }
    }
    ca_debug("recv msg: <%s>\n", buffer);

    if (NULL != (msg = malloc(sizeof(struct ca_msg)))) {
        memset(msg, 0, sizeof(*msg));
        snprintf(msg->cmd, sizeof(msg->cmd), "%s", buffer);

        pthread_mutex_lock(&mut_ca_msg);
        link_list_add_tail(&ca_msg_list, &msg->list);
        pthread_mutex_unlock(&mut_ca_msg);

        ca_debug("add new cmd <%s>\n", buffer);

        // receive message, send cond_sig_ca to ca_msg_handle.
        if (FREE == ca_msg_handle_status) {
            pthread_mutex_lock(&mut_sig_ca);
            pthread_cond_signal(&cond_sig_ca);
            pthread_mutex_unlock(&mut_sig_ca);
        }
    }

END:
    close(conn_fd);

    ca_debug("ca_request_handle end\n");
    return ret;
}

int ca_send_msg(const char *cmd_buf)
{
    int sock = FAIL;
    int ret = -1;
    struct sockaddr_un addr;

    if (FAIL ==  (sock  = socket(AF_LOCAL, SOCK_STREAM, 0))) {
        perror("ca socket fail");
        goto END;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, CMD_AGENT_ADDR, sizeof(addr.sun_path));

    if (FAIL ==  (ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr)))) {
        perror("ca connect fail");
        goto END;
    }

    if (FAIL == (ret = send(sock, cmd_buf, strlen(cmd_buf), 0))) {
        perror("ca send fail");
        goto END;
    }
    ca_debug("ca sent ret: %d!\n", ret);

END:
    if (FAIL != sock) {
        close(sock);
    }

    return ret;
}

int ca_test(int argc, char *argv[])
{
    char *cmd_buf = argv[2];

    return ca_send_msg(cmd_buf);
}

int main(int argc, char *argv[])
{
    int ca_sock = FAIL;
    int conn_fd = FAIL;
    fd_set fset;
    pthread_t tid_msg;
    pthread_t tid_request;
    pthread_attr_t thread_attr;

    if ((1 < argc) && strstr(argv[1], "test")) {
        return ca_test(argc, argv);
    }

    link_list_init(&ca_msg_list);
    if (FAIL == (ca_sock = ca_sock_init())) {
        goto END;
    }

    /* set detach attribute, do not need detach again */
    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    if (0 != pthread_create(&tid_msg, &thread_attr, (void *)ca_msg_handle, NULL)) {
        ca_debug("create ca_msg_handle thread failed\n");
        goto END;
    }
    ca_debug("create ca_msg_handle thread\n");

    while (1) {
        FD_ZERO(&fset);
        FD_SET(ca_sock, &fset);
        if (FAIL == select(ca_sock +1, &fset, NULL, NULL, NULL)) {
            if (errno == EINTR) {
                continue;	
            } else {
                perror("select");
                break;
            }
        }

        if (0 != FD_ISSET(ca_sock, &fset)) {
            if (FAIL == (conn_fd = accept(ca_sock, NULL, 0))) {
                continue;
            }

            if (0 != pthread_create(&tid_request, &thread_attr, (void *)ca_request_handle, (void *)&conn_fd)) {
                ca_debug("create ca_request_handle thread failed\n");
                goto END;
            }
            ca_debug("create ca_request_handle thread\n");
        }
    }

END:
    if (FAIL != conn_fd) {
        close(conn_fd);
    }

    if (FAIL != ca_sock) {
        close(ca_sock);
    }

    return 0;
}
