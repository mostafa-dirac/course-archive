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

void SimpleMachine::make_up_uint32(std::string IP, byte *temp){
	std::vector<std::string> ip_bytes = split(IP, '.');
	auto arr_temp = new byte(4);
	for (int i = 0; i < 4; i++) {
		arr_temp[i] = (byte) stoi(ip_bytes[i]);
	}
	memcpy(temp, arr_temp, 4);  //TODO: Delete?
}

void SimpleMachine::ip_ntop(byte IP[4]){
	std::string output1 = std::to_string(IP[0]) + std::string(".") + std::to_string(IP[1]);
	std::string output2 = std::to_string(IP[2]) + std::string(".") + std::to_string(IP[3]);
	std::string output = output1 + std::string(".") + output2;
	std::cout << output;
}
void SimpleMachine::mac_ntop(byte *MAC)
{
	for (int i = 0; i < 6; i++) {
		print_HEX_byte(MAC[i]);
	}
}
void SimpleMachine::print_HEX_byte(byte b)
{
	int mask = 0x0F;
	auto first = (char)TO_HEX((b >> 4) & mask);
	auto second = (char)TO_HEX(b & mask);
	std::cout << first << second;
}
int SimpleMachine::find_IP(std::vector<ethernet_data *> array, ethernet_frame *elem)
{
	for (int i = 0; i < array.size(); ++i) {
		if (memcmp(array[i]->IP, elem->data.IP, 4) == 0){
			return i;
		}
	}
	return -1;
}
