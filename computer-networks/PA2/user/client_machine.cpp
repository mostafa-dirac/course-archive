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

#include "client_machine.h"

#include "interface.h"
#include "frame.h"

#include <netinet/in.h>
#include <netinet/ip.h> // for iphdr struct

using namespace std;

ClientMachine::ClientMachine (SimulatedMachine *simulatedMachine, Interface* iface) :
	SimpleMachine(simulatedMachine, iface) {
	// The machine instantiated.
	// Interfaces are not valid at this point.
}

ClientMachine::~ClientMachine () {
	// destructor...
}

void ClientMachine::initialize () {
	my_ID = 0;
	connected = false;
	my_port = 0;
	peer_info = nullptr;
	peer_session_kind = INVALID;
	peer_ID = 0;
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
void ClientMachine::processFrame (Frame frame, int ifaceIndex) {

	auto ethz = (packet_pl *) frame.data;

	if (ethz->hdr.ipHeader.TTL > 0){
		if ((ethz->hdr.ipHeader.protocol == 17) && (get_checksum(&(ethz->hdr.ipHeader), 20) == 0)){
			if (ethz->hdr.ipHeader.dst_ip == htonl(iface[0].getIp())){
				switch (detect_data_type(&(ethz->hdr), ethz->pl.message)){
					case RESPONSE_ASSIGNING_ID:
						response_id(frame);
						break;
					case DROP:
						receive_drop_packet(frame);
						break;
					case RESPONSE_GETTING_IP:
						response_ip(frame);
						break;
					case REQUEST_LOCAL_SESSION:
						request_session(frame, ifaceIndex, REQUEST_LOCAL_SESSION);
						break;
					case RESPONSE_LOCAL_SESSION:
						response_session(frame);
						break;
					case REQUEST_PUBLIC_SESSION:
						request_session(frame, ifaceIndex, REQUEST_PUBLIC_SESSION);
						break;
					case RESPONSE_PUBLIC_SESSION:
						response_session(frame);
						break;
					case MESSAGE:
						receive_message(frame);
						break;
					case NAT_UPDATED:
						receive_NAT_updated_packet();
						break;
					case STATUS_RESPONSE:
						rec_status_response(frame);
						break;
					default:
						break;
				}
			} else {

				int i = find_gateway(ntohl(ethz->hdr.ipHeader.dst_ip));

				ethz->hdr.ipHeader.TTL--;
				ethz->hdr.ipHeader.header_checksum = 0;
				ethz->hdr.ipHeader.header_checksum = get_checksum(&(ethz->hdr.ipHeader), 20);

				sendFrame(frame, i);
			}
		} else {
			cout << "invalid packet, dropped" << endl;
		}
	}
}


/**
 * This method will be run from an independent thread. Use it if needed or simply return.
 * Returning from this method will not finish the execution of the program.
 */
void ClientMachine::run () {
	while (true) {
		auto input_info = new client_input;
		parse_input(input_info);

		switch (input_info->i_type) {
			case MAKE_CONNECTION_CLIENT:
				make_connection(input_info->source_port);
				break;
			case GET_ID_INFO_CLIENT:
				get_id_info(input_info->ID);
				break;
			case MAKE_LOCAL_SESSION_CLIENT:
				make_session(input_info->ID, REQUEST_LOCAL_SESSION);
				break;
			case MAKE_PUBLIC_SESSION_CLIENT:
				make_session(input_info->ID, REQUEST_PUBLIC_SESSION);
				break;
			case SEND_MSG_CLIENT:
				send_message(input_info->ID, input_info->msg, input_info->msg_length);
				break;
			case STATUS_CLIENT:
				ask_status();
				break;
			default:
				cout << "invalid command" << endl;
		}
	}
}

void ClientMachine::parse_input(client_input *input_info){
	memset(input_info, 0, sizeof(client_input));

	string command;
	getline(cin, command);
	vector<string> command_info = split(command, ' ');

	regex make_a_connection("make a connection to server on port .*");
	regex get_info("get info of .*");
	regex make_a_local_session("make a local session to .*");
	regex make_a_public_session("make a public session to .*");
	regex send_msg("send msg to .*");
	regex status("status");

	if (regex_match(command, make_a_connection)){
		input_info->i_type = MAKE_CONNECTION_CLIENT;
		input_info->source_port = static_cast<uint16>(stoi(command_info[7]));
	} else if (regex_match(command, get_info)){
		input_info->i_type = GET_ID_INFO_CLIENT;
		input_info->ID = static_cast<uint16>(stoi(command_info[3]));
	} else if (regex_match(command, make_a_local_session)){
		input_info->i_type = MAKE_LOCAL_SESSION_CLIENT;
		input_info->ID = static_cast<uint16>(stoi(command_info[5]));
	} else if(regex_match(command, make_a_public_session)){
		input_info->i_type = MAKE_PUBLIC_SESSION_CLIENT;
		input_info->ID = static_cast<uint16>(stoi(command_info[5]));
	} else if (regex_match(command, (regex) send_msg)){
		input_info->i_type = SEND_MSG_CLIENT;
		input_info->msg = new char;
		vector<string> ID_msg = split(command_info[3], ':');
		input_info->ID = static_cast<uint16>(stoi(ID_msg[0]));
		vector<string> name_resan = split(command, ':');
		unsigned long len = name_resan[1].size();
		name_resan[1].copy(input_info->msg, len);
		input_info->msg_length = static_cast<int>(len);
	} else if (regex_match(command, status)){
		input_info->i_type = STATUS_CLIENT;
	}
}

data_type ClientMachine::detect_data_type(header *packet_header, char *dm) {
	switch (packet_header->dataId.data_type){
		case 0: {
			if (packet_header->dataId.id == 0)
				return DROP;
			return RESPONSE_ASSIGNING_ID;
		}
		case 1:
			return RESPONSE_GETTING_IP;
		case 2: {
			if (memcmp(dm, ping, 4) == 0){
				return REQUEST_LOCAL_SESSION;
			} else if (memcmp(dm, pong, 4) == 0){
				return RESPONSE_LOCAL_SESSION;
			}
			return INVALID;
		}
		case 3:
			return MESSAGE;
		case 4:
			return NAT_UPDATED;
		case 7:
			return STATUS_RESPONSE;
		default:
			return INVALID;
	}
}

void ClientMachine::make_connection(uint16_t PORT) {
	if(my_ID > 0){
		cout << "you already have an id, ignored" << endl;
		return;
	}
	int data_length = get_data_length(REQUEST_ASSIGNING_ID);
	int packet_length = SIZE_OF_HEADER + data_length;
	byte *data = new byte[packet_length];
	auto ethz = (packet_md *)data;
	uint32_t dest_ip = 0x01010101;

	fill_header(&(ethz->hdr), iface[0].mac, REQUEST_ASSIGNING_ID, data_length,
		iface[0].getIp(), dest_ip, PORT, 1234, 0);
	ethz->md.local_ip = htonl(iface[0].getIp());
	ethz->md.local_port = htons(PORT);

	my_port = PORT;

	Frame frame ((uint32) packet_length, data);
	sendFrame (frame, 0);
	delete[] data;
}

void ClientMachine::receive_drop_packet(Frame frame) {
	auto ethz = (header *) frame.data;
	if (ntohs(ethz->udpHeader.src_port) == 1234){
		my_port += 100;
		make_connection(my_port);
		cout << "connection to server failed, retrying on port " << my_port << endl;
	} else if (ntohs(ethz->udpHeader.src_port) == 4321){
		if (connected && peer_info != nullptr){
			cout << "connection lost, perhaps" << peer_ID << "'s info has changed, ask server for updates" << endl;
		}
	}
}

void ClientMachine::response_id(Frame frame) {
	auto ethz = (header *) frame.data;
	my_ID = ethz->dataId.id;
	char *buf = new char[100];
	sprintf(buf, "Now My ID is %d", my_ID);
	auto output = string(buf);
	cout << output << endl;
	delete[] buf;
}

void ClientMachine::get_id_info(byte ID) {
	int data_length = get_data_length(REQUEST_GETTING_IP);
	int packet_length = SIZE_OF_HEADER + data_length;
	byte *data = new byte[packet_length];
	auto ethz = (header *)data;
	uint32_t dest_ip = 0x01010101;

	fill_header(ethz, iface[0].mac, REQUEST_GETTING_IP, data_length,
		iface[0].getIp(), dest_ip, my_port, 1234, ID);

	Frame frame ((uint32) packet_length, data);
	sendFrame (frame, 0);
	delete[] data;
}

void ClientMachine::response_ip(Frame frame) {
	auto ethz = (packet_md *)frame.data;
	if (peer_info == nullptr){
		peer_info = new metadata;
	}
	peer_info->local_ip = ntohl(ethz->md.local_ip);
	peer_info->local_port = ntohs(ethz->md.local_port);
	peer_info->public_ip = ntohl(ethz->md.public_ip);
	peer_info->public_port = ntohs(ethz->md.public_port);
	peer_ID = ethz->hdr.dataId.id;
	connected = false;

	char *buf = new char[200];
	byte *local_ip = new byte[4];
	memcpy(local_ip, &ethz->md.local_ip, 4);
	byte *public_ip = new byte[4];
	memcpy(public_ip, &ethz->md.public_ip, 4);
	sprintf(buf, "packet with (%d, %d.%d.%d.%d, %d, %d.%d.%d.%d, %d) received", ethz->hdr.dataId.id,
	        local_ip[0], local_ip[1], local_ip[2], local_ip[3], ntohs(ethz->md.local_port),
	        public_ip[0], public_ip[1], public_ip[2], public_ip[3], ntohs(ethz->md.public_port));
	auto output = string(buf);
	cout << output << endl;
	delete[] buf;
}

void ClientMachine::make_session(byte ID, data_type session_kind) {
	if (peer_info == nullptr || peer_ID != ID){
		char *buf = new char[50];
		sprintf(buf, "info of node %d was not received", ID);
		auto output = string(buf);
		cout << output << endl;
		delete[] buf;
		return;
	}

	peer_session_kind = session_kind;
	int data_length = get_data_length(session_kind);
	int packet_length = SIZE_OF_HEADER + data_length;
	byte *data = new byte[packet_length];
	auto ethz = (packet_pl *)data;

	int which_interface = (session_kind == REQUEST_LOCAL_SESSION) ? find_gateway(peer_info->local_ip) : find_gateway(
		peer_info->public_ip);
	uint32_t destination_ip = (session_kind == REQUEST_LOCAL_SESSION) ? peer_info->local_ip : peer_info->public_ip;
	uint16_t destination_port = (session_kind == REQUEST_LOCAL_SESSION) ? peer_info->local_port : peer_info->public_port;
	fill_header(&(ethz->hdr), iface[which_interface].mac, session_kind, data_length,
		iface[0].getIp(), destination_ip, my_port, destination_port, my_ID);

	memcpy(ethz->pl.message, ping, 4);

	Frame frame ((uint32) packet_length, data);
	sendFrame (frame, which_interface);
	delete[] data;
}

void ClientMachine::request_session(Frame frame, int ifaceIndex, data_type session_kind) {
	int data_length = get_data_length(session_kind);
	int packet_length = SIZE_OF_HEADER + data_length;
	auto ethz = (packet_pl *)frame.data;

	if (memcmp(ping, ethz->pl.message, 4) == 0){
		if (!check_connection(ethz->hdr.dataId.id)){
			peer_ID = ethz->hdr.dataId.id;
			connected = true;
			peer_session_kind = session_kind;

			peer_info = new metadata;
			peer_info->local_ip = ntohl(ethz->hdr.ipHeader.src_ip);
			peer_info->public_ip = ntohl(ethz->hdr.ipHeader.src_ip);
			peer_info->public_port = ntohs(ethz->hdr.udpHeader.src_port);
			peer_info->local_port = ntohs(ethz->hdr.udpHeader.src_port);

			auto buf = new char[50];
			sprintf(buf, "Connected to %d", peer_ID);
			auto output = string(buf);
			cout << output << endl;
			delete [] buf;

			byte *data = new byte[packet_length];
			auto epfl = (packet_pl *)data;
			data_type reply_session = (session_kind == REQUEST_LOCAL_SESSION) ?
			                          RESPONSE_LOCAL_SESSION : RESPONSE_PUBLIC_SESSION;
			fill_header(&(epfl->hdr), iface[ifaceIndex].mac, reply_session, get_data_length(reply_session),
			            ntohl(ethz->hdr.ipHeader.dst_ip), ntohl(ethz->hdr.ipHeader.src_ip),
			            ntohs(ethz->hdr.udpHeader.dst_port), ntohs(ethz->hdr.udpHeader.src_port), my_ID);

			memcpy(epfl->pl.message, pong, 4);

			Frame new_frame ((uint32) packet_length, data);
			sendFrame (new_frame, ifaceIndex);
			delete[] data;
		}
	}
}

bool ClientMachine::check_connection(byte ID) {
	return peer_ID == ID && connected && peer_info != nullptr;
}

void ClientMachine::response_session(Frame frame)
{
	auto ethz = (packet_pl *)frame.data;
	if (memcmp(pong, ethz->pl.message, 4) == 0){
		if (ethz->hdr.dataId.id == peer_ID){
			if (!connected){
				connected = true;
				char *buf = new char[50];
				sprintf(buf, "Connected to %d", peer_ID);
				auto output = string(buf);
				cout << output << endl;
				delete[] buf;
				return;
			}
		}
	}
}

void ClientMachine::send_message(byte ID, char *msg, int msg_length) {
	if (check_connection(ID)){
		int data_length = msg_length;
		int packet_length = SIZE_OF_HEADER + data_length;
		byte *data = new byte[packet_length];
		auto ethz = (packet_pl *)data;

		int which_interface = (peer_session_kind == REQUEST_LOCAL_SESSION ) ? find_gateway(peer_info->local_ip) : find_gateway(
			peer_info->public_ip);
		uint32_t destination_ip = (peer_session_kind == REQUEST_LOCAL_SESSION) ? peer_info->local_ip : peer_info->public_ip;
		uint16_t destination_port = (peer_session_kind == REQUEST_LOCAL_SESSION) ? peer_info->local_port : peer_info->public_port;
		fill_header(&(ethz->hdr), iface[which_interface].mac, MESSAGE, data_length,
		            iface[0].getIp(), destination_ip, my_port, destination_port, my_ID);
		memcpy(ethz->pl.message, msg, (size_t)(msg_length));

		Frame frame ((uint32) packet_length, data);
		sendFrame (frame, which_interface);
		delete[] data;
	} else {
		cout << "please make a session first" << endl;
	}
}

void ClientMachine::receive_message(Frame frame)
{
	auto ethz = (packet_pl *)frame.data;

	if (check_connection(ethz->hdr.dataId.id)){
		char *buf = new char[50];
		sprintf(buf, "received msg from %d:", ethz->hdr.dataId.id);
		auto output = string(buf);
		size_t len = ntohs(ethz->hdr.udpHeader.length) - sizeof(udp_header) - sizeof(data_id);
		ethz->pl.message[len] = '\0';
		output = output + string(ethz->pl.message);     //TODO: DANGER.
		cout << output << endl;
		delete[] buf;
	}
}


void ClientMachine::receive_NAT_updated_packet() {
	int data_length = get_data_length(REQUEST_UPDATING_INFO);
	int packet_length = SIZE_OF_HEADER + data_length;
	byte *data = new byte[packet_length];
	auto ethz = (packet_md *)data;

	uint32_t dest_ip = 0x01010101;
	fill_header(&(ethz->hdr), iface[0].mac, REQUEST_UPDATING_INFO, data_length,
		iface[0].getIp(), dest_ip, my_port, 1234, my_ID);
	ethz->md.local_port = htons(my_port);
	ethz->md.local_ip = htonl(iface[0].getIp());

	Frame frame ((uint32) packet_length, data);
	sendFrame (frame, 0);
	delete[] data;
}

void ClientMachine::ask_status() {
	int data_length = get_data_length(STATUS);
	int packet_length = SIZE_OF_HEADER + data_length;
	byte *data = new byte[packet_length];
	auto ethz = (packet_md *)data;

	uint32_t dest_ip = 0x01010101;
	fill_header(&(ethz->hdr), iface[0].mac, STATUS, data_length,
	            iface[0].getIp(), dest_ip, my_port, 1234, 0);
	ethz->md.local_ip = htonl(iface[0].getIp());
	ethz->md.local_port = htons(my_port);

	Frame frame ((uint32) packet_length, data);
	sendFrame(frame, 0);
	delete[] data;
}

void ClientMachine::rec_status_response(Frame frame)
{
	auto ethz = (packet_pl *)frame.data;

	if (ethz->hdr.dataId.id == 0){
		cout << "indirect" << endl;
	} else if (ethz->hdr.dataId.id == 1){
		cout << "direct" << endl;
	}
}

int ClientMachine::find_gateway(uint32 dst_ip_hdr) {
	int count = getCountOfInterfaces();
	for (int i = 0; i < count; ++i) {
		uint32 left = this->iface[i].getIp() & this->iface[i].getMask();
		uint32 right = dst_ip_hdr & this->iface[i].getMask();
		if (left == right){
			return i;
		}
	}
	return 0;
}