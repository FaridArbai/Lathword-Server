/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   HostingSocket.h
 * Author: Faraday
 *
 * Created on December 13, 2017, 2:37 AM
 */

#ifndef HOSTINGSOCKET_H
#define HOSTINGSOCKET_H

#include "TimingSocket.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>

using namespace std;

class HostingSocket : public TimingSocket{
public:
    struct HostingConnection {
    	int hosting_socket;
    	struct sockaddr_storage server_storage;
    	socklen_t l_server_storage;
    };
    
    HostingSocket();
    HostingSocket(const HostingSocket& orig);
    HostingSocket(string l_ip, int l_port, int max_queued_allowed);
    virtual ~HostingSocket();
    
    HostingConnection getHostingConnection() const;
    void setHostingConnection(HostingConnection hosting_connection);
    
    int acceptConnections();
    
private:
    HostingConnection hosting_connection;
    HostingConnection newHostingConnection(string l_ip, int l_port, int max_conns_queued);
};

#endif /* HOSTINGSOCKET_H */

