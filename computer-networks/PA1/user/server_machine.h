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

class ServerMachine: public SimpleMachine {
public:

	std::vector<byte*> ip_pool;
	std::vector<ethernet_data*> offered_IPs;
	std::vector<ethernet_data*> given_IPs;

	ServerMachine (SimulatedMachine*, Interface* iface);
	virtual ~ServerMachine ();

	virtual void initialize ();
	virtual void run ();
	virtual void processFrame (Frame frame, int ifaceIndex);

	void parse_admin_input (input_part *input);
	void print_pool ();
	void add_pool (byte *ip_ptr, uint8_t mask);
	void add_time (int time);
	void r_dhcp_discover (Frame frame, int iface_number);
	void r_dhcp_request (Frame frame, int iface_number);
	void r_dhcp_ack (Frame frame);
	void r_dhcp_release (Frame frame);
	void r_dhcp_extend_request (Frame frame, int iface_number);
	void t_dhcp_timeout (ethernet_data *announce);
	int find_ip_pool (byte *target_ip);

};

#endif

