/**
 * author: Tomas Bolckmans
 */


#ifndef __SCHCCOMPRESSOR_H__
#define __SCHCCOMPRESSOR_H__


#include "lwip/err.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/ip.h"
#include "lwip/ip6.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "../apps/LoRaMac/classA/SK-iM880A/Comissioning.h"

#include <stdlib.h>
#include <string.h>

#define AMOUNT_OF_FIELDS					    18

struct ipv6_hdr {
   uint8_t version:4; 		//Version: 4 bits
   uint8_t tclass;			//Traffic Class: 8bits
   uint32_t flabel:20;		//Flow label: 20 bits
   uint16_t paylength;		//Payload length: 16bits
   uint8_t nheader;			//Next header: 8bits
   uint8_t hlimit;			//Hop Limit: 8 bits

   struct v6_addr
   {
	   //Split in 4 parts to remain compatible with microchip memory requirements
	   uint32_t Prefix1;
	   uint32_t Prefix2;
	   uint32_t IID1;
	   uint32_t IID2;
   }ip6_src, ip6_dst;
};

//The name udp_hdr was already in use in LwIP
struct udp_schc_hdr {
   uint16_t srcPort;
   uint16_t dstPort;
   uint16_t length;
   uint16_t checksum;
};

typedef enum actions{NOTSENT, VALUESENT, LSB, COMPUTELENGTH, COMPUTECHECKSUM, BUILDIID} CompDecompAction;

struct SCHC_Field{
	uint8_t fieldLength;
	uint8_t msbLength;
	uint32_t targetValue;

	//schc_field: struct
	//uint32_t: headerValue
	//returns boolean
	uint8_t (*matchingOperator)(struct SCHC_Field*, uint32_t);

	//Compression and decompression action
	CompDecompAction action;
};

struct SCHC_Rule{
	uint8_t id;
	struct SCHC_Field fields[AMOUNT_OF_FIELDS];
};

err_t schc_if_init(struct netif *netif);
err_t schc_input(struct pbuf * p, struct netif *netif);
err_t schc_output(struct netif *netif, struct pbuf *p, const ip6_addr_t *ip6addr);
err_t schc_frag(struct pbuf * p, struct netif *netif);
uint8_t schc_compression(uint8_t* schc_buffer);
uint8_t schc_decompression(struct pbuf* p, uint8_t ruleId);


//Helper functions
uint8_t equal(struct SCHC_Field* field, uint32_t headerField);
uint8_t ignore(struct SCHC_Field* field, uint32_t headerField);
uint8_t MSB(struct SCHC_Field* field, uint32_t headerField);


void initializeRules();

#endif
