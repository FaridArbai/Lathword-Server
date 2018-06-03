/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TimingSocket.h
 * Author: Faraday
 *
 * Created on December 13, 2017, 2:22 AM
 */

#ifndef TIMINGSOCKET_H
#define TIMINGSOCKET_H

#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>

using namespace std;

class TimingSocket {
public:
    enum class WaitResult{
        timed_out,
        received_data,
    };
    
    TimingSocket();
    TimingSocket(int socket);
    TimingSocket(const TimingSocket& orig);
    virtual ~TimingSocket();
    
    int getSocket() const;
    void setSocket(int socket);
    
    WaitResult listenFor(int timeout_s);
    
    void close();
protected:
    int socket;
};

#endif /* TIMINGSOCKET_H */

