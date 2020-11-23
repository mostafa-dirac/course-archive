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

#ifndef _CLI_M_H_
#define _CLI_M_H_

#include "simpleMachine.h"
#include "sm.h"

enum input_type_client{
	MAKE_CONNECTION_CLIENT = 1,
	GET_ID_INFO_CLIENT,
	MAKE_LOCAL_SESSION_CLIENT,
	MAKE_PUBLIC_SESSION_CLIENT,
	SEND_MSG_CLIENT,
	STATUS_CLIENT
};

struct client_input {
	input_type_client i_type;
	uint16 ID: 5;
	uint16 source_port;
	char *msg;
	int msg_length;
};

const char ping[4] = {'p','i','n','g'};
const char pong[4] = {'p','o','n','g'};

class ClientMachine: public SimpleMachine {
public:
	uint16_t my_port;
	byte my_ID :5;
	metadata *peer_info;
	byte peer_ID: 5;
	bool connected;
	data_type peer_session_kind;        //TODO: CHANGE.

	ClientMachine (SimulatedMachine*, Interface* iface);
	virtual ~ClientMachine ();

	virtual void initialize ();
	virtual void run ();
	virtual void processFrame (Frame frame, int ifaceIndex);
	static void parse_input(client_input *input_info);
	static data_type detect_data_type(header *packet_header, char *dm);
	void make_connection(uint16_t PORT);
	void receive_drop_packet(Frame frame);
	void make_session(byte ID, data_type session_kind);
	void get_id_info(byte ID);
	void response_id(Frame frame);
	void response_ip(Frame frame);
	void request_session(Frame frame, int ifaceIndex, data_type session_kind);
	bool check_connection(byte ID);
	void response_session(Frame frame);
	void send_message(byte ID, char *msg, int msg_length);
	void receive_message(Frame frame);
	void receive_NAT_updated_packet();
	void ask_status();
	static void rec_status_response(Frame frame);
	int find_gateway(uint32 dst_ip_hdr);
};

#endif

