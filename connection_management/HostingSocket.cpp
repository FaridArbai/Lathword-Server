/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HostingSocket.cpp
 * Author: Faraday
 * 
 * Created on December 13, 2017, 2:37 AM
 */

#include "HostingSocket.h"
#include "TimingSocket.h"

HostingSocket::HostingSocket() {

}

HostingSocket::HostingSocket(string l_ip, int l_port, int max_queued_allowed){
	HostingConnection hosting_connection = newHostingConnection(l_ip,l_port,max_queued_allowed);
	this->setHostingConnection(hosting_connection);
	this->setSocket(hosting_connection.hosting_socket);
}

HostingSocket::HostingSocket(const HostingSocket& orig) {
	this->setHostingConnection(orig.getHostingConnection());
}

HostingSocket::~HostingSocket() {
}

int HostingSocket::acceptConnections(){
	int client_socket;
	
	client_socket = client_socket = accept( 
			  hosting_connection.hosting_socket, 
			  (struct sockaddr*) &(hosting_connection.server_storage), 
			  &(hosting_connection.l_server_storage) );;
	
	return client_socket;
}

HostingSocket::HostingConnection HostingSocket::newHostingConnection(string l_ip, int l_port, int max_queued_allowed){
	HostingConnection hosting_connection;
	int hosting_socket;
	int client_socket;
	
	struct sockaddr_in server_addr;
	
	struct sockaddr_storage server_storage;
	
	socklen_t l_server_addr;
	socklen_t l_server_storage;
	
	
	bool is_listening;
	
	//1. Socket creation
		hosting_socket = ::socket(PF_INET, SOCK_STREAM, 0);
	
	//2. Socket configuration
		server_addr.sin_family = AF_INET;
		
		server_addr.sin_port = htons(l_port);
		
		server_addr.sin_addr.s_addr = inet_addr(l_ip.c_str());

		memset(server_addr.__pad, '\0', sizeof server_addr.__pad);
	//3. Socket binding to local IP address
		l_server_addr = sizeof(server_addr);
		
		bind(hosting_socket, (struct sockaddr*) &server_addr, l_server_addr);
		
	//4. Trigger listening-mode on socket
		is_listening = ((listen(hosting_socket,max_queued_allowed))==0);
		
		if (is_listening){
			printf("Listening successfull \n");
		}
		else{
			printf("Listening error \n");
		}
		
		l_server_storage = sizeof(server_storage);
	// 5. assign values
		
		hosting_connection.hosting_socket = hosting_socket;
		hosting_connection.l_server_storage = l_server_storage;
		hosting_connection.server_storage = server_storage;
		
	return hosting_connection; 
}

HostingSocket::HostingConnection HostingSocket::getHostingConnection() const{
	return this->hosting_connection;
}

void HostingSocket::setHostingConnection(HostingSocket::HostingConnection hosting_connection){
	this->hosting_connection = hosting_connection;
}
