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

#ifndef _SRV_M_H_
#define _SRV_M_H_

#include "simpleMachine.h"
#include "sm.h"

struct client_info{
	metadata addresses;
	byte ID:5;
};

class ServerMachine: public SimpleMachine {
public:
	std::vector<client_info*> information;
	byte current_free_id:5;

	ServerMachine (SimulatedMachine*, Interface* iface);
	virtual ~ServerMachine ();

	virtual void initialize ();
	virtual void run ();
	virtual void processFrame (Frame frame, int ifaceIndex);
	static data_type detect_type(header *packet_header);
	void rec_request_id(Frame frame, int ifaceIndex);
	void rec_ip_request(Frame frame, int ifaceIndex);
	void rec_update_request(Frame frame);
	void receive_status(Frame frame, int ifaceIndex);
	int find_client_by_public(uint32 public_ip, uint16_t public_port);
	int find_client_by_ID(byte ID);
	int find_client_by_local(uint32 local_ip);
	int find_gateway(uint32 dst_ip_hdr);
};

#endif

