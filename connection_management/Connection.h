/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Connection.h
 * Author: Faraday
 *
 * Created on November 30, 2017, 8:36 PM
 */

#ifndef CONNECTION_H
#define CONNECTION_H



#include "ProtocolMessage.h"
#include "PM_logRep.h"
#include "PM_logOutRep.h"
#include "TimingSocket.h"
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <stdlib.h>
#include <valarray>
#include <string>
#include <mutex>
#include "aes.h"
#include <unistd.h>
#include "base64.h"
#include "./encryption_engines/symmetricengine.h"
#include "./encryption_engines/privateengine.h"

using namespace std;

class Connection : public TimingSocket{
public:
    Connection();
    Connection(const Connection& orig);
    Connection( int socket );
    
    virtual ~Connection();
    
    void sendPM(ProtocolMessage* pm);
    ProtocolMessage* recvPM(); 
    
    bool connError();
private:
    static const int BUFFER_SIZE; //1MB(max image size)
    static const char END_DELIM;
    static const char SIGNATURE_DELIM;
    static const int CLIENT_EXCHANGE_SIZE;
    
    mutex send_mtx;
    bool conn_error;
    
    SymmetricEngine encryption_engine;
    void initEngine();
    
    static string generateRandomString(int length);
    static unsigned int generateSeed();
    
    void clearDelims(string& orig);
};

#endif /* CONNECTION_H */

