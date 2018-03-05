/* arp_send use SOCK_RAW to make ARP packets. arp_recv use SOCK_RAW to recv packet and check if packet is from arp_send.
 * This app is used for check if there is a loop back on the eth0.i interface originally.
 * If there is a loop back, you can recv more than you send.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/time.h>
#include <netinet/ether.h>
#include <sys/types.h>
#include <netpacket/packet.h>
#include <pthread.h>

#define MAC_SOURCE {0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
#define MAC_TARGET {0x00, 0x00, 0x00, 0x00, 0x00, 0x02}
#define MAC_PARAMETERS(mac) mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
#define MAC_FORMAT "%x:%x:%x:%x:%x:%x"
#define IP_SOURCE "1.1.1.1"
#define IP_TARGET "1.1.1.2"
#define IP_ADDR_LEN 4
#define ARP_SEND_TIMES 5
#define LAN_NUM 4
#define LAN_INTERFACE_LEN 8
#define LAN_INTERFACE "eth0"
#define SUCCESS 0
#define FAIL -1
#define LOOP_TRUE SUCCESS
#define LOOP_FALSE FAIL
#define THREAD_ACTIVE SUCCESS
#define THREAD_DEACTIVE FAIL


struct arp_packet {
    struct ether_header header;
    struct ether_arp content;
    // If there is no padding, the arp_packet is 42 bytes, driver will add it to 60 bytes because the packet must more than 60 bytes in ethernet protocol. 
    // Why more than 60 byte? Actually 64 bytes which 4 bytes is FCS in tail. Round-trip time is less than 512 bit time defined by IEEE
    // If there is collistion on the line(nomally less than round-trip time, or called late collistion),  retransmission will not happen if packet is send over.
    // Make packet 64 bytes to make sure packet can be retransmission if there is collision.
    char padding[18];
};

static int g_loop_flag = LOOP_FALSE;
static int g_send_thread_status = THREAD_DEACTIVE;

void fill_ether_header(struct ether_header *header, const u_int8_t *src_mac, const u_int8_t *dst_mac, const u_int16_t type)
{
    memcpy(header->ether_shost, src_mac, ETH_ALEN);
    memcpy(header->ether_dhost, dst_mac, ETH_ALEN);
    header->ether_type = type;
}

int check_ether_header(struct ether_header *header, const u_int8_t *src_mac, const u_int8_t *dst_mac, const u_int16_t type)
{
    if ((SUCCESS != memcmp(header->ether_shost, src_mac, ETH_ALEN)) ||
        (SUCCESS != memcmp(header->ether_dhost, dst_mac, ETH_ALEN))) {
        return FAIL;
    }

    if (type != header->ether_type) {
        printf("header type different, expected type: %x, actual type: %x\n", type, header->ether_type);
        return FAIL;
    }

    return SUCCESS;
}

void fill_arp_content(struct ether_arp *arp_content, const u_int8_t *sender_mac, const u_int8_t *receiver_mac,
    const char *src_ip, const char *dst_ip, int arp_opt)
{
    struct in_addr src_in_addr, dst_in_addr;

    inet_pton(AF_INET, src_ip, &src_in_addr);
    inet_pton(AF_INET, dst_ip, &dst_in_addr);

    arp_content->arp_hrd = htons(ARPHRD_ETHER);
    arp_content->arp_pro = htons(ETHERTYPE_IP);
    arp_content->arp_hln = ETH_ALEN;
    arp_content->arp_pln = IP_ADDR_LEN;
    arp_content->arp_op = htons(arp_opt);
    memcpy(arp_content->arp_sha, sender_mac, ETH_ALEN);
    memcpy(arp_content->arp_tha, receiver_mac, ETH_ALEN);
    memcpy(arp_content->arp_spa, &src_in_addr, IP_ADDR_LEN);
    memcpy(arp_content->arp_tpa, &dst_in_addr, IP_ADDR_LEN);
}

void fill_arp_header(struct ether_header *arp_header, const u_int8_t *src_mac, const u_int8_t *dst_mac)
{
    fill_ether_header(arp_header, src_mac, dst_mac, ETHERTYPE_ARP);
}

int check_arp_header(struct ether_header *arp_header, const u_int8_t *src_mac, const u_int8_t *dst_mac)
{
    return check_ether_header(arp_header, src_mac, dst_mac, ETHERTYPE_ARP);
}

int check_arp_content(struct ether_arp *arp_content, const u_int8_t *sender_mac, const u_int8_t *receiver_mac,
    const char *src_ip, const char *dst_ip, int arp_opt)
{
    struct in_addr src_in_addr, dst_in_addr;

    inet_pton(AF_INET, src_ip, &src_in_addr);
    inet_pton(AF_INET, dst_ip, &dst_in_addr);
    if ((SUCCESS != memcmp(&arp_content->arp_sha, sender_mac, sizeof(arp_content->arp_sha)))
        || (SUCCESS != memcmp(&arp_content->arp_tha, receiver_mac, sizeof(arp_content->arp_tha)))) {
        printf("Content mac different\n");
        return FAIL;
    }

    if ((SUCCESS != memcmp(&arp_content->arp_spa, &src_in_addr, sizeof(arp_content->arp_spa)))
        || (SUCCESS != memcmp(&arp_content->arp_tpa, &dst_in_addr, sizeof(arp_content->arp_tpa)))) {
        printf("Content ip different\n");
        return FAIL;
    }

    printf("Get expected arp: sender_mac: "MAC_FORMAT", receiver_mac: "MAC_FORMAT", src_ip: %s, dst_ip: %s\n",
        MAC_PARAMETERS(sender_mac), MAC_PARAMETERS(receiver_mac), src_ip, dst_ip);

    return SUCCESS;
}

void arp_recv(char *interface)
{
    int sock_raw_fd = 0;
    int recv_bytes = 0;
    int repeat_count = 0;
    struct arp_packet arp_message;
    struct ifreq ethreq;
    u_int8_t sender_mac[ETH_ALEN] = MAC_SOURCE;
    u_int8_t receiver_mac[ETH_ALEN] = MAC_TARGET;
    u_int8_t src_mac[ETH_ALEN] = MAC_SOURCE;
    u_int8_t dst_mac[ETH_ALEN] = MAC_TARGET;

    printf("Recv message at %s\n", interface);

    if (FAIL == (sock_raw_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))) {
        perror("socket");
        goto END;
    }

    strncpy(ethreq.ifr_name, interface, IFNAMSIZ);
    if (FAIL == ioctl(sock_raw_fd, SIOCGIFFLAGS, &ethreq)) {
        perror("ioctl");
        goto END;
    }

    while (1) {
        if (THREAD_DEACTIVE == g_send_thread_status) {
            break;
        }

        bzero(&arp_message, sizeof(struct arp_packet));
        recv_bytes = recv(sock_raw_fd, &arp_message, sizeof(arp_message), 0);

        if (FAIL == recv_bytes) {
            if (EINTR != errno) {
                perror("recvfrom");
                goto END;
            }

            continue;
        }

        if (sizeof(arp_message) != recv_bytes) {
            continue;
        }

        if (SUCCESS != check_arp_header(&arp_message.header, src_mac, dst_mac)) {
            continue;
        }

        if (SUCCESS != check_arp_content(&arp_message.content, sender_mac, receiver_mac,
            IP_SOURCE, IP_TARGET, ARPOP_REPLY)) {
            continue;
        }

        repeat_count++;
        printf("Recv expected arp message %d time(s) at interface: %s\n", repeat_count, interface);

        if (ARP_SEND_TIMES < repeat_count) {
            g_loop_flag = LOOP_TRUE;
            printf("Recv repeat arp, loop at interface: %s\n", interface);
            break;
        }
    }

END:
    if (FAIL != sock_raw_fd) {
        close(sock_raw_fd);
    }

    printf("Recv end\n");
}

void arp_send(char *interface)
{
    int sock_raw_fd = 0;
    int i = 0;
    struct arp_packet arp_message;
    struct ifreq ethreq;
    struct sockaddr_ll sll;
    u_int8_t sender_mac[ETH_ALEN] = MAC_SOURCE;
    u_int8_t receiver_mac[ETH_ALEN] = MAC_TARGET;
    u_int8_t src_mac[ETH_ALEN] = MAC_SOURCE;
    u_int8_t dst_mac[ETH_ALEN] = MAC_TARGET;

    printf("Send arp message at %s %d times\n" , interface, ARP_SEND_TIMES);

    if (FAIL == (sock_raw_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))) {
        perror("socket");
        goto END;
    }

    strncpy(ethreq.ifr_name, interface, IFNAMSIZ);
    if (FAIL == ioctl(sock_raw_fd, SIOCGIFINDEX, &ethreq)) {
        perror("ioctl");
        goto END;
    }

    bzero(&sll, sizeof(sll));
    sll.sll_ifindex = ethreq.ifr_ifindex;
    sll.sll_family = AF_PACKET;

    bzero(&arp_message, sizeof(struct arp_packet));
    fill_arp_header(&arp_message.header, src_mac, dst_mac);
    fill_arp_content(&arp_message.content, sender_mac, receiver_mac, IP_SOURCE, IP_TARGET, ARPOP_REPLY);

    for (i = 1; i <= ARP_SEND_TIMES; i++) {
        if (LOOP_TRUE == g_loop_flag) {
            break;
        }

        if (FAIL == (sendto(sock_raw_fd, &arp_message, sizeof(arp_message), 0, (struct sockaddr *)&sll, sizeof(sll)))) {
            perror("sendto");
            goto END;
        }

        printf("Send arp message %d time(s)\n" , i);
        printf("Send arp message size: %lu\n", sizeof(arp_message));

        sleep(1);
    }

END:
    if (FAIL != sock_raw_fd) {
        close(sock_raw_fd);
    }

    printf("Send end\n");
}

int main(int argc, char *argv[])
{
    int i = 0;
    int loop_interface_count = 0;
    char interface[LAN_INTERFACE_LEN] = {0};
    pthread_t tid_arp_send;
    pthread_t tid_arp_recv;

    for (i = 1; i <= LAN_NUM; i++) {
        g_loop_flag = LOOP_FALSE;
        g_send_thread_status = THREAD_ACTIVE;

        snprintf(interface, sizeof(interface), LAN_INTERFACE".%d", i);
        printf("Send and recv arp at interface %s\n", interface);

        if (SUCCESS != pthread_create(&tid_arp_recv, NULL, (void *) arp_recv, interface)) {
            perror("Create arp_recv thread failed");
            return FAIL;
        }

        if (SUCCESS != pthread_create(&tid_arp_send, NULL, (void *) arp_send, interface)) {
            perror("Create arp_send thread failed");
            return FAIL;
        }

        pthread_join(tid_arp_send, NULL);
        g_send_thread_status = THREAD_DEACTIVE;

        pthread_join(tid_arp_recv, NULL);

        if (LOOP_TRUE != g_loop_flag) {
            break;
        }

        loop_interface_count++;
    }

    if (LAN_NUM == loop_interface_count) {
        printf("ALL interface loop detected!\n");
    }

    return SUCCESS;
}
