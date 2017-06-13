///
/// @file	 coapClient.h
/// @author	 Tomas Bolckmans
/// @date	 2017-05-18
/// @brief	 CoAP Message Send and Receive
///
/// @details
///

#ifndef _COAPCLIENT_H_
#define _COAPCLIENT_H_

#include "coap.h"
#include "lwip/opt.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/udp.h"

void udp_coap_pcpb_init();
int coap_output(uint8_t* temp);

#endif /*_COAPCLIENT_H_*/
