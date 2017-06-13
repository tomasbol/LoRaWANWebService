
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
 * Author: Tomas Bolckmans
 */

/* -------------------------------------------------------------------------------------------------------------
 * This virtual LoRa Interface is the connection between het IPv6 stack (LwIP) and the LoRaMAC code.
 * The AppData byte array is used to exchange data between the two layers (L3 <-> L2)
 */

#include "lwip/opt.h"
#include "string.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "netif/ppp/pppoe.h"

#include "netif/virtualloraif.h"


/**
 * In this function, the hardware should be initialized.
 * Called from virtualloraif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this virtualloraif
 */
static void low_level_init(struct netif *netif){
  //struct virtualloraif *virtualloraif = netif->state;


  /* maximum transfer unit
   * */
  netif->mtu = 255;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_LINK_UP;

#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

	/* Do whatever else is needed to initialize interface. */
	struct ip6_addr ip6_global;

//Sensor global adres:  2001:06a8:1d80:0602:00AF:0085:89A1:001F/64  or  2001-6a8-1d80-602-af-85-89a1-1f
	IP6_ADDR_PART( &ip6_global, 0, 0x20, 0x01, 0x06, 0xA8);
	IP6_ADDR_PART( &ip6_global, 1, 0x1D, 0x80, 0x06, 0x02);
	IP6_ADDR_PART( &ip6_global, 2, 0x00, 0xAF, 0x00, 0x85);
	IP6_ADDR_PART( &ip6_global, 3, 0x89, 0xA1, 0x00, 0x1F);

	netif_ip6_addr_set(netif, 1, &ip6_global);
	netif_ip6_addr_set_state(netif,1, IP6_ADDR_PREFERRED);
}



/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this virtualloraif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t low_level_output(struct netif *netif, struct pbuf *p){


#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

  //Copy the payload in de pbuf packet to the AppData byte-array
  	pbuf_copy_partial(p, AppData, p->tot_len, 0);

  	//set total length of the compressed IP packet
  	AppDataSize = p->tot_len;

  	pbuf_free(p);

  /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this virtualloraif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf * low_level_input(struct netif *netif){
  //struct virtualloraif *virtualloraif = netif->state;
  struct pbuf *p;
  //u16_t len;
  uint8_t len;

  /* The size of the packet and put it into the "len" variable. */
  len = AppDataSize;

#if ETH_PAD_SIZE
  len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

  if (p != NULL) {

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif


    p->len = AppDataSize;
    pbuf_take(p, AppData, AppDataSize);  //Put received data from AppData array in pBuf packet

    //acknowledge that packet has been read();
    MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
    if (((u8_t*)p->payload)[0] & 1) {
      /* broadcast or multicast packet*/
      MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
    } else {
      /* unicast packet*/
      MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
    }
#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.recv);
  } else {
    //drop packet();
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
  }

  return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this virtualloraif
 */
void virtualloraif_input(struct netif *netif){
  struct pbuf *p;

  /* put received data in pbuf packet */
  p = low_level_input(netif);

  /* If packet is not null: */
  if (p != NULL) {


//netif->input is a function callback to schc_input and eventually it will be send to ipv6_input
//ipv6_input > udp_input > pcb->recv

    // pass all packets to schc_input
    if (netif->input(p, netif) != ERR_OK) {
      LWIP_DEBUGF(NETIF_DEBUG, ("virtualloraif_input: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
  }
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this virtualloraif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t virtualloraif_init(struct netif *netif){
  //struct virtualloraif *virtualloraif;

  LWIP_ASSERT("netif != NULL", (netif != NULL));

//  virtualloraif = (struct virtualloraif *)mem_malloc(sizeof(struct virtualloraif));
//  if (virtualloraif == NULL) {
//    LWIP_DEBUGF(NETIF_DEBUG, ("virtualloraif_init: out of memory\n"));
//    return ERR_MEM;
//  }

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, 300);

  //netif->state = virtualloraif; //stores struct with information
  netif->name[0] = 'L';
  netif->name[1] = 'o';
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = schc_output;  //Pass down to SCHC compressor
#endif /* LWIP_IPV6 */
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

