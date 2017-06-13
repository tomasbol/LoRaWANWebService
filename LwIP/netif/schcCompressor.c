/**
 *The schcCompressor interface provide the functionality of a SCHC compressor and a decompressor.
 *The proposal of the lpwan IETF WG is used to implement the code.
 *Internet-Draft: https://datatracker.ietf.org/doc/draft-ietf-lpwan-ipv6-static-context-hc/
 *
 *
 * author: Tomas Bolckmans
 */

#include "netif/schcCompressor.h"

uint8_t DevEuiArray[] = LORAWAN_DEVICE_EUI;
struct ipv6_hdr ipv6_header;	//This variable need to be global to support the contrained memory of the sensor
struct udp_schc_hdr udp_header;

//Defined as global variables so they can be changed or updated via an external method.
//For example from the coapClient input method.
struct SCHC_Rule rule1;
struct SCHC_Rule rule2;
struct SCHC_Rule rule3;
struct SCHC_Rule rule4;

struct SCHC_Rule rules[4];



/**
 * Will be called when an compressed IPv6 arrived on the virtualloraif input.
 * This method will defragment the packet if needed and starts decompression of the header fields
 *
 * @param netif The virtualloraif interface which the IP packet will be sent on.
 * @param p The pbuf(s) containing the IP packet to be sent.
 *
 * @return err_t
 */
err_t schc_input(struct pbuf * p, struct netif *netif)
{
	uint8_t ruleId;

	if(!p->payload){
		return ERR_BUF;
	}

	ruleId = (uint8_t) pbuf_get_at(p, 0);

	struct pbuf* q;

	//Packet not compressed
	if(ruleId == 0){
		q = pbuf_alloc(PBUF_IP, p->tot_len-1, PBUF_POOL);

		//place schc_header data in new pbuf
		pbuf_take(q, p->payload+1, p->tot_len-1);
	}

	//packet is compressed, apply decompression
	else{

		//Apply decompression
		uint8_t schc_offset = schc_decompression(p, ruleId);

		//Put the information of the globally used IPv6_header and UDP header struct in a byte array.
		uint8_t buffer[48];

		buffer[0] = ipv6_header.version << 4 | ipv6_header.tclass >> 4;
		buffer[1] = ipv6_header.tclass << 4 | ipv6_header.flabel >> 16;
		buffer[2] = ipv6_header.flabel >> 8;
		buffer[3] = ipv6_header.flabel;
		buffer[4] = ipv6_header.paylength >> 8;
		buffer[5] = ipv6_header.paylength;

		memcpy(&buffer[6],&ipv6_header.nheader,1);
		memcpy(&buffer[7],&ipv6_header.hlimit,1);

		memcpy(&buffer[8],&ipv6_header.ip6_src.Prefix1,4);
		memcpy(&buffer[12],&ipv6_header.ip6_src.Prefix2,4);
		memcpy(&buffer[16],&ipv6_header.ip6_src.IID1,4);
		memcpy(&buffer[20],&ipv6_header.ip6_src.IID2,4);

		memcpy(&buffer[24],&ipv6_header.ip6_dst.Prefix1,4);
		memcpy(&buffer[28],&ipv6_header.ip6_dst.Prefix2,4);
		memcpy(&buffer[32],&ipv6_header.ip6_dst.IID1,4);
		memcpy(&buffer[36],&ipv6_header.ip6_dst.IID2,4);


		//UDP HEADER DATA
		buffer[40] = udp_header.srcPort >> 8;
		buffer[41] = udp_header.srcPort;
		buffer[42] = udp_header.dstPort >> 8;
		buffer[43] = udp_header.dstPort;
		buffer[44] = udp_header.length >> 8;
		buffer[45] = udp_header.length;
		buffer[46] = udp_header.checksum >> 8;
		buffer[47] = udp_header.checksum;

		//Allocate a new packet
		q = pbuf_alloc(PBUF_IP, IP6_HLEN + ipv6_header.paylength, PBUF_POOL);

		//place schc_header data in new pbuf
		pbuf_take(q, buffer, IP6_HLEN + UDP_HLEN);

		//place original data (without ipv6 header) in the pbuf
		pbuf_take_at(q, p->payload + schc_offset, p->tot_len - schc_offset, IP6_HLEN + UDP_HLEN);

	}

	pbuf_free(p);

	return ip6_input(q, netif);
}


/**
 * Will be called when an IPv6 has to be send. This method calls the method that will compress the IPv6 and UDP header.
 *
 * @param netif The virtualloraif interface which the IP packet will be sent on.
 * @param q The pbuf(s) containing the IP packet to be sent.
 * @param ip6addr The IP address of the packet destination.
 *
 * @return err_t
 */
err_t schc_output(struct netif *netif, struct pbuf *p, const ip6_addr_t *ip6addr){

	//char buffer[200];
	uint8_t* buffer;
	buffer = p->payload;

	//Put the IPv6 header in the generaly used IPv6 and UDP header struct format.
    ipv6_header.version = ((buffer[0]) >> 4) & 15; 													// version: bit 0-3
    ipv6_header.tclass = ((buffer[0] << 4) & 240) | ((buffer[1] >> 4) & 15); 						// traffic class: bit 4-11
    ipv6_header.flabel = ((buffer[1] << 16) & 983040) | ((buffer[2] << 8) & 65280) | buffer[3] ; // flow label: bit 12-31

    // header next bytes
    ipv6_header.paylength = (buffer[4] << 8) | buffer[5];
    memcpy(&ipv6_header.nheader,&buffer[6],1);
    memcpy(&ipv6_header.hlimit,&buffer[7],1);

    // source address and destination address
    memcpy(&ipv6_header.ip6_src.Prefix1,&buffer[8],4);
    memcpy(&ipv6_header.ip6_src.Prefix2,&buffer[12],4);
    memcpy(&ipv6_header.ip6_src.IID1,&buffer[16],4);
    memcpy(&ipv6_header.ip6_src.IID2,&buffer[20],4);

    memcpy(&ipv6_header.ip6_dst.Prefix1,&buffer[24],4);
    memcpy(&ipv6_header.ip6_dst.Prefix2,&buffer[28],4);
    memcpy(&ipv6_header.ip6_dst.IID1,&buffer[32],4);
    memcpy(&ipv6_header.ip6_dst.IID2,&buffer[36],4);

    //UDP header
    udp_header.srcPort = (buffer[40] << 8) | buffer[41];
    udp_header.dstPort = (buffer[42] << 8) | buffer[43];
    udp_header.length = (buffer[44] << 8) | buffer[45];
    udp_header.checksum = (buffer[46] << 8) | buffer[47];


    //allocate buffer for maximum 40 bytes
    uint8_t schc_header[40];

    uint8_t schc_offset = schc_compression(schc_header);

    struct pbuf *p_compressed;

    //packet is not compressed, keep IPv6 header
    if(schc_header[0] == 0){
    	p_compressed = pbuf_alloc(PBUF_RAW, schc_offset, PBUF_RAM);
    	pbuf_take(p_compressed, schc_header, schc_offset);

    	//Concatenate the ruleId with the original data (IPv6 header included)
    	pbuf_cat(p_compressed, p);
    	//pbuf_chain(p_compressed, p);
    }

    //packet is compressed, strip IPv6 header
    //UDP support will be added later
    else{

    	//UDP header will be stripped later
    	//uint16_t data_length = p->tot_len - IP6_HLEN;
    	uint16_t data_length = p->tot_len - IP6_HLEN - UDP_HLEN;

    	uint16_t compressed_packet_length = data_length + schc_offset;

    	p_compressed = pbuf_alloc(PBUF_RAW, compressed_packet_length, PBUF_RAM);

    	//place schc_header data in new pbuf
    	pbuf_take(p_compressed, schc_header, schc_offset);

    	//place original data (without ipv6 header) in the pbuf
    	pbuf_take_at(p_compressed, p->payload+IP6_HLEN+UDP_HLEN, data_length, schc_offset);
    }


    pbuf_free(p);
    mem_free(schc_header);


	return schc_frag(p_compressed, netif);
}

/**
 * Arg1: pointer to schc_buffer
 * Returns: the SCHC_offset
 */
uint8_t schc_compression(uint8_t* schc_buffer){


	// offset bytes for the return buffer (after rule_id)
	uint16_t schc_offset = 1;

	//First look for a matching rule, after that create the schc_buffer with the information of that rule.
	uint8_t match = 0;
	uint8_t ruleId = 0;

	while(match != 1 && ruleId<1){

		//IPv6 Version
		if(!rules[ruleId].fields[0].matchingOperator(&rules[ruleId].fields[0], ipv6_header.version)){
			ruleId++;
			continue;
		}


		//Traffic Class
		if(!rules[ruleId].fields[1].matchingOperator(&rules[ruleId].fields[1], ipv6_header.tclass)){
			ruleId++;
			continue;
		}

		//Flow Label
		if(!rules[ruleId].fields[2].matchingOperator(&rules[ruleId].fields[2], ipv6_header.flabel)){
			ruleId++;
			continue;
		}

		//Payload length
		if(!rules[ruleId].fields[3].matchingOperator(&rules[ruleId].fields[3], ipv6_header.paylength)){
			ruleId++;
			continue;
		}

		//Next Header
		if(!rules[ruleId].fields[4].matchingOperator(&rules[ruleId].fields[4], ipv6_header.nheader)){
			ruleId++;
			continue;
		}

		//Hop Limit
		if(!rules[ruleId].fields[5].matchingOperator(&rules[ruleId].fields[5], ipv6_header.hlimit)){
			ruleId++;
			continue;
		}

		//IPv6 src prefix
		if(!rules[ruleId].fields[6].matchingOperator(&rules[ruleId].fields[6], ipv6_header.ip6_src.Prefix1) ||
				!rules[ruleId].fields[7].matchingOperator(&rules[ruleId].fields[7], ipv6_header.ip6_src.Prefix2)){
			ruleId++;
			continue;
		}

		//IPv6 src IID
		if(!rules[ruleId].fields[8].matchingOperator(&rules[ruleId].fields[8], ipv6_header.ip6_src.IID1) ||
				!rules[ruleId].fields[9].matchingOperator(&rules[ruleId].fields[9], ipv6_header.ip6_src.IID2)){
			ruleId++;
			continue;
		}

		//IPv6 dst prefix
		if(!rules[ruleId].fields[10].matchingOperator(&rules[ruleId].fields[10], ipv6_header.ip6_dst.Prefix1) ||
				!rules[ruleId].fields[11].matchingOperator(&rules[ruleId].fields[11], ipv6_header.ip6_dst.Prefix2)){
			ruleId++;
			continue;
		}

		//IPv6 dst IID
		if(!rules[ruleId].fields[12].matchingOperator(&rules[ruleId].fields[12], ipv6_header.ip6_dst.IID1) ||
				!rules[ruleId].fields[13].matchingOperator(&rules[ruleId].fields[13], ipv6_header.ip6_dst.IID2)){
			ruleId++;
			continue;
		}

		/* UDP HEADER  */

		//Src Port
		if(!rules[ruleId].fields[14].matchingOperator(&rules[ruleId].fields[14], udp_header.srcPort)){
			ruleId++;
			continue;
		}

		//Dst Port
		if(!rules[ruleId].fields[15].matchingOperator(&rules[ruleId].fields[15], udp_header.dstPort)){
			ruleId++;
			continue;
		}

		//Length
		if(!rules[ruleId].fields[16].matchingOperator(&rules[ruleId].fields[16], udp_header.length)){
			ruleId++;
			continue;
		}

		//Checksum
		if(!rules[ruleId].fields[17].matchingOperator(&rules[ruleId].fields[17], udp_header.checksum)){
			ruleId++;
			continue;
		}

		match = 1;
	}

	//Add rule_id to front of the schc_buffer
	if(match){
		int byteLength;
		int fieldId;
		for(fieldId = 0 ; fieldId < AMOUNT_OF_FIELDS; fieldId++){
			switch(rules[ruleId].fields[fieldId].action){
				case NOTSENT:
					//No action needed
					break;
				case VALUESENT:
					byteLength = rules[ruleId].fields[fieldId].fieldLength % 8 == 0 ? rules[ruleId].fields[fieldId].fieldLength / 8 : (rules[ruleId].fields[fieldId].fieldLength / 8) + 1;
					memcpy(&schc_buffer[schc_offset], &(rules[ruleId].fields[fieldId].targetValue), byteLength);
					schc_offset += byteLength;
					break;
				case LSB:
					//Not implemented yet
					break;
				case COMPUTELENGTH:
					//No action needed
					break;
				case COMPUTECHECKSUM:
					//No action needed
					break;
				case BUILDIID:
					//No action needed
					break;
			}
		}

		schc_buffer[0] = ruleId + 1;  //RuleID 0 is reserved to indicate that the packet is not compressed

	}
	else{
		//Non off the rules matched, don't compress packet
		schc_buffer[0] = 0;
	}

	return schc_offset;
}


uint8_t schc_decompression(struct pbuf* p, uint8_t ruleId){
	//RuleId numbering starts with 0
	//but in the schc_header 0 is reserved to indicate a non compressed packet
	ruleId--;

	//Start reading bytes from offset 1
	uint8_t schc_offset = 1;
	uint8_t fieldId = 0; //This is the field iterator

	/* Set IPv6 version: this is always the same value, no check is needed*/
	ipv6_header.version = rules[ruleId].fields[fieldId].targetValue;
	fieldId++;

	/* Set IPv6 Traffic Class: depends on the variation of the DiffServ field*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.tclass = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			ipv6_header.tclass = pbuf_get_at(p, schc_offset);
			schc_offset += 1;

			break;
		case LSB:
			ipv6_header.tclass = (rules[ruleId].fields[fieldId].targetValue << (rules[ruleId].fields[fieldId].fieldLength - rules[ruleId].fields[fieldId].msbLength)) || pbuf_get_at(p, schc_offset);
			schc_offset += 1;

			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 Flow Label: depends on the variation of the field*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.flabel = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			ipv6_header.flabel = (pbuf_get_at(p, schc_offset) << 20 & 983040) | ((pbuf_get_at(p, schc_offset+1) << 16) & 65280) | (pbuf_get_at(p, schc_offset+2));
			schc_offset += 3;

			break;
		case LSB:
			//MSB(length) + LSB (8-length)

			//Klopt nog niet helemaal want 20 bits, dus de pbuf_get_at kan meer dan 1 keer nodig zijn afhankelijk van msbLength
			ipv6_header.flabel = (rules[ruleId].fields[fieldId].targetValue << (rules[ruleId].fields[fieldId].fieldLength - rules[ruleId].fields[fieldId].msbLength)) || pbuf_get_at(p, schc_offset);

			//Calculate amount of bytes used for the LSB(20-msbLength)
			schc_offset += (rules[ruleId].fields[fieldId].fieldLength - rules[ruleId].fields[fieldId].msbLength) % 8 == 0 ?
					(rules[ruleId].fields[fieldId].fieldLength - rules[ruleId].fields[fieldId].msbLength) / 8 : ((rules[ruleId].fields[fieldId].fieldLength - rules[ruleId].fields[fieldId].msbLength) / 8) + 1;

			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 Length: taken from the compressed header, length calculation must be done at the end of this method*/
	switch(rules[ruleId].fields[fieldId].action){
		case VALUESENT:
			ipv6_header.paylength = (pbuf_get_at(p, schc_offset) << 16 & 65280) | (pbuf_get_at(p, schc_offset+1) & 255);
			schc_offset += 2;

			break;
		default:
			break;
	}
	/* Set IPv6 Length: needs to be calculated at the end!*/
	fieldId++;


	/* Set IPv6 Next Header: depends on the variation of the field*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.nheader = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			ipv6_header.nheader = pbuf_get_at(p, schc_offset);
			schc_offset += 1;

			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 Hop Limit: depends on the variation of the field*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.hlimit = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			ipv6_header.hlimit = pbuf_get_at(p, schc_offset);
			schc_offset += 1;

			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 srcAdd Prefix1: depends on the variation of the field*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.ip6_src.Prefix1 = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			//Little endian
			ipv6_header.ip6_src.Prefix1 = pbuf_get_at(p, schc_offset)
											| (pbuf_get_at(p, schc_offset+1) << 8)
											| (pbuf_get_at(p, schc_offset+2) << 16)
											| (pbuf_get_at(p, schc_offset+3) << 24);
			schc_offset += 4;
			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 srcAdd Prefix2: depends on the variation of the field*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.ip6_src.Prefix2 = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			//Little endian
			ipv6_header.ip6_src.Prefix2 = pbuf_get_at(p, schc_offset)
											| (pbuf_get_at(p, schc_offset+1) << 8)
											| (pbuf_get_at(p, schc_offset+2) << 16)
											| (pbuf_get_at(p, schc_offset+3) << 24);
			schc_offset += 4;
			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 srcAdd IID1: can be calculated from LoRaWAN Dev EUI*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.ip6_src.IID1 = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			//Little endian
			ipv6_header.ip6_src.IID1 = pbuf_get_at(p, schc_offset)
											| (pbuf_get_at(p, schc_offset+1) << 8)
											| (pbuf_get_at(p, schc_offset+2) << 16)
											| (pbuf_get_at(p, schc_offset+3) << 24);
			schc_offset += 4;
			break;
		case BUILDIID:
			//Little endian
			ipv6_header.ip6_src.IID1 = DevEuiArray[0]
											| (DevEuiArray[1] << 8)
											| (DevEuiArray[2] << 16)
											| (DevEuiArray[3] << 24);
			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 srcAdd IID2: can be calculated from LoRaWAN Dev EUI*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.ip6_src.IID2 = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			//Little endian
			ipv6_header.ip6_src.IID2 = pbuf_get_at(p, schc_offset)
											| (pbuf_get_at(p, schc_offset+1) << 8)
											| (pbuf_get_at(p, schc_offset+2) << 16)
											| (pbuf_get_at(p, schc_offset+3) << 24);
			schc_offset += 4;
			break;
		case BUILDIID:
			//Little endian
			ipv6_header.ip6_src.IID2 = DevEuiArray[4]
											| (DevEuiArray[5] << 8)
											| (DevEuiArray[6] << 16)
											| (DevEuiArray[7] << 24);
			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 dstAdd Prefix1: depends on the variation of the field*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.ip6_dst.Prefix1 = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			//Little endian
			ipv6_header.ip6_dst.Prefix1 = pbuf_get_at(p, schc_offset)
											| (pbuf_get_at(p, schc_offset+1) << 8)
											| (pbuf_get_at(p, schc_offset+2) << 16)
											| (pbuf_get_at(p, schc_offset+3) << 24);
			schc_offset += 4;
			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 dstAdd Prefix2: depends on the variation of the field*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.ip6_dst.Prefix2 = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			//Little endian
			ipv6_header.ip6_dst.Prefix2 = pbuf_get_at(p, schc_offset)
											| (pbuf_get_at(p, schc_offset+1) << 8)
											| (pbuf_get_at(p, schc_offset+2) << 16)
											| (pbuf_get_at(p, schc_offset+3) << 24);
			schc_offset += 4;
			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 dstAdd IID1: depends on the variation of the field*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.ip6_dst.IID1 = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			//Little endian
			ipv6_header.ip6_dst.IID1 = pbuf_get_at(p, schc_offset)
											| (pbuf_get_at(p, schc_offset+1) << 8)
											| (pbuf_get_at(p, schc_offset+2) << 16)
											| (pbuf_get_at(p, schc_offset+3) << 24);
			schc_offset += 4;
			break;
		case BUILDIID:
			//Little endian
			ipv6_header.ip6_dst.IID1 = DevEuiArray[0]
											| (DevEuiArray[1] << 8)
											| (DevEuiArray[2] << 16)
											| (DevEuiArray[3] << 24);
			break;
		default:
			break;
	}
	fieldId++;

	/* Set IPv6 dstAdd IID2: depends on the variation of the field*/
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			ipv6_header.ip6_dst.IID2 = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			//Little endian
			ipv6_header.ip6_dst.IID2 = pbuf_get_at(p, schc_offset)
											| (pbuf_get_at(p, schc_offset+1) << 8)
											| (pbuf_get_at(p, schc_offset+2) << 16)
											| (pbuf_get_at(p, schc_offset+3) << 24);
			schc_offset += 4;
			break;
		case BUILDIID:
			//Little endian
			ipv6_header.ip6_dst.IID2 = DevEuiArray[4]
											| (DevEuiArray[5] << 8)
											| (DevEuiArray[6] << 16)
											| (DevEuiArray[7] << 24);
			break;
		default:
			break;
	}
	fieldId++;

	//#### UDP HEADER

	/* Set UDP Src Port: from target value or from value in schc header   */
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			udp_header.srcPort = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			udp_header.srcPort = pbuf_get_at(p, schc_offset);
			schc_offset += 1;

			break;
		default:
			break;
	}
	fieldId++;

	/* Set UDP Dst Port: from target value or from value in schc header   */
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			udp_header.dstPort = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			udp_header.dstPort = pbuf_get_at(p, schc_offset);
			schc_offset += 1;

			break;
		default:
			break;
	}
	fieldId++;

	/* Set UDP Length: taken from the compressed header, length calculation must be done at the end.*/
	switch(rules[ruleId].fields[fieldId].action){
		case VALUESENT:
			udp_header.length = (pbuf_get_at(p, schc_offset) << 8) | (pbuf_get_at(p, schc_offset+1));
			schc_offset += 2;

			break;

		default:
			break;
	}

	fieldId++;

	/* Set UDP Checksum: from target value or from value in schc header   */
	switch(rules[ruleId].fields[fieldId].action){
		case NOTSENT:
			udp_header.checksum = rules[ruleId].fields[fieldId].targetValue;
			break;
		case VALUESENT:
			udp_header.checksum = pbuf_get_at(p, schc_offset);
			schc_offset += 1;
			break;
		case COMPUTECHECKSUM:
			udp_header.checksum = 0x0000; //LoRaWAN frame Layer 2 already uses MIC and/or CRC so UDP checksum can be omitted.
			break;

		default:
			break;
	}


	/* Set IPv6 Length: can be calculated or taken from the compressed header*/
	switch(rules[ruleId].fields[3].action){
		case COMPUTELENGTH:
			//Length of received compressed packet - 1(rule ID) - schc header
			ipv6_header.paylength = (uint8_t) p->tot_len + UDP_HLEN - schc_offset;
			break;
		default:
			break;
	}

	/* Set UDP Length: can be calculated or taken from the compressed header*/
	switch(rules[ruleId].fields[16].action){
		case COMPUTELENGTH:
			//Length of received compressed packet - 1(rule ID) - schc header
			udp_header.length = (uint8_t) p->tot_len + UDP_HLEN - schc_offset;
			break;

		default:
			break;
	}

	return schc_offset;
}

/**
 * This method fragments the packet in pieces if needed.
 *
 * @param netif The virtualloraif interface which the IP packet will be sent on.
 * @param q The pbuf(s) containing the IP packet to be sent.
 *
 * @return err_t
 */
err_t schc_frag(struct pbuf * p, struct netif *netif){

	//enkel fragmenteren als het nodig is...

	return netif->linkoutput(netif, p);     //Geeft door aan low_level_output v/d loraninterface
}

//Matching Operator: equal
uint8_t equal(struct SCHC_Field* field, uint32_t headerField) {
    return (field->targetValue == headerField);
}

//Matching Operator: ignore
uint8_t ignore(struct SCHC_Field* field, uint32_t headerField) {
    return 1;
}

//Most Significatn Bits, only 'msbLength' amount of MSB need to be compared.
uint8_t MSB(struct SCHC_Field* field, uint32_t headerField) {
	return ((field->targetValue >> (field->fieldLength - field->msbLength)) == (headerField >> (field->fieldLength -  field->msbLength)));
}



/**
 * Should be called at the beginning of the program to set up the
 * SCHC network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this virtualloraif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t schc_if_init(struct netif *netif){
  netif->name[0] = 'S';
  netif->name[1] = 'C';

  netif->output_ip6 = schc_output;  /* Deze methode stuurt gewoon het pakket naar netif->linkoutput, ik behoud ze voor compatibiliteit met de library  */

  initializeRules();

  return ERR_OK;
}

//This method initialize the Static Context rules at the startup of the device
//Since it's static, it only needs to initialize one time.
void initializeRules(){
	/*  Rule1     */
	struct SCHC_Field r1Version;
	r1Version.fieldLength = 4;
	r1Version.targetValue = 6;
	r1Version.matchingOperator = &equal;
	r1Version.action = NOTSENT;

	struct SCHC_Field r1TClass;
	r1TClass.fieldLength = 8;
	r1TClass.targetValue = 0;
	r1TClass.matchingOperator = &equal;
	r1TClass.action = NOTSENT;

	struct SCHC_Field r1FLabel;
	r1FLabel.fieldLength = 20;
	r1FLabel.targetValue = 0;
	r1FLabel.matchingOperator = &equal;
	r1FLabel.action = NOTSENT;

	struct SCHC_Field r1Length;
	r1Length.fieldLength = 16;
	r1Length.targetValue = 0;
	r1Length.matchingOperator = &ignore;
	r1Length.action = COMPUTELENGTH;

	struct SCHC_Field r1NHeader;
	r1NHeader.fieldLength = 8;
	r1NHeader.targetValue = 17;  //UDP
	r1NHeader.matchingOperator = &equal;
	r1NHeader.action = NOTSENT;

	struct SCHC_Field r1HLimit;
	r1HLimit.fieldLength = 8;
	r1HLimit.targetValue = 255;
	r1HLimit.matchingOperator = &ignore;
	r1HLimit.action = NOTSENT;

	struct SCHC_Field r1IP6SrcPrefix1;
	r1IP6SrcPrefix1.fieldLength = 32;
	r1IP6SrcPrefix1.targetValue = 0xA8060120;	//Little-endian
	r1IP6SrcPrefix1.matchingOperator = &equal;
	r1IP6SrcPrefix1.action = NOTSENT;

	struct SCHC_Field r1IP6SrcPrefix2;
	r1IP6SrcPrefix2.fieldLength = 32;
	r1IP6SrcPrefix2.targetValue = 0x0206801D;    //Little-endian
	r1IP6SrcPrefix2.matchingOperator = &equal;
	r1IP6SrcPrefix2.action = NOTSENT;

	struct SCHC_Field r1IP6SrcIID1;
	r1IP6SrcIID1.fieldLength = 0;
	r1IP6SrcIID1.targetValue = 0;
	r1IP6SrcIID1.matchingOperator = &ignore;
	r1IP6SrcIID1.action = BUILDIID;

	struct SCHC_Field r1IP6SrcIID2;
	r1IP6SrcIID2.fieldLength = 0;
	r1IP6SrcIID2.targetValue = 0;
	r1IP6SrcIID2.matchingOperator = &ignore;
	r1IP6SrcIID2.action = BUILDIID;

	struct SCHC_Field r1IP6DestPrefix1;
	r1IP6DestPrefix1.fieldLength = 32;
	r1IP6DestPrefix1.targetValue = 0xA8060120;		//Little-endian
	r1IP6DestPrefix1.matchingOperator = &equal;
	r1IP6DestPrefix1.action = NOTSENT;

	struct SCHC_Field r1IP6DestPrefix2;
	r1IP6DestPrefix2.fieldLength = 32;
	r1IP6DestPrefix2.targetValue = 0x2120801D;	//Little-endian
	r1IP6DestPrefix2.matchingOperator = &equal;
	r1IP6DestPrefix2.action = NOTSENT;

	struct SCHC_Field r1IP6DestIID1;
	r1IP6DestIID1.fieldLength = 32;
	r1IP6DestIID1.targetValue = 0xFF483002;			//Little-endian
	r1IP6DestIID1.matchingOperator = &equal;
	r1IP6DestIID1.action = NOTSENT;

	struct SCHC_Field r1IP6DestIID2;
	r1IP6DestIID2.fieldLength = 32;
	r1IP6DestIID2.targetValue = 0xE43E5AFE;			//Little-endian
	r1IP6DestIID2.matchingOperator = &equal;
	r1IP6DestIID2.action = NOTSENT;

	/* UDP HEADER */
	struct SCHC_Field r1UDPsrcPort;
	r1UDPsrcPort.fieldLength = 16;
	r1UDPsrcPort.targetValue = 1086;
	r1UDPsrcPort.matchingOperator = &equal;
	r1UDPsrcPort.action = NOTSENT;

	struct SCHC_Field r1UDPdstPort;
	r1UDPdstPort.fieldLength = 16;
	r1UDPdstPort.targetValue = 5683;
	r1UDPdstPort.matchingOperator = &equal;
	r1UDPdstPort.action = NOTSENT;

	struct SCHC_Field r1UDPlength;
	r1UDPlength.fieldLength = 16;
	r1UDPlength.targetValue = 0;
	r1UDPlength.matchingOperator = &ignore;
	r1UDPlength.action = COMPUTELENGTH;

	struct SCHC_Field r1UDPchecksum;
	r1UDPchecksum.fieldLength = 16;
	r1UDPchecksum.targetValue = 0;
	r1UDPchecksum.matchingOperator = &ignore;
	r1UDPchecksum.action = COMPUTECHECKSUM;

	rule1.fields[0] = r1Version;
	rule1.fields[1] = r1TClass;
	rule1.fields[2] = r1FLabel;
	rule1.fields[3] = r1Length;
	rule1.fields[4] = r1NHeader;
	rule1.fields[5] = r1HLimit;
	rule1.fields[6] = r1IP6SrcPrefix1;
	rule1.fields[7] = r1IP6SrcPrefix2;
	rule1.fields[8] = r1IP6SrcIID1;
	rule1.fields[9] = r1IP6SrcIID2;
	rule1.fields[10] = r1IP6DestPrefix1;
	rule1.fields[11] = r1IP6DestPrefix2;
	rule1.fields[12] = r1IP6DestIID1;
	rule1.fields[13] = r1IP6DestIID2;
	rule1.fields[14] = r1UDPsrcPort;
	rule1.fields[15] = r1UDPdstPort;
	rule1.fields[16] = r1UDPlength;
	rule1.fields[17] = r1UDPchecksum;
	rule1.id=1;

	/* Rule 2 */
	struct SCHC_Field r2Version;
	r2Version.fieldLength = 4;
	r2Version.targetValue = 6;
	r2Version.matchingOperator = &equal;
	r2Version.action = NOTSENT;

	struct SCHC_Field r2TClass;
	r2TClass.fieldLength = 8;
	r2TClass.targetValue = 0;
	r2TClass.matchingOperator = &ignore;
	r2TClass.action = NOTSENT;

	struct SCHC_Field r2FLabel;
	r2FLabel.fieldLength = 20;
	r2FLabel.targetValue = 0;
	r2FLabel.matchingOperator = &ignore;
	r2FLabel.action = NOTSENT;

	struct SCHC_Field r2Length;
	r2Length.fieldLength = 16;
	r2Length.targetValue = 0;
	r2Length.matchingOperator = &ignore;
	r2Length.action = COMPUTELENGTH;

	struct SCHC_Field r2NHeader;
	r2NHeader.fieldLength = 8;
	r2NHeader.targetValue = 17;  //UDP
	r2NHeader.matchingOperator = &equal;
	r2NHeader.action = NOTSENT;

	struct SCHC_Field r2HLimit;
	r2HLimit.fieldLength = 8;
	r2HLimit.targetValue = 255;
	r2HLimit.matchingOperator = &ignore;
	r2HLimit.action = NOTSENT;

	struct SCHC_Field r2IP6SrcPrefix1;
	r2IP6SrcPrefix1.fieldLength = 32;
	r2IP6SrcPrefix1.targetValue = 0xA8060120;       //Little-endian
	r2IP6SrcPrefix1.matchingOperator = &equal;
	r2IP6SrcPrefix1.action = NOTSENT;

	struct SCHC_Field r2IP6SrcPrefix2;
	r2IP6SrcPrefix2.fieldLength = 32;
	r2IP6SrcPrefix2.targetValue = 0x2120801D;    //Little-endian
	r2IP6SrcPrefix2.matchingOperator = &equal;
	r2IP6SrcPrefix2.action = NOTSENT;

	struct SCHC_Field r2IP6SrcIID1;
	r2IP6SrcIID1.fieldLength = 32;
	r2IP6SrcIID1.targetValue = 0xFF483002;
	r2IP6SrcIID1.matchingOperator = &equal;
	r2IP6SrcIID1.action = NOTSENT;

	struct SCHC_Field r2IP6SrcIID2;
	r2IP6SrcIID2.fieldLength = 32;
	r2IP6SrcIID2.targetValue = 0xE43E5AFE;
	r2IP6SrcIID2.matchingOperator = &equal;
	r2IP6SrcIID2.action = NOTSENT;

    struct SCHC_Field r2IP6DestPrefix1;
    r2IP6DestPrefix1.fieldLength = 32;
    r2IP6DestPrefix1.targetValue = 0xA8060120;              //Little-endian
    r2IP6DestPrefix1.matchingOperator = &equal;
    r2IP6DestPrefix1.action = NOTSENT;

    struct SCHC_Field r2IP6DestPrefix2;
    r2IP6DestPrefix2.fieldLength = 32;
    r2IP6DestPrefix2.targetValue = 0x0206801D;      //Little-endian
    r2IP6DestPrefix2.matchingOperator = &equal;
    r2IP6DestPrefix2.action = NOTSENT;

    struct SCHC_Field r2IP6DestIID1;
    r2IP6DestIID1.fieldLength = 0;
    r2IP6DestIID1.targetValue = 0;                  //Little-endian
    r2IP6DestIID1.matchingOperator = &ignore;
    r2IP6DestIID1.action = BUILDIID;

    struct SCHC_Field r2IP6DestIID2;
    r2IP6DestIID2.fieldLength = 0;
    r2IP6DestIID2.targetValue = 0;                  //Little-endian
    r2IP6DestIID2.matchingOperator = &ignore;
    r2IP6DestIID2.action = BUILDIID;

	/* UDP HEADER */
	struct SCHC_Field r2UDPsrcPort;
	r2UDPsrcPort.fieldLength = 16;
	r2UDPsrcPort.targetValue = 5683;
	r2UDPsrcPort.matchingOperator = &equal;
	r2UDPsrcPort.action = NOTSENT;

	struct SCHC_Field r2UDPdstPort;
	r2UDPdstPort.fieldLength = 16;
	r2UDPdstPort.targetValue = 1086;
	r2UDPdstPort.matchingOperator = &equal;
	r2UDPdstPort.action = NOTSENT;

	struct SCHC_Field r2UDPlength;
	r2UDPlength.fieldLength = 16;
	r2UDPlength.targetValue = 0;
	r2UDPlength.matchingOperator = &ignore;
	r2UDPlength.action = COMPUTELENGTH;

	struct SCHC_Field r2UDPchecksum;
	r2UDPchecksum.fieldLength = 16;
	r2UDPchecksum.targetValue = 0;
	r2UDPchecksum.matchingOperator = &ignore;
	r2UDPchecksum.action = COMPUTECHECKSUM;


    rule2.fields[0] = r2Version;
    rule2.fields[1] = r2TClass;
    rule2.fields[2] = r2FLabel;
    rule2.fields[3] = r2Length;
    rule2.fields[4] = r2NHeader;
    rule2.fields[5] = r2HLimit;
    rule2.fields[6] = r2IP6SrcPrefix1;
    rule2.fields[7] = r2IP6SrcPrefix2;
    rule2.fields[8] = r2IP6SrcIID1;
    rule2.fields[9] = r2IP6SrcIID2;
    rule2.fields[10] = r2IP6DestPrefix1;
    rule2.fields[11] = r2IP6DestPrefix2;
    rule2.fields[12] = r2IP6DestIID1;
    rule2.fields[13] = r2IP6DestIID2;
    rule2.fields[14] = r2UDPsrcPort;
    rule2.fields[15] = r2UDPdstPort;
    rule2.fields[16] = r2UDPlength;
    rule2.fields[17] = r2UDPchecksum;
    rule2.id=2;

	rules[0] = rule1;
	rules[1] = rule2;
	rules[2] = rule3;
	rules[3] = rule4;
}
