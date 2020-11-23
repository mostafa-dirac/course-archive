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

#ifndef _NAT_M_H_
#define _NAT_M_H_

#include "simpleMachine.h"
#include "sm.h"

enum input_type_nat{
	BLOCK_RANGE = 1,
	RESET
};

struct nat_input {
	input_type_nat i_type;
	uint16_t min;
	uint16_t max;
} __attribute__((packed));

struct Session{
	uint32_t local_ip;
	uint16_t local_port;
	uint32_t outer_ip;
	uint16_t outer_port;
}__attribute__ ((packed));

struct Range{
	uint16_t begin;
	uint16_t end;
}__attribute__ ((packed));

struct address{
	uint32_t ip;
	uint16_t port;
}__attribute__ ((packed));

class NatMachine: public SimpleMachine {
public:

	std::vector<Range*> blocked_range;
	std::vector<Session*> sessions;
	std::vector<metadata*> table;

	uint16_t base_port;
	unsigned int counter;

	NatMachine (SimulatedMachine*, Interface* iface);
	virtual ~NatMachine ();

	virtual void initialize ();
	virtual void run ();
	virtual void processFrame (Frame frame, int ifaceIndex);
	address calculate_new_address();
	bool valid_in_range(uint16_t port);
	int find_in_local_table(uint32_t local_ip, uint16_t local_port);
	int find_in_session(uint32_t local_ip, uint16_t local_port, uint32_t outer_ip, uint16_t outer_port);
	static void parse_input(nat_input *input_info);
	void block_range(uint16_t min, uint16_t max);
	void reset_setting();
	int find_in_outer_table(uint32_t public_ip, uint16_t public_port);
	int find_gateway(uint32 dst_ip_hdr);
};

#endif

