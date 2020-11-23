//                   In the name of GOD
/**
 * Partov is a simulation engine, supporting emulation as well,
 * making it possible to create virtual networks.
 *  
 * Copyright © 2009-2015 Behnam Momeni.
 * 
 * This file is part of the Partov.
 * 
 * Partov is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Partov is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Partov.  If not, see <http://www.gnu.org/licenses/>.
 *  
 */


#include <iomanip>
#include "simpleMachine.h"

SimpleMachine::SimpleMachine (const SimulatedMachine *simulatedMachine, Interface* iface){
		this->simulatedMachine = simulatedMachine;
		this->iface = iface;
}


SimpleMachine::~SimpleMachine(){
}

int SimpleMachine::getCountOfInterfaces () const {
	return simulatedMachine->getCountOfInterfaces();
}

void SimpleMachine::printInterfacesInformation () const {
	simulatedMachine->printInterfacesInformation();
}

const std::string SimpleMachine::getCustomInformation () {
	return simulatedMachine->getCustomInformation();
}

bool SimpleMachine::sendFrame (Frame frame, int ifaceIndex){
	return simulatedMachine->sendFrame(frame, ifaceIndex);
}

std::vector<std::string> SimpleMachine::split(std::string str, char delimiter)
{
	std::vector<std::string> internal;
	std::stringstream ss(str); // Turn the string into a stream.
	std::string tok;

	while(getline(ss, tok, delimiter)) {
		internal.push_back(tok);
	}

	return internal;
}

uint16_t SimpleMachine::get_checksum(const void *buf, size_t buf_len){
	char* data=(char*)buf;
	uint32_t acc=0xffff;

	for (size_t i=0;i+1<buf_len;i+=2) {
		uint16_t word;
		memcpy(&word,data+i,2);
		acc+=ntohs(word);
		if (acc>0xffff) {
			acc-=0xffff;
		}
	}

	if (buf_len&1) {
		uint16_t word=0;
		memcpy(&word,data+buf_len-1,1);
		acc+=ntohs(word);
		if (acc>0xffff) {
			acc-=0xffff;
		}
	}

	return htons(~acc);
}

uint8_t SimpleMachine::get_data_type(data_type type)
{
	switch (type){
		case REQUEST_ASSIGNING_ID:
			return 0;
		case RESPONSE_ASSIGNING_ID:
			return 0;
		case DROP:
			return 0;
		case REQUEST_GETTING_IP:
			return 1;
		case RESPONSE_GETTING_IP:
			return 1;
		case REQUEST_LOCAL_SESSION:
			return 2;
		case RESPONSE_LOCAL_SESSION:
			return 2;
		case REQUEST_PUBLIC_SESSION:
			return 2;
		case RESPONSE_PUBLIC_SESSION:
			return 2;
		case MESSAGE:
			return 3;
		case NAT_UPDATED:
			return 4;
		case REQUEST_UPDATING_INFO:
			return 5;
		case STATUS:
			return 6;
		case STATUS_RESPONSE:
			return 7;
		default:
			return -1;  //TODO: 255?
	}
}
uint16_t SimpleMachine::get_data_length(data_type type)
{
	switch (type){
		case REQUEST_ASSIGNING_ID:
			return 6;
		case RESPONSE_ASSIGNING_ID:
			return 0;
		case DROP:
			return 4;
		case REQUEST_GETTING_IP:
			return 0;
		case RESPONSE_GETTING_IP:
			return 12;
		case REQUEST_LOCAL_SESSION:
			return 4;
		case RESPONSE_LOCAL_SESSION:
			return 4;
		case REQUEST_PUBLIC_SESSION:
			return 4;
		case RESPONSE_PUBLIC_SESSION:
			return 4;
		case NAT_UPDATED:
			return 0;
		case REQUEST_UPDATING_INFO:
			return 6;
		case STATUS:
			return 6;
		case STATUS_RESPONSE:
			return 6;
		default:
			return -1; //TODO: 255?
	}
}

void SimpleMachine::fill_header(header *packet_header, byte *mac, data_type type, int data_length, uint32 src_ip, uint32 dst_ip,
	uint16_t src_port, uint16_t dst_port, uint8_t ID){

	packet_header->ethernetHeader.type = htons (0x0800);
	memset(packet_header->ethernetHeader.dst, 255, 6);
	memcpy(packet_header->ethernetHeader.src, mac, 6);

	packet_header->ipHeader.version_IHL = 0x45;
	packet_header->ipHeader.DSCP_ECN = 0;
	packet_header->ipHeader.total_length = htons(sizeof(ip_header) + sizeof(udp_header) + sizeof(data_id) + data_length);
	packet_header->ipHeader.identification = 0;
	packet_header->ipHeader.flags_fragmentation_offset = 0;
	packet_header->ipHeader.TTL = 64;
	packet_header->ipHeader.protocol = 17;
	packet_header->ipHeader.header_checksum = 0x0;
	packet_header->ipHeader.src_ip = htonl(src_ip);
	packet_header->ipHeader.dst_ip = htonl(dst_ip);
	packet_header->ipHeader.header_checksum = (get_checksum(&(packet_header->ipHeader), 20));

	packet_header->udpHeader.src_port = htons(src_port);
	packet_header->udpHeader.dst_port = htons(dst_port);
	packet_header->udpHeader.length = htons(sizeof(udp_header) + sizeof(data_id) + data_length);
	packet_header->udpHeader.checksum = 0x00;


	packet_header->dataId.data_type = get_data_type(type);
	packet_header->dataId.id = ID;
}
