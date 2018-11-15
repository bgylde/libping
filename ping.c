#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>

#define DATA_SIZE 32

#define pri_debug printf
#define pri_error printf

typedef struct tag_icmp_header
{
    u_int8_t  type;
    u_int8_t  code;
    u_int16_t check_sum;
    u_int16_t id;
    u_int16_t seq;
} icmp_header;

typedef struct tag_iphdr
{
    u_int8_t        ip_head_verlen;
    u_int8_t        ip_tos;
    unsigned short  ip_length;
    unsigned short  ip_id;
    unsigned short  ip_flags;
    u_int8_t        ip_ttl;
    u_int8_t        ip_protacol;
    unsigned short  ip_checksum;
    int             ip_source;
    int             ip_destination;
} ip_header;

unsigned short generation_checksum(unsigned short * buf, int size)
{
    unsigned long cksum = 0;
    while(size > 1)
    {
        cksum += *buf++;
        size -= sizeof(unsigned short);
    }

    if(size)
    {
        cksum += *buf++;
    }

    cksum =  (cksum>>16) + (cksum & 0xffff);
    cksum += (cksum>>16);

    return (unsigned short)(~cksum);
}

double get_time_interval(struct timeval * start, struct timeval * end)
{
    double interval;
    struct timeval tp;

    tp.tv_sec = end->tv_sec - start->tv_sec;
    tp.tv_usec = end->tv_usec - start->tv_usec;
    if(tp.tv_usec < 0)
    {
        tp.tv_sec -= 1;
        tp.tv_usec += 1000000;
    }

    interval = tp.tv_sec * 1000 + tp.tv_usec * 0.001;
    return interval;
}

int ping_host_ip(const char * domain)
{
    int i;
    int ret = -1;
    int client_fd;
    int size = 50 * 1024;
    struct timeval timeout;
    char * icmp;
    in_addr_t dest_ip;
    icmp_header * icmp_head;
    struct sockaddr_in dest_socket_addr;

    if(domain == NULL)
    {
        pri_debug("ping_host_ip domain is NULL!\n");
        return ret;
    }

    dest_ip = inet_addr(domain);
    if(dest_ip == INADDR_NONE)
    {
        struct hostent* p_hostent = gethostbyname(domain);
        if(p_hostent)
        {
            dest_ip = (*(in_addr_t*)p_hostent->h_addr);
        }
    }

    client_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (client_fd == -1)
    {
        pri_error("socket error: %s!\n", strerror(errno));
        return ret;
    }

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
    if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)))
    {
        pri_error("setsocketopt SO_RCVTIMEO error!\n");
        return ret;
    }

    if(setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval)))
    {
        pri_error("setsockopt SO_SNDTIMEO error!\n");
        return ret;
    }

    dest_socket_addr.sin_family = AF_INET;
    dest_socket_addr.sin_addr.s_addr = dest_ip;
    dest_socket_addr.sin_port = htons(0);
    memset(dest_socket_addr.sin_zero, 0, sizeof(dest_socket_addr.sin_zero));

    icmp = (char *)malloc(sizeof(icmp_header) + DATA_SIZE);
    memset(icmp, 0, sizeof(icmp_header) + DATA_SIZE);

    icmp_head = (icmp_header *)icmp;
    icmp_head->type = 8;
    icmp_head->code = 0;
    icmp_head->id = 1;

    pri_debug("PING %s (%s).\n", domain, inet_ntoa(*((struct in_addr*)&dest_ip)));

    for(i = 0; i < 8; i++)
    {
        struct timeval start;
        struct timeval end;
        long result;
        struct sockaddr_in from;
        socklen_t from_packet_len;
        long read_length;
        char recv_buf[1024];

        icmp_head->seq = htons(i);
        icmp_head->check_sum = 0;
        icmp_head->check_sum = generation_checksum((unsigned short*)icmp,
            sizeof(icmp_header) + DATA_SIZE);
        gettimeofday(&start, NULL);
        result = sendto(client_fd, icmp, sizeof(icmp_header) +
            DATA_SIZE, 0, (struct sockaddr *)&dest_socket_addr,
            sizeof(dest_socket_addr));
        if(result == -1)
        {
            pri_debug("PING: sendto: Network is unreachable\n");
            continue;
        }

        from_packet_len = sizeof(from);
        memset(recv_buf, 0, sizeof(recv_buf));
        while(1)
        {
            read_length = recvfrom(client_fd, recv_buf, 1024, 0,
                (struct sockaddr*)&from, &from_packet_len);
            gettimeofday( &end, NULL );

            if(read_length != -1)
            {
                ip_header * recv_ip_header = (ip_header*)recv_buf;
                int ip_ttl = (int)recv_ip_header->ip_ttl;
                icmp_header * recv_icmp_header = (icmp_header *)(recv_buf +
                    (recv_ip_header->ip_head_verlen & 0x0F) * 4);

                if(recv_icmp_header->type != 0)
                {
                    pri_error("error type %d received, error code %d \n", recv_icmp_header->type, recv_icmp_header->code);
                    break;
                }

                if(recv_icmp_header->id != icmp_head->id)
                {
                    pri_error("some else's packet\n");
                    break;
                }

                if(read_length >= (sizeof(ip_header) +
                    sizeof(icmp_header) + DATA_SIZE))
                {
                    pri_debug("%ld bytes from %s (%s): icmp_seq=%d ttl=%d time=%.2f ms\n",
                        read_length, domain, inet_ntoa(from.sin_addr), recv_icmp_header->seq / 256,
                        ip_ttl, get_time_interval(&start, &end));

                    ret = 0;
                    // 证明网络通畅，后续的包已经不许再需要验证
                    goto PING_EXIT;
                }

                break;
            }
            else
            {
                pri_error("receive data error!\n");
                break;
            }
        }
    }

PING_EXIT:
    if(NULL != icmp)
    {
        free(icmp);
        icmp = NULL;
    }

    if(client_fd != -1)
    {
        close(client_fd);
    }

    return ret;
}