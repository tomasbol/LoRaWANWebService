///
/// @file	 coapClient.c
/// @author	 Tomas Bolckmans
/// @date	 2017-05-18
/// @brief	 CoAP Message Send and Receive
///
/// @details
///

#include <lwip/netdb.h>
#include "coapClient.h"

struct udp_pcb *udp_pcb;
struct ip6_addr ip6_dest;

#define MSG_BUF_LEN 64
uint8_t msg_send_buf[MSG_BUF_LEN];
coap_pdu msg_send = {msg_send_buf, 0, 64};
uint8_t msg_recv_buf[MSG_BUF_LEN];
coap_pdu msg_recv = {msg_recv_buf, 0, 64};

//When it receives a CoAP response
static void coap_input(void *arg, struct udp_pcb *upcb, struct pbuf *p,
                 const ip_addr_t *addr, u16_t port)
{
		/** Eliminates compiler warning about unused arguments (GCC -Wextra -Wunused). */
		LWIP_UNUSED_ARG(arg);

		if (p != NULL) {
		  //Extract CoAP message
		  pbuf_free(p);
		}
}

//When it sends a CoAP request
int coap_output(uint8_t* temp)
{
	err_t err;

	uint8_t* resource = (uint8_t*) "temp";
	uint16_t message_id_counter = (uint16_t)rand();

	// Build Message
	coap_init_pdu(&msg_send);
	coap_set_version(&msg_send, COAP_V1);
	coap_set_type(&msg_send, CT_NON);     //non-confirmable
	coap_set_code(&msg_send, CC_PUT); // PUT to write
	coap_set_mid(&msg_send, message_id_counter++);
	coap_add_option(&msg_send, CON_URI_PATH, resource, 4);

	// coap_add_option(&msg_send, CON_URI_PATH, (uint8_t*)alias, strlen(alias));
	// coap_add_option(&msg_send, CON_URI_QUERY, (uint8_t*)cik, strlen(cik));

	// to write (Put), set payload:
	coap_set_payload(&msg_send, (uint8_t*)temp, 6);
	//coap_set_payload(&msg_send, (uint8_t*)"282929", 6);

	//Create pbuf for the udp/coap message
	struct pbuf *p;
	p = pbuf_alloc(PBUF_TRANSPORT, (u16_t) msg_send.len, PBUF_RAM);
	pbuf_take(p,msg_send.buf,msg_send.len);

	//Pass the pbuf to the transport layer (udp_send)
	err = udp_send(udp_pcb, p);
	if(err != ERR_OK){
		return 1;
	}

	//Free pbuf when packet send.
	pbuf_free(p);

    return 0;
}

//Initialisation of the CoAP interface
void udp_coap_pcpb_init(){

	//Application Server IPv6 Address: 2001:6a8:1d80:2021:230:48ff:fe5a:3ee4
	//This is the IPv6 address of the server that will receive all the sensor (temperature) data.
	IP6_ADDR_PART( &ip6_dest, 0, 0x20, 0x01, 0x06, 0xA8);
	IP6_ADDR_PART( &ip6_dest, 1, 0x1D, 0x80, 0x20, 0x21);
	IP6_ADDR_PART( &ip6_dest, 2, 0x02, 0x30, 0x48, 0xFF);
	IP6_ADDR_PART( &ip6_dest, 3, 0xFE, 0x5A, 0x3E, 0xE4);

	//luister naar elk lokaal ipv6 adres
	err_t err;

	//setup of the UDP port
	udp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
		if (udp_pcb != NULL) {

			//Setup src port: 1086
			err = udp_bind(udp_pcb, IP_ANY_TYPE, 1086);

			if (err == ERR_OK) {
				//UDP port 5683 is default port for CoAP traffic
				err = udp_connect(udp_pcb, &ip6_dest, 5683);

				//Setup function pointer for received udp messages on port 1086
				udp_recv(udp_pcb, coap_input, NULL);

			} else {
				  /* abort */
			}
		} else {
		/* abort */
	}
}
