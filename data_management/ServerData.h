/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ServerData.h
 * Author: Faraday
 *
 * Created on December 1, 2017, 4:46 PM
 */

#ifndef SERVERDATA_H
#define SERVERDATA_H

#include <string>
#include <iostream>
#include <map>
#include "PM_logRep.h"
#include "PM_regRep.h"
#include "PM_msg.h"
#include "PM_addContactCom.h"
#include "PM_updateStatus.h"
#include <mutex>
#include <algorithm>
#include <vector>
#include "Connection.h"
#include <ctime>
#include <fstream>

using namespace std;

class ServerData {
public:
    ServerData();
    ServerData(const ServerData& orig);
    virtual ~ServerData();
    
    PM_logRep checkCredentials(string username, string password);
    PM_regRep checkRegistration(string username, string password);
    
    void unlog(string username);
    void log(string username, Connection* conn);
    
    Connection* getConnection(string username);
    
    bool isRegistered(string username);
    
    string getPassword(string username);
    
    void addCredentials(string username, string password);
    
    bool isLogged(string username);
    
    void block(string from, string who);
    void tryUnblock(string from, string who);
    bool hasBlocked(string from, string who);
    
    bool hasBlockedSomeone(string from);
    vector<string>& getUsersBlockedBy(string from);
    
    void addContact(string publisher, string subscriber);
    void delContact(string publisher, string subscriber);
    bool isContact(string publisher, string subscriber);
    
    vector<string> getSubscribers(string publisher);
    void printSubscribers(string publisher);
    
    bool isOnline(string username);
    
    void queueMessage(ProtocolMessage* pm, string username);
    
    vector<ProtocolMessage*> getPendingDB(string username);
    void restorePendingDB(string username);
    
    string getStatus(string username);
    void setStatus(string username, string status, string date);
    
    string getStatusDate(string username);
    void setStatusDate(string username, string date);
    
    string getImage(string username);
    void setImage(string username, string image);
    
    string getPresence(string username);
    void setPresence(string username, string presence, string date);
    
    string getPresenceDate(string username);
    void setPresenceDate(string username, string date);
    
    void incrementNumberOfThreads();
    void decrementNumberOfThreads();
    int getNumberOfThreads();
    
    void backup();
    void load();
    
    void freeResources();
    
    bool shutdownActivated();
    void activateShutdown();
private:
    int n_threads = 0;
    bool shutdown_activated = false;
    
    map<string, string> credentialsDB;      // Farid$Arbai:password\n
    
    vector<string>  loggedDB;
    
    map<string,Connection*> connectionDB;
    
    map<string,string> statusDB;            // Farid$Arbai:Rolling this thing like really fine\n
    map<string,string> statusDateDB;        // Farid$Arbai:0-10/12/12-18:47\n
    
    map<string,string> imageDB;             // Farid$Arbai:eh9238eh8hd9823hd982hd3d92839hdowuhd\n
    
    map<string, string> presenceDB;         // Farid$Arbai:online\n
    map<string,string> presenceDateDB;      // Farid$Arbai:0-10/12/17-19:54\n
    
    map<string,vector<string>>  subscriptionDB; // Farid$Arbai:uno%dos%Joaquin$Lara%Fran\n
    map<string,vector<string>>  blockedDB;      // Farid$Arbai:hola%alguien\n
    
    map<string, vector<ProtocolMessage*>> pendingDB; // Farid$Arbai:msg...%addContactCom...\n
    
    recursive_mutex data_lock;
    
    const string CREDENTIALS_PATH   =   "./backup/credentials.backup";
    const string STATUS_PATH        =   "./backup/status.backup";
    const string STATUS_DATE_PATH   =   "./backup/statusDate.backup";
    const string IMAGE_PATH         =   "./backup/image.backup";
    const string PRESENCE_PATH      =   "./backup/presence.backup";
    const string PRESENCE_DATE_PATH =   "./backup/presenceDate.backup";
    const string SUBSCRIPTION_PATH  =   "./backup/subscription.backup";
    const string BLOCKED_PATH       =   "./backup/blocked.backup";
    const string PENDING_PATH       =   "./backup/pending.backup";
    
    void backupDB(map<string,string>& db, string PATH);
    void loadDB(map<string,string>& db, string PATH);
    
    void backupDB(map<string,vector<string>>& db, string PATH);
    void loadDB(map<string,vector<string>>& db, string PATH);
    
    void backupDB(map<string,vector<ProtocolMessage*>>& db, string PATH);
    void loadDB(map<string,vector<ProtocolMessage*>>& db, string PATH);
};

#endif /* SERVERDATA_H */

