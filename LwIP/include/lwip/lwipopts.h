/**
 * @file
 *
 * lwIP Options Configuration
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * NOTE: || defined __DOXYGEN__ is a workaround for doxygen bug -
 * without this, doxygen does not see the actual #define
 */

#if !defined LWIP_HDR_LWIPOPTS_H
#define LWIP_HDR_LWIPOPTS_H

/*
 * Include user defined options first. Anything not defined in these files
 * will be set to standard values. Override anything you don't like!
 */
#include "lwip/debug.h"

/**
 * @defgroup lwip_opts Options (lwipopts.h)
 * @ingroup lwip
 *
 * @defgroup lwip_opts_debug Debugging
 * @ingroup lwip_opts
 *
 * @defgroup lwip_opts_infrastructure Infrastructure
 * @ingroup lwip_opts
 *
 * @defgroup lwip_opts_callback Callback-style APIs
 * @ingroup lwip_opts
 *
 * @defgroup lwip_opts_threadsafe_apis Thread-safe APIs
 * @ingroup lwip_opts
 */

 /*
   ------------------------------------
   -------------- NO SYS --------------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_nosys NO_SYS
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * NO_SYS==1: Use lwIP without OS-awareness (no thread, semaphores, mutexes or
 * mboxes). This means threaded APIs cannot be used (socket, netconn,
 * i.e. everything in the 'api' folder), only the callback-style raw API is
 * available (and you have to watch out for yourself that you don't access
 * lwIP functions/structures from more than one context at a time!)
 */
#if !defined NO_SYS || defined __DOXYGEN__
#define NO_SYS                          1
#endif

#if !defined SYS_LIGHTWEIGHT_PROT || defined __DOXYGEN__
#define SYS_LIGHTWEIGHT_PROT            0
#endif


/**
 * @}
 */

/**
 * @defgroup lwip_opts_timers Timers
 * @ingroup lwip_opts_infrastructure
 * @{
 */

/**
 * LWIP_TIMERS==0: Drop support for sys_timeout and lwip-internal cyclic timers.
 * (the array of lwip-internal cyclic timers is still provided)
 * (check NO_SYS_NO_TIMERS for compatibility to old versions)
 */
#if !defined LWIP_TIMERS || defined __DOXYGEN__
#ifdef NO_SYS_NO_TIMERS
#define LWIP_TIMERS                     (!NO_SYS || (NO_SYS && !NO_SYS_NO_TIMERS))
#else
#define LWIP_TIMERS                     0
#endif
#endif


/*
   ------------------------------------
   ---------- Memory options ----------
   ------------------------------------
*/

/**
 * MEM_SIZE: the size of the heap memory. If the application will send
 * a lot of data that needs to be copied, this should be set high.
 */
#if !defined MEM_SIZE || defined __DOXYGEN__
//#define MEM_SIZE                        1510
//#define MEM_SIZE                        1510  //6500
#define MEM_SIZE                        3000  //6500
#endif


/*
   ------------------------------------------------
   ---------- Internal Memory Pool Sizes ----------
   ------------------------------------------------
*/
/**
 * @defgroup lwip_opts_memp Internal memory pools
 * @ingroup lwip_opts_infrastructure
 * @{
 */
/**
 * MEMP_NUM_PBUF: the number of memp struct pbufs (used for PBUF_ROM and PBUF_REF).
 * If the application sends a lot of data out of ROM (or other static memory),
 * this should be set high.
 */
#if !defined MEMP_NUM_PBUF || defined __DOXYGEN__
#define MEMP_NUM_PBUF                   4  //3
#endif

/**
 * MEMP_NUM_TCP_PCB: the number of simultaneously active TCP connections.
 * (requires the LWIP_TCP option)
 */
#if !defined MEMP_NUM_TCP_PCB || defined __DOXYGEN__
#define MEMP_NUM_TCP_PCB                1
#endif



/**
 * MEMP_NUM_FRAG_PBUF: the number of IP fragments simultaneously sent
 * (fragments, not whole packets!).
 * This is only used with LWIP_NETIF_TX_SINGLE_PBUF==0 and only has to be > 1
 * with DMA-enabled MACs where the packet is not yet sent when netif->output
 * returns.
 */
#if !defined MEMP_NUM_FRAG_PBUF || defined __DOXYGEN__
#define MEMP_NUM_FRAG_PBUF              4
#endif


/**
 * PBUF_POOL_SIZE: the number of buffers in the pbuf pool.
 */
#if !defined PBUF_POOL_SIZE || defined __DOXYGEN__
//#define PBUF_POOL_SIZE                  4
#define PBUF_POOL_SIZE                  2   //1
#endif


/*
   ---------------------------------
   ---------- ARP options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_arp ARP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_ARP==1: Enable ARP functionality.
 */
#if !defined LWIP_ARP || defined __DOXYGEN__
#define LWIP_ARP                        0
#endif

/**
 * ARP_TABLE_SIZE: Number of active MAC-IP address pairs cached.
 */
#if !defined ARP_TABLE_SIZE || defined __DOXYGEN__
#define ARP_TABLE_SIZE                  1
#endif

/** the time an ARP entry stays valid after its last update,
 *  for ARP_TMR_INTERVAL = 1000, this is
 *  (60 * 5) seconds = 5 minutes.
 */
#if !defined ARP_MAXAGE || defined __DOXYGEN__
#define ARP_MAXAGE                      300
#endif

/**
 * ARP_QUEUEING==1: Multiple outgoing packets are queued during hardware address
 * resolution. By default, only the most recent packet is queued per IP address.
 * This is sufficient for most protocols and mainly reduces TCP connection
 * startup time. Set this to 1 if you know your application sends more than one
 * packet in a row to an IP address that is not in the ARP cache.
 */
#if !defined ARP_QUEUEING || defined __DOXYGEN__
#define ARP_QUEUEING                    0
#endif

/*
   --------------------------------
   ---------- IP options ----------
   --------------------------------
*/
/**
 * @defgroup lwip_opts_ipv4 IPv4
 * @ingroup lwip_opts
 * @{
 */
/**
 * LWIP_IPV4==1: Enable IPv4
 */
#if !defined LWIP_IPV4 || defined __DOXYGEN__
#define LWIP_IPV4                       0
#endif


/*
   ----------------------------------
   ---------- ICMP options ----------
   ----------------------------------
*/
/**
 * @defgroup lwip_opts_icmp ICMP
 * @ingroup lwip_opts_ipv4
 * @{
 */
/**
 * LWIP_ICMP==1: Enable ICMP module inside the IP stack.
 * Be careful, disable that make your product non-compliant to RFC1122
 */
#if !defined LWIP_ICMP || defined __DOXYGEN__
#define LWIP_ICMP                       0
#endif


/*
   ---------------------------------
   ---------- RAW options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_raw RAW
 * @ingroup lwip_opts_callback
 * @{
 */
/**
 * LWIP_RAW==1: Enable application layer to hook into the IP layer itself.
 */
#if !defined LWIP_RAW || defined __DOXYGEN__
#define LWIP_RAW                        1
#endif


/*
   ---------------------------------
   ---------- TCP options ----------
   ---------------------------------
*/
/**
 * @defgroup lwip_opts_tcp TCP
 * @ingroup lwip_opts_callback
 * @{
 */
/**
 * LWIP_TCP==1: Turn on TCP.
 */
#if !defined LWIP_TCP || defined __DOXYGEN__
#define LWIP_TCP                        0
#endif



/*
   ----------------------------------------------
   ---------- Sequential layer options ----------
   ----------------------------------------------
*/
/**
 * @defgroup lwip_opts_netconn Netconn
 * @ingroup lwip_opts_threadsafe_apis
 * @{
 */
/**
 * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
 */
#if !defined LWIP_NETCONN || defined __DOXYGEN__
#define LWIP_NETCONN                    0
#endif

/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
*/
/**
 * @defgroup lwip_opts_socket Sockets
 * @ingroup lwip_opts_threadsafe_apis
 * @{
 */
/**
 * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
 */
#if !defined LWIP_SOCKET || defined __DOXYGEN__
#define LWIP_SOCKET                     0
#endif


/*
   ---------------------------------------
   ---------- IPv6 options ---------------
   ---------------------------------------
*/
/**
 * @defgroup lwip_opts_ipv6 IPv6
 * @ingroup lwip_opts
 * @{
 */
/**
 * LWIP_IPV6==1: Enable IPv6
 */
#if !defined LWIP_IPV6 || defined __DOXYGEN__
#define LWIP_IPV6                       1
#endif


#endif /* LWIP_HDR_LWIPOPTS_H */
