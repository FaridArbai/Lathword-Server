/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TimingSocket.cpp
 * Author: Faraday
 * 
 * Created on December 13, 2017, 2:22 AM
 */

#include "TimingSocket.h"

TimingSocket::TimingSocket() {
}

TimingSocket::TimingSocket(int socket) {
	this->setSocket(socket);
}

TimingSocket::TimingSocket(const TimingSocket& orig) {
}

TimingSocket::~TimingSocket() {
}

void TimingSocket::setSocket(int socket){
	this->socket = socket;
}

int TimingSocket::getSocket() const{
	int socket = this->socket;
	return socket;
}

TimingSocket::WaitResult TimingSocket::listenFor(int timeout_s){
	WaitResult result;
	fd_set read_fd;
	int read_socket = (this->getSocket());
	int next_fd = read_socket + 1;
	struct timeval tv;
	int waiting_value;
	
	tv.tv_sec = timeout_s;
	tv.tv_usec = 0;
	
	FD_ZERO(&read_fd);
	FD_SET(read_socket,&read_fd);
	
	waiting_value = select(next_fd,&read_fd,nullptr,nullptr,&tv); 
	
	switch(waiting_value){
		case 0:{
			result = TimingSocket::WaitResult::timed_out;
			break;
		}
		case 1:{
			result = TimingSocket::WaitResult::received_data;
			break;
		}
	}
	
	return result;
}

void TimingSocket::close(){
	int socket = this->getSocket();
	
	shutdown(socket, SHUT_RDWR);
}