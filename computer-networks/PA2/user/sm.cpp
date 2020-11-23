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
#include "simpleMachine.h"
#include "client_machine.h"
#include "nat_machine.h"
#include "server_machine.h"

#include <netinet/in.h>
#include <netinet/ip.h> // for iphdr struct
using namespace std;


SimpleMachine *simpleMachine;
SimulatedMachine::SimulatedMachine (const ClientFramework *cf, int count) :
	Machine (cf, count) {
	// The machine instantiated.
	// Interfaces are not valid at this point.
}

SimulatedMachine::~SimulatedMachine () {
	// destructor...
}

void SimulatedMachine::initialize () {
	// TODO: Initialize your program here; interfaces are valid now.
	string rule = getCustomInformation();
	if(rule == "Client")
			simpleMachine = (SimpleMachine*) new ClientMachine(this, iface);
	else if(rule == "NAT")
		simpleMachine = (SimpleMachine*) new NatMachine(this, iface);
	else if(rule == "Server")
		simpleMachine = (SimpleMachine*) new ServerMachine(this, iface);
	simpleMachine->initialize();
}


void SimulatedMachine::processFrame (Frame frame, int ifaceIndex) {
	simpleMachine->processFrame(frame, ifaceIndex);
}


/**
 * This method will be run from an independent thread. Use it if needed or simply return.
 * Returning from this method will not finish the execution of the program.
 */
void SimulatedMachine::run () {
	simpleMachine->run();
}


/**
 * You could ignore this method if you are not interested on custom arguments.
 */
void SimulatedMachine::parseArguments (int argc, char *argv[]) {
	// TODO: parse arguments which are passed via --args
}

