#pragma once
#include <stddef.h>
#include <stdint.h>
#define NET_MAX_FRAME 1514u
struct net_endpoint {uint8_t mac[6],ip[4];};
struct net_stats {uint64_t received, rejected, arp, icmp, udp, replies;};
/* Bounded local frame processor: Ethernet II / ARP / IPv4 / ICMP / UDP echo.
 * No physical interface, routing, forwarding, sockets or automatic transmission. */
size_t net_receive(const struct net_endpoint *,const uint8_t *,size_t,uint8_t *,size_t,struct net_stats *);
uint16_t net_checksum(const void *,size_t);
int net_selftest(void);
int net_command(const char *);
