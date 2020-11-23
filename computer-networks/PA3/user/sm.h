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

#ifndef _S_M_H_
#define _S_M_H_

#include "machine.h"
#include "sr_protocol.h"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <sstream>
#include <regex>
#include <zconf.h>
#include <thread>
#include <iomanip>


#define MAX_RTO 30
#define MAX_CRT 31
#define MAX_HT 240
#define SIZE_OF_ETHHDR 14
#define SIZE_OF_IPHDR 20
#define SIZE_OF_TCPHDR 20
#define SIZE_OF_BGPHDR 19



using namespace std;

enum bgp_state {
	IDLE = 1,
	CONNECT,
	O_SENT,
	O_CONFIRM,
	ESTABLISHED,
	ACTIVE_STATE
};

enum message_type{
	OPEN = 1,
	UPDATE = 2,
	KEEPALIVE = 4,
	SYN,
	SYNACK,
	ACK,
	INVALID
};

struct bgp_header{
	byte marker[16];
	uint16_t length;
	byte type;
} __attribute__ ((packed));

struct normal_header{
	sr_ethernet_hdr ethernetHeader;
	ip ipHeader;
	sr_tcp tcpHeader;
} __attribute__ ((packed));

struct false_header{
	sr_ethernet_hdr ethernetHeader;
	ip ipHeader;
	bgp_header bgpHeader;
} __attribute__ ((packed));

struct address_prefix{
	uint32_t ip;
	uint8_t mask;
} /*__attribute__ ((packed))*/;

enum input_type_as{
	START = 1,
	ADVERTISE,
	WITHDRAW,
	HIGHJACK,
	PRINT_R,
	PRIORITY
};

enum interaction_type {
	PEER,
	PROVIDER,
	CUSTOMER,
	MYASSET
};

struct as_input {
	address_prefix target_address;
	input_type_as i_type;
	int priority;
	int ifaceIndex;
} /*__attribute__ ((packed))*/;

struct iface_state {
	bgp_state current_state;
	uint8_t RTO, HT, CRT;
	bool syn_sent, synAck_sent;
	int ifaceIndex;
	uint32_t neighbor_ip;
	interaction_type neighborhood;
} __attribute__ ((packed));

struct hop_by_hop_path{
	int priority;
	vector<uint16_t> ases;
	int start_interface;
	address_prefix destination;
	interaction_type advertiser;
} /*__attribute__ ((packed))*/;

struct as_prefix{
	uint16_t AS;
	address_prefix prefix;
};

struct bgp_open{
	byte version;
	uint16_t my_as;
	uint16_t hold_time;
	uint32_t bgp_id;
	byte opt_param_len;
} __attribute__ ((packed)) ;

class SimulatedMachine : public Machine {
public:
	vector<iface_state*> ifsm_array;
	uint16_t as_number;
	vector<address_prefix*> assets;
	vector<hop_by_hop_path*> paths;
	vector<as_prefix*> as_prefix_table;

	SimulatedMachine (const ClientFramework *cf, int count);
	virtual ~SimulatedMachine ();

	virtual void initialize ();
	virtual void run ();
	virtual void processFrame (Frame frame, int ifaceIndex);

	static void parseArguments (int argc, char *argv[]);
	static void make_ethernethdr(normal_header *ethz);
	static void make_iphdr(normal_header *ethz, uint32_t src_ip, uint32_t dst_ip, uint16_t ip_length);
	static void make_tcphdr(normal_header *ethz, byte TCP_FLAGS);
	static void make_bgphdr(false_header *ethz, uint16_t bgp_length, byte type);
	static void str_to_ip(string &IP, uint32_t *dst);
	static void report_change(bgp_state first, bgp_state second, int index);
	static void parse_input(as_input *input_info);
	static bool comparator_for_path(hop_by_hop_path *first, hop_by_hop_path *second);
	static int get_if_priority(interaction_type advertiser);
	static vector<string> split(string &str, char delimiter);
	static message_type detect_type_of_packet(normal_header *ethz);
	static string enum2state(bgp_state enum_state);

	void start_connection(int index);
	void handle_tcp_handshake(message_type type, int index);
	void manage_time();
	void print_routes(address_prefix prefix);
	void highjack(address_prefix prefix);
	void setting_priority(int ifaceIndex, int priority);
	void withdraw(address_prefix prefix);
	void advertise();
	void handle_bgp_update(Frame frame, int ifaceIndex);
	void send_bgp_open(int ifaceIndex);
	void send_bgp_keepalive(int ifaceIndex);
	void printFrame(Frame &frame);
	void empty_trash_path(address_prefix prefix);
	void handle_withdrawn_routes(Frame &frame, int ifaceIndex, int doff, int number);
	void handle_advertisement(Frame &frame, int ifaceIndex, int doff, uint16_t number);
	bool detect_loop(hop_by_hop_path *path);
	bool detect_thief(hop_by_hop_path *path);
	bool is_in_archive(hop_by_hop_path *intern_path);
	vector<hop_by_hop_path *> get_admissible_routes(interaction_type rshp, int ifaceIndex);
};

#endif /* sm.h */

