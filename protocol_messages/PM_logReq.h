/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PM_logReq.h
 * Author: Faraday
 *
 * Created on December 1, 2017, 1:32 AM
 */

#ifndef PM_LOGREQ_H
#define PM_LOGREQ_H

#include "ProtocolMessage.h"
#include <string>


using std::string;
using std::cout;
using std::endl;

class PM_logReq: public ProtocolMessage{
public:
    static const string name;
    
    PM_logReq();
    PM_logReq(const PM_logReq& orig);
    PM_logReq(string username, string password);
    PM_logReq(string code);
    virtual ~PM_logReq();
    
    string getUsername() const;
    string getPassword() const;
    void setUsername(string username);
    void setPassword(string password);
    
    string toString();

private:
    string username;
    string password;
};

#endif /* PM_LOGREQ_H */

