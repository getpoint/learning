/* Parse url to 3 parts: host, port, path. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <error.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PARSE_FAIL -1
#define PARSE_SUCCESS 0
int parse_url(const char *url, char *host, unsigned int *port, char *path)
{
    int url_offset = 0;
    int end_flag = 0;
    char str_port[8] = {0};
    char *p = NULL;
    char *p_host = host;

    if (NULL == url) {
        return PARSE_FAIL;
    }

    if (0 == strncmp(url, "http", strlen("http"))) {
        url_offset += strlen("http");

        if (0 == strncmp(url + url_offset, "://", strlen("://"))) {
            url_offset += strlen("://");
            *port = 80;
        } else if (0 == strncmp(url + url_offset, "s://", strlen("s://"))) {
            url_offset += strlen("s://");
            *port = 443;
        } else {
            // enter here if url like httpabc.com/
            strncpy(p_host, "http", strlen("http"));
            p_host += strlen("http");
            *port = 80;
        }
    } else {
        *port = 80;
    }

    for ( ;url[url_offset] != '\0'; url_offset++) {
        if (':' == url[url_offset]) {
            url_offset += strlen(":");

            if (NULL != (p = strstr((url + url_offset), "/"))) {
                strncpy(str_port, (url + url_offset), (p - (url + url_offset)));
                url_offset = p - url;
            } else {
                strncpy(str_port, (url + url_offset), sizeof(str_port));
                strcpy(path, "/");
                end_flag = 1;
            }

            *port = (unsigned int)strtol(str_port, &p, 10);
            if (p != '\0' || *port > 0xffff) {
                // return if url is wrong like "http://test.com:1a" or "http://test.com:111111111"
                return PARSE_FAIL;
            }

            if (1 == end_flag) {
                // return if url like "http://test.com:12"
                return PARSE_SUCCESS;
            }

            // break if url like "http://test.com:12/abc"
            break;
        } else if ('/' == url[url_offset]) {
            // break if url like "http://test.com/abc
            break;
        }

        *p_host++ = url[url_offset];
    }

    *p_host = '\0';

    if ('\0' != url[url_offset]) {
        strcpy(path, (url + url_offset));
    } else {
        strcpy(path, "/");
    }

    return PARSE_SUCCESS;
}

/* It is not thread-safe if use sockaddr_in.sin_addr.s_addr and then use inet_ntoa to convert, .
 * Use hostname_to_ip to convert hostname is thread-safe.
 * */
int hostname_to_ip(char *hostname, char *ip, size_t len)
{
    struct hostent *he = NULL;

    if (NULL == (he = gethostbyname(hostname))) {
        perror("gethostbyname fail\n");
        return PARSE_FAIL;
    }

    /* h_addr(h_addr_list[0]) is the first ip address of hostname. */
    if (NULL == inet_ntop(he->h_addrtype, he->h_addr, ip, len)) {
        perror("inet_ntop fail\n");
        return PARSE_FAIL;
    }

    printf("hostname: %s, ip: %s\n", hostname, ip);

    return PARSE_SUCCESS;
}

int main(int argc, char *argv[])
{
    char url[256] = {0};
    char host[256] = {0};
    char path[256] = {0};
    unsigned int port = 0;

    if (NULL == argv[1]) {
        while('\0' == *url) {
            printf("Please input the url: ");
            scanf(" %s", url);
        }

        parse_url(url, host, &port, path);
        printf("url: %s, host:%s, port:%u, path:%s\n", url, host, port, path);
    } else {
        parse_url(argv[1], host, &port, path);
        printf("url: %s, host:%s, port:%u, path:%s\n", argv[1], host, port, path);
    }


    return 0;
}
