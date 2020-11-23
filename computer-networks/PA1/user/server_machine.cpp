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

#include "server_machine.h"

#include "interface.h"
#include "frame.h"

#include <netinet/in.h>
#include <netinet/ip.h> // for iphdr struct
using namespace std;

ServerMachine::ServerMachine (SimulatedMachine *simulatedMachine, Interface* iface) :
	SimpleMachine(simulatedMachine, iface) {
	// The machine instantiated.
	// Interfaces are not valid at this point.
}

ServerMachine::~ServerMachine () {
	// destructor...
}

void ServerMachine::initialize () {
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
void ServerMachine::processFrame (Frame frame, int ifaceIndex) {
	auto frame_packet = (ethernet_frame *) frame.data;

	switch (frame_packet->data.data_type){
		case DHCP_DISCOVER:
			r_dhcp_discover(frame, ifaceIndex);
			break;
		case DHCP_REQUEST:
			r_dhcp_request(frame, ifaceIndex);
			break;
		case DHCP_ACK:
			r_dhcp_ack(frame);
			break;
		case DHCP_RELEASE:
			r_dhcp_release(frame);
			break;
		case DHCP_REQUEST_EXTEND:
			r_dhcp_extend_request(frame, ifaceIndex);
			break;
		default:
			std::cout << "invalid packet, dropped" << std::endl;
	}
}


/**
 * This method will be run from an independent thread. Use it if needed or simply return.
 * Returning from this method will not finish the execution of the program.
 */
void ServerMachine::run () {
	while (true){
		auto *input = new input_part;
		parse_admin_input(input);

		switch (input->c_type){
			case ADD_POOL:
				add_pool(input->IP, input->mask);
				break;
			case ADD_TIME:
				add_time(input->time);
				break;
			case PRINT_POOL:
				print_pool();
				break;
			default:
				cout << "invalid command" << std::endl;
		}
	}
}
void ServerMachine::parse_admin_input(input_part *input_info)
{
	string command;
	getline(cin, command);

	vector<string> command_info = split(command, ' ');

	regex add_pool("add pool .*");
	regex add_time("add time .*");
	regex print_pool("print pool");

	if (regex_match(command, add_pool)){
		input_info->c_type = ADD_POOL;
		vector<string> temp = split(command_info[2], '/');
		make_up_uint32(temp[0], input_info->IP);
		input_info->mask = (uint8_t)stoi(temp[1]);
	} else if (regex_match(command, add_time)){
		input_info->c_type = ADD_TIME;
		input_info->time = stoi(command_info[2]);
	} else if (regex_match(command, print_pool)){
		input_info->c_type = PRINT_POOL;
	}
}
void ServerMachine::print_pool()
{
	sort(ip_pool.begin(), ip_pool.end(),
	     [](const byte *a, const byte *b){
		     return (a[0] < b[0])?true:(a[1] < b[1]? true:(a[2] < b[2]? true:a[3] < b[3]));
	});

	for (auto &i : ip_pool) {
		ip_ntop(i);
		cout << endl;
	}
}
void ServerMachine::add_pool(byte *ip_ptr, uint8_t mask)
{
	auto shift = (uint8_t)32 - mask;
	ip_ptr[3] = (ip_ptr[3] >> shift) << shift;

	for (int i = 0; i < 1 << shift; i++) {
		auto new_ip = new byte[4];
		memcpy(new_ip, ip_ptr, 4);
		ip_ptr[3]++;
		if (find_ip_pool(new_ip) == -1) {
			ip_pool.push_back(new_ip);
		}
	}
}
void ServerMachine::add_time(int time)
{
	for (auto &given_IP : given_IPs) {
		given_IP->time -= time;
	}
	sort(given_IPs.begin(), given_IPs.end(), []
	(const ethernet_data *a, const ethernet_data *b){
		return a->time < b->time;
	});
	while (!given_IPs.empty()){
		if (given_IPs[0]->time <= 0){
			t_dhcp_timeout(given_IPs[0]);
			ip_pool.push_back(given_IPs[0]->IP);
			given_IPs.erase(given_IPs.begin());
		}
		else
			return;
	}
}
void ServerMachine::r_dhcp_discover(Frame frame, int iface_number)
{
	if (ip_pool.begin() != ip_pool.end()){
		auto frame_data = (ethernet_frame *) frame.data;
		frame_data->data.time = ntohl(frame_data->data.time);

		auto data = new byte[SIZE_OF_FRAME];
		auto ethz = (ethernet_frame *) data;

		memset(ethz->header.dst, 255, 6);
		memcpy(ethz->header.src, iface[iface_number].mac, 6);
		ethz->header.type = htons (0x0);

		ethz->data.data_type = DHCP_OFFER;
		memcpy(ethz->data.MAC, frame_data->data.MAC, 6);
		sort(ip_pool.begin(), ip_pool.end(),
			[](const byte *a, const byte *b){
				return (a[0] < b[0])?true:(a[1] < b[1]? true:(a[2] < b[2]? true:a[3] < b[3]));
		});
		memcpy(ethz->data.IP, ip_pool[0], 4);
		ethz->data.time = (frame_data->data.time == 0) ? 10 :frame_data->data.time;

		auto instance = new ethernet_data;
		instance->time = ethz->data.time;
		memcpy(instance->IP, ethz->data.IP, 4);
		memcpy(instance->MAC, ethz->data.MAC, 6);
		offered_IPs.push_back(instance);
		ip_pool.erase(ip_pool.begin());

		cout << "offer ";
		ip_ntop(ethz->data.IP);
		cout << " to ";
		mac_ntop(ethz->data.MAC);
		cout << " for time " << ethz->data.time << endl;
		ethz->data.time = htonl(ethz->data.time);
		Frame o_frame ((uint32) SIZE_OF_FRAME, data);
		sendFrame(o_frame, iface_number);

		delete[] data;
	}
}
void ServerMachine::r_dhcp_request(Frame frame, int iface_number)
{
	auto frame_data = (ethernet_frame *) frame.data;
	frame_data->data.time = ntohl(frame_data->data.time);
	auto itr = find_if(offered_IPs.begin(), offered_IPs.end(), [frame_data](const ethernet_data* a){
		return (memcmp(a->IP, frame_data->data.IP, 4) == 0);
	});
	if (itr != offered_IPs.end()){
		auto data = new byte[SIZE_OF_FRAME];
		auto ethz = (ethernet_frame *) data;

		memset(ethz->header.dst, 255, 6);
		memcpy(ethz->header.src, iface[iface_number].mac, 6);
		ethz->header.type = htons (0x0);

		ethz->data.data_type = DHCP_ACK;
		memcpy(ethz->data.MAC, frame_data->data.MAC, 6);
		memcpy(ethz->data.IP, frame_data->data.IP, 4);
		ethz->data.time = (frame_data->data.time == 0) ? 10 : frame_data->data.time;

		cout << "assign ";
		ip_ntop(ethz->data.IP);
		cout << " to ";
		mac_ntop(ethz->data.MAC);
		cout << " for " << ethz->data.time << endl;

		auto inst = new ethernet_data;
		inst->time = ethz->data.time;
		memcpy(inst->IP, ethz->data.IP, 4);
		memcpy(inst->MAC, ethz->data.MAC, 6);
		given_IPs.push_back(inst);
		offered_IPs.erase(itr);

		ethz->data.time = htonl(ethz->data.time);
		Frame ack ((uint32) SIZE_OF_FRAME, data);
		sendFrame(ack, iface_number);
		delete[] data;
	}
}
void ServerMachine::r_dhcp_ack(Frame frame)
{
	auto frame_data = (ethernet_frame *) frame.data;
	frame_data->data.time = ntohl(frame_data->data.time);
	auto itr = find_if(offered_IPs.begin(), offered_IPs.end(),
		[frame_data](const ethernet_data* a){
			return memcmp(a->MAC, frame_data->data.MAC, 6) == 0;
	});
	if (itr != offered_IPs.end()){
		ip_ntop((*itr)->IP);
		cout << " back to pool" << endl;
		ip_pool.push_back((*itr)->IP);
		offered_IPs.erase(itr);
	}
}
void ServerMachine::r_dhcp_release(Frame frame)
{
	auto frame_data = (ethernet_frame *) frame.data;
	frame_data->data.time = ntohl(frame_data->data.time);
	auto itr = find_if(given_IPs.begin(), given_IPs.end(),[frame_data]
	(const ethernet_data* a){
		return memcmp(a->IP, frame_data->data.IP, 4) == 0;
	});
	if (itr != given_IPs.end()){
		ip_pool.push_back(frame_data->data.IP);
		given_IPs.erase(itr);
	}
}
void ServerMachine::r_dhcp_extend_request(Frame frame, int iface_number)
{
	auto frame_data = (ethernet_frame *) frame.data;
	frame_data->data.time = ntohl(frame_data->data.time);
	auto itr = find_if(given_IPs.begin(), given_IPs.end(),[frame_data]
		(const ethernet_data* a){
		return memcmp(a->IP, frame_data->data.IP, 4) == 0;
	});
	if (itr != given_IPs.end()){
		uint32 another_time = (frame_data->data.time == 0)?
			(*itr)->time + 10:
			(*itr)->time + frame_data->data.time;
		ip_pool.push_back(frame_data->data.IP);
		given_IPs.erase(itr);

		auto data = new byte[SIZE_OF_FRAME];
		auto ethz = (ethernet_frame *) data;

		memset(ethz->header.dst, 255, 6);
		memcpy(ethz->header.src, iface[iface_number].mac, 6);
		ethz->header.type = htons (0x0);

		ethz->data.data_type = DHCP_RESPONSE_EXTEND;
		memcpy(ethz->data.MAC, frame_data->data.MAC, 6);
		memcpy(ethz->data.IP, frame_data->data.IP, 4);
		sort(ip_pool.begin(), ip_pool.end(),
		     [](const byte *a, const byte *b){
			     return (a[0] < b[0])?true:(a[1] < b[1]? true:(a[2] < b[2]? true:a[3] < b[3]));
		});
		memcpy(ethz->data.IP, ip_pool[0], 4);
		ethz->data.time = another_time;

		ip_pool.erase(ip_pool.begin());
		auto inst = new ethernet_data;
		inst->time = ethz->data.time;
		memcpy(inst->IP, ethz->data.IP, 4);
		memcpy(inst->MAC, ethz->data.MAC, 6);
		given_IPs.push_back(inst);
		cout << "assign ";
		ip_ntop(ethz->data.IP);
		cout << " to ";
		mac_ntop(ethz->data.MAC);
		cout << " for " << ethz->data.time << endl;

		ethz->data.time = htonl(ethz->data.time);
		Frame shot ((uint32) SIZE_OF_FRAME, data);
		sendFrame(shot, iface_number);

		delete[] data;
	}
}
void ServerMachine::t_dhcp_timeout(ethernet_data *announce)
{
	int count = getCountOfInterfaces();
	for (int i = 0; i < count; i++){
		auto data = new byte[SIZE_OF_FRAME - sizeof(uint32)];
		auto ethz = (ethernet_frame *) data;

		memset(ethz->header.dst, 255, 6);
		memcpy(ethz->header.src, iface[i].mac, 6);
		ethz->header.type = htons (0x0);

		ethz->data.data_type = DHCP_TIMEOUT;
		memcpy(ethz->data.MAC, announce->MAC, 6);
		memcpy(ethz->data.IP, announce->IP, 4);

		Frame timeout ((uint32)(SIZE_OF_FRAME - sizeof(uint32)), data);
		sendFrame(timeout, i);
	}
}
int ServerMachine::find_ip_pool(byte *target_ip)
{
	for (int i = 0; i < ip_pool.size(); ++i) {
		if (memcmp(ip_pool[i], target_ip, 4) == 0){
			return i;
		}
	}
	return -1;
}

