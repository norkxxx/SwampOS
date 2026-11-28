#pragma once
#include <stddef.h>
#include <stdint.h>
struct tcp_view { uint16_t src,dst,window; uint32_t seq,ack; uint8_t flags; size_t payload; };
struct dns_view { uint16_t id,flags,questions,answers; };
struct dhcp_view { uint8_t type, address[4], server[4]; uint32_t lease; };
/* Validation only: no TCP state machine, DNS resolver or DHCP client. */
int tcp4_parse(const uint8_t *,size_t,struct tcp_view *);
int dns_parse(const uint8_t *,size_t,struct dns_view *);
int dhcp_parse(const uint8_t *,size_t,struct dhcp_view *);
int ipv6_parse(const uint8_t *,size_t,uint8_t *,size_t *);
int transport_selftest(void);
