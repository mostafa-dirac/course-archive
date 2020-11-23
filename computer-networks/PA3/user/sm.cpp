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

#include "sm.h"

#include "interface.h"
#include "frame.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <map>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <stdio.h>


using namespace std;


SimulatedMachine::SimulatedMachine (const ClientFramework *cf, int count) :
  Machine (cf, count) {
  // The machine instantiated.
  // Interfaces are not valid at this point.
}

SimulatedMachine::~SimulatedMachine () {
  // destructor...
}

void SimulatedMachine::initialize () {
	int countOfInterface = getCountOfInterfaces();
	iface_state *temp_ifsm;
	for (int i = 0; i < countOfInterface ; i++){
		temp_ifsm = new iface_state;
		ifsm_array.push_back(temp_ifsm);
		ifsm_array[i]->current_state = IDLE;
		ifsm_array[i]->ifaceIndex = i;
		ifsm_array[i]->syn_sent = false;
		ifsm_array[i]->synAck_sent = false;
	}
	string got_custom_information = getCustomInformation();
	vector<string> line_of_information = split(got_custom_information, '\n');

	regex AS("AS[[:digit:]]+");
	regex if_customer("Interface [[:digit:]]+ : your customer, .*");
	regex if_peer("Interface [[:digit:]]+ : peer, .*");
	regex if_provider("Interface [[:digit:]]+ : your provider, .*");
	regex asset("We are Owner of: .*");
	for (auto &str : line_of_information){
		if (regex_match(str, AS)){
			as_number = stoi(str.substr(2));
		}
		else if (regex_match(str, asset)){
			vector<string> words = split(str, ' ');
			vector<string> prefix_info = split(words[4], '/');
			auto *new_add_pre = new address_prefix;
			str_to_ip(prefix_info[0], &new_add_pre->ip);
			new_add_pre->mask = (uint8_t)stoi(prefix_info[1]);
			assets.push_back(new_add_pre);
		}
		else if (regex_match(str, if_customer) || regex_match(str, if_peer) || regex_match(str, if_provider)) {
			vector<string> words = split(str, ' ');
			int ifaceIndex = stoi(words[1]);
			ifsm_array[ifaceIndex]->ifaceIndex = ifaceIndex;

			if (regex_match(str, if_customer)){
				ifsm_array[ifaceIndex]->neighborhood = CUSTOMER;
				str_to_ip(words[6], &ifsm_array[ifaceIndex]->neighbor_ip);
			}
			else if (regex_match(str, if_peer)){
				ifsm_array[ifaceIndex]->neighborhood = PEER;
				str_to_ip(words[5], &ifsm_array[ifaceIndex]->neighbor_ip);
			}
			else if (regex_match(str, if_provider)){
				ifsm_array[ifaceIndex]->neighborhood = PROVIDER;
				str_to_ip(words[6], &ifsm_array[ifaceIndex]->neighbor_ip);
			}
		}
		else
			continue;

	}
	for (auto &itemAsset : assets) {
		auto new_path = new hop_by_hop_path;
		new_path->ases.push_back(this->as_number);
		new_path->priority = 0;
		new_path->start_interface = 0;
		new_path->advertiser = MYASSET;
		new_path->destination.mask = itemAsset->mask;
		new_path->destination.ip = itemAsset->ip;
		paths.push_back(new_path);
	}
	new thread(&SimulatedMachine::manage_time, this);
}

/**
 * This method is called from the main thread.
 * Also ownership of the data of the frame is not with you.
 * If you need it, make a copy for yourself.
 *
 * You can also send frames using:
 * <code>
 *     bool synchronized sendFrame (Frame frame, int ifaceIndex) const;
 * </code>
 * which accepts one frame and the interface index (counting from 0) and
 * sends the frame on that interface.
 * The Frame class used here, encapsulates any kind of network frame.
 * <code>
 *     class Frame {
 *     public:
 *       uint32 length;
 *       byte *data;
 *
 *       Frame (uint32 _length, byte *_data);
 *       virtual ~Frame ();
 *     };
 * </code>
 */

void SimulatedMachine::manage_time(){
	while(!this->ifsm_array.empty()) {   //TODO ::
		usleep(1000000);
		for (auto & itr : ifsm_array){
			if (itr->current_state == ESTABLISHED || itr->current_state == O_CONFIRM || itr->current_state == O_SENT){
				if (itr->HT > 0){
					itr->HT--;
				}
				if (itr->HT == 0){
					itr->CRT = MAX_CRT;
					itr->RTO = MAX_RTO;
					itr->HT= MAX_HT;
					report_change(itr->current_state, IDLE, itr->ifaceIndex);//TODO
					report_change(IDLE, CONNECT, itr->ifaceIndex);
					itr->current_state = CONNECT;
					handle_tcp_handshake(SYN, itr->ifaceIndex);
					itr->syn_sent = true;
				}
			}
			if (itr->current_state == CONNECT){
				if (itr->RTO > 0){
					itr->RTO--;
				}
				if (itr->RTO == 0){
					itr->CRT = MAX_CRT;
					itr->current_state = ACTIVE_STATE;
					report_change(CONNECT, ACTIVE_STATE, itr->ifaceIndex);
					continue;
				}
				if (itr->CRT > 0){
					itr->CRT--;
				}
				if (itr->CRT == 0){
					itr->CRT = MAX_CRT;
					itr->RTO = MAX_RTO;
					report_change(CONNECT, IDLE, itr->ifaceIndex);
					report_change(IDLE, CONNECT, itr->ifaceIndex);
					handle_tcp_handshake(SYN, itr->ifaceIndex);
					itr->syn_sent = true;
				}
			}
			if (itr->current_state == ACTIVE_STATE){
				if (itr->CRT > 0){
					itr->CRT--;
				}
				if (itr->CRT == 0){
					itr->CRT = MAX_CRT;
					itr->RTO = MAX_RTO;
					itr->current_state = CONNECT;
					report_change(ACTIVE_STATE, CONNECT, itr->ifaceIndex); //&itr - ifsm_array.data()
					handle_tcp_handshake(SYN, itr->ifaceIndex);
					itr->syn_sent = true;
				}
			}
		}
	}
}
void SimulatedMachine::processFrame (Frame frame, int ifaceIndex) {
	auto ethz = (normal_header *) frame.data;

	if (ifsm_array[ifaceIndex]->current_state == IDLE)  //TODO: Simply Return?
		return;

	message_type dtop = detect_type_of_packet(ethz);
	switch (dtop) {
		case SYN:
			if (ifsm_array[ifaceIndex]->current_state == CONNECT || ifsm_array[ifaceIndex]->current_state == ACTIVE_STATE){
				handle_tcp_handshake(SYNACK, ifaceIndex);
				ifsm_array[ifaceIndex]->synAck_sent = true;
				ifsm_array[ifaceIndex]->RTO = MAX_RTO;
			}
			break;
		case ACK:
			if (ifsm_array[ifaceIndex]->current_state == CONNECT || ifsm_array[ifaceIndex]->current_state == ACTIVE_STATE){
				if (ifsm_array[ifaceIndex]->synAck_sent){
					report_change(ifsm_array[ifaceIndex]->current_state, O_SENT, ifaceIndex);
					send_bgp_open(ifaceIndex);
					ifsm_array[ifaceIndex]->RTO = MAX_RTO;
					ifsm_array[ifaceIndex]->CRT = MAX_CRT;
					ifsm_array[ifaceIndex]->HT = MAX_HT;
					ifsm_array[ifaceIndex]->syn_sent = false;
					ifsm_array[ifaceIndex]->synAck_sent = false;
					ifsm_array[ifaceIndex]->current_state = O_SENT;
				}
			}
			break;
		case SYNACK:
			if (ifsm_array[ifaceIndex]->current_state == CONNECT || ifsm_array[ifaceIndex]->current_state == ACTIVE_STATE){
				if (ifsm_array[ifaceIndex]->syn_sent){
					handle_tcp_handshake(ACK, ifaceIndex);
					report_change(ifsm_array[ifaceIndex]->current_state, O_SENT, ifaceIndex);
					send_bgp_open(ifaceIndex);
					ifsm_array[ifaceIndex]->RTO = MAX_RTO;
					ifsm_array[ifaceIndex]->CRT = MAX_CRT;
					ifsm_array[ifaceIndex]->HT = MAX_HT;
					ifsm_array[ifaceIndex]->syn_sent = false;
					ifsm_array[ifaceIndex]->synAck_sent = false;
					ifsm_array[ifaceIndex]->current_state = O_SENT;
				}
			}
			break;
		case KEEPALIVE:
			if (ifsm_array[ifaceIndex]->current_state == O_CONFIRM || ifsm_array[ifaceIndex]->current_state == ESTABLISHED){
				if (ifsm_array[ifaceIndex]->current_state == O_CONFIRM)
					report_change(O_CONFIRM, ESTABLISHED, ifaceIndex);
				ifsm_array[ifaceIndex]->current_state = ESTABLISHED;
				ifsm_array[ifaceIndex]->HT = MAX_HT;
			}
			break;
		case UPDATE:
			if (ifsm_array[ifaceIndex]->current_state == ESTABLISHED){
				handle_bgp_update(frame, ifaceIndex);
				ifsm_array[ifaceIndex]->HT = MAX_HT;
			}
			break;
		case OPEN:
			if (ifsm_array[ifaceIndex]->current_state == O_SENT){
				send_bgp_keepalive(ifaceIndex);
				report_change(ifsm_array[ifaceIndex]->current_state, O_CONFIRM, ifaceIndex);
				ifsm_array[ifaceIndex]->current_state = O_CONFIRM;
				ifsm_array[ifaceIndex]->HT = MAX_HT;
			}
			break;
		default:
			break;
	}
}


/**
 * This method will be run from an independent thread. Use it if needed or simply return.
 * Returning from this method will not finish the execution of the program.
 */
void SimulatedMachine::run () {
	while (!ifsm_array.empty()) {       //TODO:
		auto input_info = new as_input;
		parse_input(input_info);

		switch (input_info->i_type) {
			case START:
				this->start_connection(input_info->ifaceIndex);
				delete input_info; //TODO:?????
				break;
			case PRINT_R:
				this->print_routes(input_info->target_address);
				delete input_info;
				break;
			case HIGHJACK:
				this->highjack(input_info->target_address);
				delete input_info;
				break;
			case PRIORITY:
				this->setting_priority(input_info->ifaceIndex, input_info->priority);
				delete input_info;
				break;
			case WITHDRAW:
				this->withdraw(input_info->target_address);
				delete input_info;
				break;
			case ADVERTISE:
				this->advertise();
				delete input_info;
				break;
			default:
				delete input_info;
				break;
		}
	}
}


/**
 * You could ignore this method if you are not interested on custom arguments.
 */
void SimulatedMachine::parseArguments (int argc, char *argv[]) {
  // TODO: parse arguments which are passed via --args
}

void SimulatedMachine::parse_input(as_input *input_info){
	memset(input_info, 0, sizeof(as_input));

	string command;
	getline(cin, command);
	vector<string> command_info = split(command, ' ');

	regex start_connection("start connection on interface .*");
	regex withdraw ("withdraw .*");
	regex advertise_all ("advertise all");
	regex print_routes ("print routes to .*");
	regex set_priority ("priority of .*");
	regex highjack ("hijack .*");

	if (regex_match(command, start_connection)){
		input_info->i_type = START;
		input_info->ifaceIndex = stoi(command_info[4]);
	}
	else if (regex_match(command, withdraw)){
		input_info->i_type = WITHDRAW;
		vector<string> str_prefix = split(command_info[1],'/');
		str_to_ip(str_prefix[0], &input_info->target_address.ip);
		input_info->target_address.mask = (uint8_t)stoi(str_prefix[1]);
	}
	else if (regex_match(command, advertise_all)){
		input_info->i_type = ADVERTISE;
	}
	else if (regex_match(command, print_routes)){
		input_info->i_type = PRINT_R;
		vector<string> str_prefix = split(command_info[3],'/');
		str_to_ip(str_prefix[0], &input_info->target_address.ip);
		input_info->target_address.mask = (uint8_t)stoi(str_prefix[1]);
	}
	else if (regex_match(command, set_priority)){
		input_info->i_type = PRIORITY;
		input_info->priority = stoi(command_info[4]);
		input_info->ifaceIndex = stoi(command_info[2]);
	}
	else if (regex_match(command, highjack)){
		input_info->i_type = HIGHJACK;
		vector<string> str_prefix = split(command_info[1],'/');
		str_to_ip(str_prefix[0], &input_info->target_address.ip);
		input_info->target_address.mask = (uint8_t)stoi(str_prefix[1]);
	}
}

std::vector<std::string> SimulatedMachine::split(std::string &str, char delimiter)  //TODO: reference.
{
	std::vector<std::string> internal;
	bool isValid = (str.find(delimiter) != string::npos);
	std::stringstream ss(str); // Turn the string into a stream.
	std::string tok;
	if (isValid){
		while(getline(ss, tok, delimiter)) {
			internal.push_back(tok);
		}

		return internal;
	}
	else {
//		cout << "Very bad mistake" << endl;
		internal.emplace_back(string(""));
		return internal;
	}
}
void SimulatedMachine::start_connection(int index)
{
	if (ifsm_array[index]->current_state == IDLE){
		ifsm_array[index]->current_state = CONNECT;
		ifsm_array[index]->RTO = MAX_RTO;
		ifsm_array[index]->CRT = MAX_CRT;
		handle_tcp_handshake(SYN, index);
		ifsm_array[index]->syn_sent = true;
		report_change(IDLE, CONNECT, index);
	}
}

void SimulatedMachine::report_change(bgp_state first, bgp_state second, int index)
{
	cout << "state changed from " << enum2state(first) << " to " << enum2state(second) << " on interface " << index << endl;
}

std::string SimulatedMachine::enum2state(bgp_state enum_state){
	switch (enum_state) {
		case IDLE:
			return string("IDLE");
		case O_SENT:
			return string("OPENSTATE");
		case O_CONFIRM:
			return string("OPENCONFIRM");
		case ACTIVE_STATE:
			return string("ACTIVESTATE");
		case ESTABLISHED:
			return string("ESTABLISHED");
		case CONNECT:
			return string("CONNECT");
		default:
			return string("");
	}
}
void SimulatedMachine::handle_tcp_handshake(message_type type, int index)
{
	int packet_length = SIZE_OF_ETHHDR + SIZE_OF_IPHDR + SIZE_OF_TCPHDR;
	byte* data = new byte[packet_length];
	auto ethz = (normal_header *) data;
	switch (type) {
		case SYN:
			make_tcphdr(ethz, TCP_SYN);
			break;
		case ACK:
			make_tcphdr(ethz, TCP_ACK);
			break;
		case SYNACK:
			make_tcphdr(ethz, TCP_ACK + TCP_SYN);
			break;
		default:
			return;
	}
	make_iphdr(ethz, iface[index].getIp(), ifsm_array[index]->neighbor_ip, SIZE_OF_IPHDR + SIZE_OF_TCPHDR);
	make_ethernethdr(ethz);

	Frame frame ((uint32)packet_length, data);
	sendFrame(frame, index);

	delete[](data);
}

void SimulatedMachine::make_ethernethdr(normal_header *ethz) {
	ethz->ethernetHeader.ether_type = htons(0x0800);
	memset(ethz->ethernetHeader.ether_dhost, 255, 6);
	memset(ethz->ethernetHeader.ether_shost, 255, 6);
}

void SimulatedMachine::make_iphdr(normal_header *ethz, uint32_t src_ip, uint32_t dst_ip, uint16_t ip_length) {
	memset(&(ethz->ipHeader), 0, SIZE_OF_IPHDR);
	ethz->ipHeader.ip_v = 4;
	ethz->ipHeader.ip_hl = 5;
	ethz->ipHeader.ip_len = htons(ip_length);
	ethz->ipHeader.ip_ttl = 0xFF;
	ethz->ipHeader.ip_src.s_addr = htonl(src_ip);
	ethz->ipHeader.ip_dst.s_addr = htonl(dst_ip);
}

void SimulatedMachine::make_tcphdr(normal_header *ethz, byte TCP_FLAGS) {
	memset(&(ethz->tcpHeader), 0, SIZE_OF_TCPHDR);
	ethz->tcpHeader.tcp_doff = 5;
	ethz->tcpHeader.tcp_flags = TCP_FLAGS;
}

void SimulatedMachine::make_bgphdr(false_header *ethz, uint16_t bgp_length, byte type) {
//	auto epfl = (false_header*) ethz;
	memset(ethz->bgpHeader.marker, 0xFF, 16);
	ethz->bgpHeader.length = htons(bgp_length);
	ethz->bgpHeader.type = type;
}

void SimulatedMachine::str_to_ip(std::string &IP, uint32_t *dst){
	vector<string> ip_bytes = split(IP, '.');
//	auto arr_temp = new byte(4);
	uint8_t arr_temp[4];
	for (int i = 0; i < 4; i++) {
		arr_temp[i] = (byte) stoi(ip_bytes[3-i]);
	}
	memcpy(dst, arr_temp, 4);  //TODO: Delete?
//	delete (arr_temp);
}
void SimulatedMachine::print_routes(address_prefix prefix)
{
	vector<hop_by_hop_path*> road_to_reality;
	int ip_byte[4];
	for (auto &item : paths){
		if (item->destination.ip == prefix.ip && item->destination.mask == prefix.mask){
			road_to_reality.push_back(item);    //TODO
		}
	}
	if (road_to_reality.empty()){
		ip_byte[0] = prefix.ip >> (unsigned)24;
		ip_byte[1] = (uint8_t)((prefix.ip << (unsigned)8) >> (unsigned)24);
		ip_byte[2] = (uint8_t)((prefix.ip << (unsigned)16) >> (unsigned)24);
		ip_byte[3] = (uint8_t)((prefix.ip << (unsigned)24) >> (unsigned)24);
		cout << "no routes found for " << ip_byte[0] << "." << ip_byte[1] << "." <<
		ip_byte[2] << "." << ip_byte[3] << "/" << to_string(prefix.mask) << endl;
	}

	sort(road_to_reality.begin(), road_to_reality.end(), SimulatedMachine::comparator_for_path);
	for (auto &item : road_to_reality){
		for (uint16_t number : item->ases)
			cout << number << " " ;
		ip_byte[0] = prefix.ip >> (unsigned)24;
		ip_byte[1] = (uint8_t)((prefix.ip << (unsigned)8) >> (unsigned)24);
		ip_byte[2] = (uint8_t)((prefix.ip << (unsigned)16) >> (unsigned)24);
		ip_byte[3] = (uint8_t)((prefix.ip << (unsigned)24) >> (unsigned)24);
		cout << ip_byte[0] << "." << ip_byte[1] << "." <<
		     ip_byte[2] << "." << ip_byte[3] << "/" << to_string(prefix.mask) << endl;
	}
}
void SimulatedMachine::highjack(address_prefix prefix)
{
	int hdr_len = SIZE_OF_ETHHDR + SIZE_OF_IPHDR + SIZE_OF_BGPHDR;
	int msg_len = 13; //RFC
	byte *msg = new byte[hdr_len + msg_len];
	auto ethz = (normal_header*) msg;

	make_ethernethdr(ethz);
	make_tcphdr(ethz, 0);
	make_bgphdr((false_header*)ethz, (uint16_t)(SIZE_OF_BGPHDR + msg_len), UPDATE);

	struct hijack_struct{
		uint16_t withdraw_number;
		uint16_t TPAL;
		uint16_t path_attr_number;
		uint16_t path_hop;
		uint32_t dest;
		uint8_t dest_mask;
	} __attribute__((packed));
	auto liar = new hijack_struct;
	liar->withdraw_number = 0;
	liar->TPAL = htons(1);
	liar->path_attr_number = htons(1);
	liar->path_hop = htons(as_number);
	liar->dest = htonl(prefix.ip);
	liar->dest_mask = prefix.mask;
	memcpy(&msg[hdr_len], liar, msg_len);   //TODO: LIAR
	delete liar;

	int count_of_if = getCountOfInterfaces();
	for (int i = 0; i < count_of_if; ++i) {
		if (ifsm_array[i]->current_state == ESTABLISHED){
			make_iphdr(ethz, iface[i].getIp(), ifsm_array[i]->neighbor_ip, (uint16_t)(hdr_len + msg_len - SIZE_OF_ETHHDR));

			Frame frame ((uint32_t)(hdr_len + msg_len), msg);
			sendFrame(frame, i);
		}
	}
	delete[](msg);
}
void SimulatedMachine::setting_priority(int ifaceIndex, int priority)
{
	for (auto &item : paths){
		if(item->start_interface == ifaceIndex){
			item->priority = priority;
		}
	}
}
void SimulatedMachine::withdraw(address_prefix prefix)
{
	struct withdraw_struct{
		uint16_t withdraw_number;
		uint32_t withdraw_ip;
		uint8_t withdraw_mask;
		uint16_t path_attr_number;
	} __attribute__((packed));

	int hdr_len = SIZE_OF_ETHHDR + SIZE_OF_IPHDR + SIZE_OF_BGPHDR;
	int msg_len = sizeof(withdraw_struct); //RFC
	byte *msg = new byte[hdr_len + msg_len];
	auto ethz = (false_header*) msg;

	make_ethernethdr((normal_header*) ethz);
//	make_tcphdr(ethz, 0);
	make_bgphdr(ethz, (uint16_t)(SIZE_OF_BGPHDR + msg_len), UPDATE);

	auto loser = new withdraw_struct;
	loser->withdraw_number = htons(5);
	loser->withdraw_ip = htonl(prefix.ip);
	loser->withdraw_mask = prefix.mask;
	loser->path_attr_number = 0;

	memcpy(&msg[hdr_len], loser, msg_len);   //TODO: LIAR
	delete loser;

	empty_trash_path(prefix);

	int count_of_if = getCountOfInterfaces();
	for (int i = 0; i < count_of_if; ++i) {
		if (ifsm_array[i]->current_state == ESTABLISHED){
			make_iphdr((normal_header*) ethz, iface[i].getIp(), ifsm_array[i]->neighbor_ip, (uint16_t)(hdr_len + msg_len - SIZE_OF_ETHHDR));

			Frame frame ((uint32_t)(hdr_len + msg_len), msg);
			sendFrame(frame, i);
		}
	}
	delete[](msg);
}
void SimulatedMachine::advertise()
{
	int header_length = SIZE_OF_ETHHDR + SIZE_OF_IPHDR + SIZE_OF_BGPHDR;
	int count_of_if = getCountOfInterfaces();
	int PAL = 0, NLRI = 0;
	int data_length = 0, packet_length = 0;
	int count_of_paths = 0;
	int offset = 0;
	uint16_t withdraw_len = 0;
	uint16_t next_hop = 0, TPAL = 0, count_of_hops = 0;
	uint32_t dst_ip = 0;
	vector<hop_by_hop_path*> valid_paths;

	for (int if_idx = 0; if_idx < count_of_if; ++if_idx) {
		if (ifsm_array[if_idx]->current_state != ESTABLISHED)
			continue;
		PAL = 0;
		valid_paths = get_admissible_routes(ifsm_array[if_idx]->neighborhood, ifsm_array[if_idx]->ifaceIndex);
		for (auto &each_path : valid_paths) {
			PAL += 2 + each_path->ases.size() * 2;
		}
		NLRI = valid_paths.size() * 5;
		data_length = 2 + 2 + PAL + NLRI;
		packet_length = header_length + data_length;
		byte *data = new byte[packet_length];
		auto *ethz = (false_header *)data;

		make_ethernethdr((normal_header *)ethz);
		make_iphdr((normal_header *)ethz, iface[if_idx].getIp(), ifsm_array[if_idx]->neighbor_ip, SIZE_OF_IPHDR + SIZE_OF_BGPHDR + data_length);
		make_bgphdr(ethz, SIZE_OF_BGPHDR + data_length, UPDATE);

		memcpy(data + header_length, &withdraw_len, 2);

		TPAL = htons((uint16_t)(valid_paths.size()));
		memcpy(data + header_length + 2, &TPAL, 2);

		offset = 0;
		for (auto &item : valid_paths) {
			count_of_hops = htons((uint16_t)(item->ases.size()));
			memcpy(data + header_length + 4 + offset, &count_of_hops, 2);
			count_of_hops = ntohs(count_of_hops);
			for (int j = 0; j < count_of_hops; ++j) {
				next_hop = htons(item->ases[j]);
				memcpy(data + header_length + 4 + offset + (j + 1) * 2, &next_hop, 2);
			}
			offset += item->ases.size() * 2 + 2;
		}

		count_of_paths = valid_paths.size();
		for (int i = 0; i < count_of_paths; ++i) {
			dst_ip = htonl(valid_paths[i]->destination.ip);
			memcpy(data + header_length + 4 + PAL + i * 5, &dst_ip, 4);
			data[header_length + PAL + i * 5 + 8] = valid_paths[i]->destination.mask;
		}

		Frame frame ((uint32) packet_length, data);
		sendFrame(frame, if_idx);
		delete[](data);
	}
}
void SimulatedMachine::handle_bgp_update(Frame frame, int ifaceIndex)
{
	auto data = (byte*) frame.data;     //TODO: Do we need allocation?
	int header_length = SIZE_OF_ETHHDR + SIZE_OF_IPHDR + SIZE_OF_BGPHDR;
	uint16_t WRL;
	memcpy(&WRL, data + header_length, 2);
	WRL = ntohs(WRL);

	if (WRL != 0){
		handle_withdrawn_routes(frame, ifaceIndex, header_length + 2, WRL / 5);
	}

	uint16_t TPAL;
	memcpy(&TPAL, data + header_length + 2 + WRL, 2);
	TPAL = ntohs(TPAL);

	if (TPAL != 0){
		handle_advertisement(frame, ifaceIndex, header_length + 4 + WRL, TPAL);
	}

}

void SimulatedMachine::handle_withdrawn_routes(Frame &frame, int ifaceIndex, int doff, int number)
{
	auto data = (byte*) frame.data;
	auto temp_prefix = new address_prefix;
	for (int i = 0; i < number; ++i) {
		memcpy(&temp_prefix->ip, data + doff + i * 5, 4);
		temp_prefix->ip = ntohl(temp_prefix->ip);
		temp_prefix->mask = data[doff + (i + 1) * 5 -1];
		empty_trash_path(*temp_prefix);
	}
	delete temp_prefix;

	auto ethz = (normal_header *) frame.data;
	uint16_t packet_len = ntohs(ethz->ipHeader.ip_len);
	int count_interface = getCountOfInterfaces();
	for (int j = 0; j < count_interface; ++j) {
		if (j != ifaceIndex){
			make_iphdr(ethz, iface[j].getIp(), ifsm_array[j]->neighbor_ip, packet_len);

			Frame reply_frame ((uint32_t) frame.length, data);
			sendFrame(reply_frame, j);
		}
	}
}

void SimulatedMachine::handle_advertisement(Frame &frame, int ifaceIndex, int doff, uint16_t number)
{
	auto data = (byte *) frame.data;
	vector<hop_by_hop_path*> advertised_paths;
	auto intern_path = new hop_by_hop_path[number];
	int offset_holder = 0;
	int path_len;
	int ip_byte[4];
	uint16_t next_hop;
	uint32_t temp_ip;

	for (int i = 0; i < number; ++i) {
		intern_path[i].start_interface = ifaceIndex;
		intern_path[i].advertiser = ifsm_array[ifaceIndex]->neighborhood;
		intern_path[i].priority = get_if_priority(intern_path[i].advertiser);

		memcpy(&path_len, data + doff + offset_holder, 2);
		path_len = ntohs(path_len);
		for (int j = 0; j < path_len; ++j) {
			memcpy(&next_hop, data + doff + offset_holder + (j + 1) * 2, 2);
			intern_path[i].ases.push_back(ntohs(next_hop));  //TODO: push_back?
		}
		advertised_paths.push_back(&intern_path[i]);
		offset_holder += 2 + path_len * 2;
	}

	for (int i = 0; i < number; ++i) {
		memcpy(&temp_ip, data + doff + offset_holder + i * 5, 4);
		advertised_paths[i]->destination.ip = ntohl(temp_ip);
		advertised_paths[i]->destination.mask = data[doff + offset_holder + (i + 1) * 5 - 1];
	}

	for (int i = 0; i < number; ++i) {
		if (detect_loop(advertised_paths[i])){
			continue;
		}
		if (detect_thief(advertised_paths[i])){
			ip_byte[0] = advertised_paths[i]->destination.ip >> (unsigned)24;
			ip_byte[1] = (uint8_t)((advertised_paths[i]->destination.ip << (unsigned)8) >> (unsigned)24);
			ip_byte[2] = (uint8_t)((advertised_paths[i]->destination.ip << (unsigned)16) >> (unsigned)24);
			ip_byte[3] = (uint8_t)((advertised_paths[i]->destination.ip << (unsigned)24) >> (unsigned)24);
			cout << ip_byte[0] << "." << ip_byte[1] << "." << ip_byte[2] << "." << ip_byte[3] << "/" <<
			     to_string(advertised_paths[i]->destination.mask) << " is hijacked!" << endl;
			continue;
		}
		advertised_paths[i]->ases.insert(advertised_paths[i]->ases.begin(), as_number);
		if (is_in_archive(advertised_paths[i])){
			continue;
		}
		paths.push_back(advertised_paths[i]);
	}


}

void SimulatedMachine::send_bgp_open(int ifaceIndex)
{
	struct packet{
		false_header whole_header;
		bgp_open open_data;
	} __attribute__((packed));
	int msg_length = sizeof(struct bgp_open) + sizeof(struct false_header);
	auto msg = new byte[msg_length];
	auto open_msg = (packet*) msg;

	make_ethernethdr((normal_header*)&open_msg->whole_header);
	make_iphdr((normal_header*)&open_msg->whole_header, iface[ifaceIndex].getIp(), ifsm_array[ifaceIndex]->neighbor_ip, msg_length - SIZE_OF_ETHHDR);
	make_bgphdr(&open_msg->whole_header, SIZE_OF_BGPHDR + sizeof(struct bgp_open), OPEN);

	open_msg->open_data.version = 4;
	open_msg->open_data.my_as = htons(as_number);
	open_msg->open_data.hold_time = 0;
	open_msg->open_data.opt_param_len = 0;
	open_msg->open_data.bgp_id = htonl(iface[ifaceIndex].getIp());

	Frame frame ((uint32) msg_length, msg);
	sendFrame(frame, ifaceIndex);

	delete[](msg);
}
void SimulatedMachine::send_bgp_keepalive(int ifaceIndex)
{
	int msg_length = sizeof(struct false_header);
	auto msg = new byte[msg_length];
	auto keepalive_msg = (normal_header*) msg;

	make_ethernethdr(keepalive_msg);
	make_iphdr(keepalive_msg, iface[ifaceIndex].getIp(), ifsm_array[ifaceIndex]->neighbor_ip,
	           SIZE_OF_IPHDR + SIZE_OF_BGPHDR);
	make_bgphdr((false_header*)keepalive_msg, SIZE_OF_BGPHDR, KEEPALIVE);

	Frame frame ((uint32) msg_length, msg);
	sendFrame(frame, ifaceIndex);

	delete[](msg);
}
message_type SimulatedMachine::detect_type_of_packet(normal_header* ethz)
{
	uint8_t bgp_marker[16] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	                          0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	if (memcmp(((byte*)ethz) + SIZE_OF_ETHHDR + SIZE_OF_IPHDR, bgp_marker, 16) == 0){
		switch (*(((byte*)(&ethz->tcpHeader)) + 18)){
			case 1:
				return OPEN;
			case 2:
				return UPDATE;
			case 4:
				return KEEPALIVE;
			default:
				return INVALID;
		}
	}
	else{
		if (ethz->tcpHeader.tcp_flags == TCP_ACK){
			return ACK;
		}
		else if (ethz->tcpHeader.tcp_flags == TCP_SYN){
			return SYN;
		}
		else if (ethz->tcpHeader.tcp_flags == TCP_ACK + TCP_SYN){
			return SYNACK;
		}
	}

	return INVALID;
}

void SimulatedMachine::printFrame(Frame &frame) {
	std::cerr << "frame with length " << frame.length << std::endl;
	for (uint32 i = 0; i < frame.length; ++i) {
		std::cout << std::hex << std::setw(2) << std::setfill('0') << (unsigned int)(*(frame.data + i));
	}
	std::cout << std::dec;
	std::cout << std::endl;
}

bool SimulatedMachine::comparator_for_path(hop_by_hop_path* first, hop_by_hop_path* second){
	if (first->priority > second->priority)
		return true;
	else if (first->priority < second->priority)
		return false;

	if (first->ases.size() < second->ases.size())
		return true;
	else if (first->ases.size() > second->ases.size())
		return false;

	if (first->start_interface < second->start_interface)
		return true;
	else if (first->start_interface > second->start_interface)
		return false;

	int size = first->ases.size();
	for (int i = 0; i < size; ++i) {
		if (first->ases[i] < second->ases[i])
			return true;
		else if (first->ases[i] < second->ases[i])
			return false;
	}
	return false;
}
void SimulatedMachine::empty_trash_path(address_prefix prefix)
{
	for (auto item = paths.begin() ; item != paths.end() ; item++){
		if ((*item)->destination.ip == prefix.ip && (*item)->destination.mask){
			paths.erase(item);      //TODO: Dangerous.
			item--;
		}
	}
}

bool SimulatedMachine::detect_loop(hop_by_hop_path* path){
	for (auto &item : path->ases){
		if (item == as_number)
			return true;
	}
	return false;
}

bool SimulatedMachine::detect_thief(hop_by_hop_path* path){
	for (auto & item : paths){
		if (item->destination.ip == path->destination.ip && item->destination.mask == path->destination.mask) {
			if (item->ases.back() != path->ases.back()) {
				return true;
			}
		}

	}
	return false;
}

bool SimulatedMachine::is_in_archive(hop_by_hop_path *intern_path) {

	int i = 0, intern_size = intern_path->ases.size();
	for (auto & path : paths) {
		if (intern_path->destination.ip != path->destination.ip || intern_path->destination.mask != path->destination.mask)
			continue;
		if (intern_path->ases.size() != path->ases.size())
			continue;

		for (int j = 0; j < intern_size; ++j) {
			if (intern_path->ases[j] == path->ases[j])
				i++;
		}
		if (i == intern_size)
			return true;
		i = 0;
	}
	return false;
}

int SimulatedMachine::get_if_priority(interaction_type advertiser) {
	switch (advertiser){
		case CUSTOMER:
			return 100;
		case PEER:
			return 90;
		case PROVIDER:
			return 80;
		default:
			return -1;
	}
}

vector<hop_by_hop_path*> SimulatedMachine::get_admissible_routes(interaction_type rshp, int ifaceIndex) {
	vector<hop_by_hop_path*> admissible;
	for (auto &path : paths) {
		if (path->start_interface == ifaceIndex){
			admissible.push_back(path);
			continue;
		}
		switch (rshp){
			case PEER:{
				if (path->advertiser != PEER){
					admissible.push_back(path);
				}
				break;
			}
			case CUSTOMER:{
				admissible.push_back(path);
				break;
			}
			case PROVIDER:{
				if (path->advertiser != PROVIDER){
					admissible.push_back(path);
				}
				break;
			}
			default:
				cout << "very bad error in selecting admissible paths." << endl;
				break;
		}
	}
	return admissible;
}