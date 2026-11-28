#pragma once
#include <stdint.h>
/* Route-decision model only; no tunnel or packet routing is attached. */
enum vpn_mode {VPN_BLOCK=0,VPN_REQUIRED=1,VPN_DIRECT=2};
enum vpn_path {VPN_DROP=0,VPN_TUNNEL=1,VPN_BYPASS=2};
struct vpn_state {uint8_t tunnel_ready,kill_switch;};
enum vpn_path vpn_decide(const struct vpn_state *,enum vpn_mode,int endpoint_granted);
int vpn_selftest(void);
