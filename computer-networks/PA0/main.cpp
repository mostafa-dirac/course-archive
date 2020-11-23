#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

#define MAXSIZE 100

int main(int argc, char **argv) {
	auto my_listen_port = (uint16_t)std::atoi(argv[1]);
	uint16_t dest_port = 0;
	bool in_connection = false;
	if (my_listen_port < 10000){
		std::cerr << "Port in not valid. Use port numbers greater than 10000 and smaller than 65535." << std::endl;
		return 0;
	}
	struct sockaddr_in my_listen_address, my_speak_address, peer_address;
	size_t peer_address_size = sizeof(peer_address);
	int peer_socket;

	int my_listen_socket = socket(PF_INET, SOCK_STREAM, 0);
	if (my_listen_socket == -1) {
		std::cerr << "Failed to create a new socket" << std::endl;
		return 0;
	}
	memset(&my_listen_address, 0, sizeof(my_listen_address));
	my_listen_address.sin_family = AF_INET;
	my_listen_address.sin_addr.s_addr = htons(INADDR_ANY);
	my_listen_address.sin_port = htons(my_listen_port);

	memset(&my_speak_address, 0, sizeof(my_speak_address));
	my_speak_address.sin_family = AF_INET;
	my_speak_address.sin_addr.s_addr = htons(INADDR_ANY);
	my_speak_address.sin_port = htons(my_listen_port + (uint16_t)1);

	if (bind(my_listen_socket, (struct sockaddr *) &my_listen_address,
	         sizeof(my_listen_address)) == -1) {
		std::cerr << "Failed to bind on socket." << std::endl;
		return 0;
	}

	if (listen(my_listen_socket, 1) == -1) {
		std::cerr << "Failed to listen on socket." << std::endl;
		return 0;
	}

	std::string cmd, argument;
	char recv_mess[MAXSIZE] = {0};

	while(true) {
		std::cin >> cmd;
		if (cmd == "connect") {
			std::cin >> argument;
			dest_port = (uint16_t) std::stoi(argument);
		}
		else if (cmd == "send" || cmd == "recv"){
			std::getline(std::cin, argument);
			if(argument.begin() != argument.end())
				argument.erase(argument.begin());
		}

		if (cmd == "exit"){
			if (in_connection){
				in_connection = false;
				int temp1 = shutdown(peer_socket, SHUT_RDWR);
				int temp2 = close(peer_socket);
			}
			else{
				std::cout << "invalid command" << std::endl;
			}
		}
		else if (cmd == "connect"){
			if (in_connection){
				std::cout << "invalid command" << std::endl;
			}
			else{
				in_connection = true;
				memset(&peer_address, 0, sizeof(peer_address));
				peer_address.sin_family = AF_INET;
				peer_address.sin_addr.s_addr = htons(INADDR_ANY);
				peer_address.sin_port = htons(dest_port);
				memset(&my_speak_address, 0, sizeof(my_speak_address));
				my_speak_address.sin_family = AF_INET;
				my_speak_address.sin_addr.s_addr = htons(INADDR_ANY);
				my_speak_address.sin_port = htons(my_listen_port + (uint16_t)1);
				peer_socket = socket(PF_INET, SOCK_STREAM, 0);
				if (peer_socket == -1) {
					std::cerr << "Failed to create a new socket" << std::endl;
					return 0;
				}
				//TODO: need to bind?
				if (bind(peer_socket, (struct sockaddr *) &my_speak_address,
				         sizeof(my_speak_address)) == -1) {
					std::cerr << "Failed to bind on socket." << std::endl;
					return 0;
				}
				if ((connect(peer_socket, (sockaddr *) &peer_address, (socklen_t) peer_address_size)) == 0){
					std::cout << "connected to " << ntohs(peer_address.sin_port) << std::endl;
				}
				else{
					std::cerr << "Failed to connect." << std::endl;
					return 0;
				}
			}
		}
		else if (cmd == "listen"){
			if (in_connection){
				std::cout << "invalid command" << std::endl;
			}
			else {
				peer_socket = accept(my_listen_socket, (sockaddr *) &peer_address, (socklen_t *) &peer_address_size);
				in_connection = true;
				if (peer_socket < 0){
					std::cerr << "Failed to accept." << std::endl;
				}
				else{
					std::cout << "connected to " << ntohs(peer_address.sin_port) << std::endl;
				}
			}
		}
		else if (cmd == "send"){
			if(in_connection){
				send(peer_socket, argument.c_str(), strlen(argument.c_str()), 0);
			}
			else{
				std::cout << "invalid command" << std::endl;
			}
		}
		else if (cmd == "recv"){
			if (in_connection){
				recv(peer_socket, recv_mess, (size_t) MAXSIZE, 0);
				std::cout << "recv " << recv_mess << std::endl;
			}
			else{
				std::cout << "invalid command" << std::endl;
			}
		}
	}
}