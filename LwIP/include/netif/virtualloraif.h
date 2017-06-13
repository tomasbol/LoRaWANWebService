#ifndef __VIRTUALLORA_H__
#define __VIRTUALLORA_H__


#include "lwip/err.h"
#include "lwip/netif.h"
#include "netif/schcCompressor.h"


extern uint8_t AppDataSize;
extern uint8_t AppData[240];

extern err_t virtualloraif_init(struct netif *netif);

extern void virtualloraif_input(struct netif *netif);
//static void  virtualloraif_input(struct netif *netif);


#endif
