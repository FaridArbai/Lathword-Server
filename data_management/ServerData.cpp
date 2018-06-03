/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ServerData.cpp
 * Author: Faraday
 * 
 * Created on December 1, 2017, 4:46 PM
 */

#include <vector>
#include <algorithm>

#include "ServerData.h"

#define LOCK ((this->data_lock).lock())
#define UNLOCK ((this->data_lock).unlock())

ServerData::ServerData() {
	this->load();
}

ServerData::ServerData(const ServerData& orig) {
}

ServerData::~ServerData() {
	this->freeResources();
}



PM_logRep ServerData::checkCredentials(string username, string password){
	PM_logRep pm_logRep;
	bool is_registered;
	bool is_correct;
	string err_str;
	
	is_registered = this->isRegistered(username);
	
	if(is_registered){
		string actual_password = this->getPassword(username);
		
		cout <<endl<<"{"+username+"::"+password+"}"<<endl;
		
		if(actual_password == password){
			bool is_logged;
			
			is_logged = this->isLogged(username);
			
			if(!is_logged){
				is_correct = true;
				err_str = "-";
			}
			else{
				is_correct = false;
				err_str = "User {" + username + "} is already logged in!";
			}
		}
		else{
			is_correct = false;
			err_str = "Wrong password for username {" + username + "}!";
		}
	}
	else{
		is_correct = false;
		err_str = "User {" + username + "} does not exist!";
	}
	
	pm_logRep = PM_logRep(is_correct,err_str);
	
	return pm_logRep;
}


PM_regRep ServerData::checkRegistration(string username, string password){
	PM_regRep pm_regRep;
	bool is_registered;
	string err_str;
	bool is_correct;
	
	is_registered = this->isRegistered(username);
	
	if(is_registered==false){
		is_correct = true;
		err_str = "-";
		
		cout <<endl<<"{"+username+"::"+password+"}"<<endl;
		
		this->addCredentials(username,password);
	}
	else{
		is_correct = false;
		err_str = "User {" + username + "} already exists!";
	}
	
	pm_regRep = PM_regRep(is_correct,err_str);
	
	return pm_regRep;
}

//SYNCHRONIZED
void ServerData::unlog(string username){
	LOCK;
	
		loggedDB.erase(std::remove(loggedDB.begin(), loggedDB.end(), username), loggedDB.end());
	
		connectionDB.erase(username);
	
	UNLOCK;
}

//SYNCHRONIZED
void ServerData::log(string username, Connection* conn){
	LOCK;
	
		loggedDB.push_back(username);
		connectionDB.insert(std::pair<string,Connection*>(username,conn));
	
	UNLOCK;
}


Connection* ServerData::getConnection(string username){
	LOCK;
	
		Connection* conn = connectionDB[username];
	
	UNLOCK;
	
	return conn;
}


//SYNCHRONIZED
bool ServerData::isRegistered(string username){
	LOCK;
	
		bool is_registered;
		is_registered = (credentialsDB.find(username) != credentialsDB.end());
		
	UNLOCK;
	
	return is_registered;
}

//SYNCHRONIZED
string ServerData::getPassword(string username){
	LOCK;
	
		string password;
		password = credentialsDB.at(username);
	
	UNLOCK;
	return password;
}

//SYNCHRONIZED
void ServerData::addCredentials(string username, string password){
	LOCK;
		vector<string> subscribers;
		
		vector<ProtocolMessage*> pending;
		
		credentialsDB.insert( std::pair<string,string>(username,password) );
		subscriptionDB.insert( std::pair<string,vector<string>>(username,subscribers));
		
		pendingDB.insert( std::pair<string,vector<ProtocolMessage*>>(username,pending) );
		
		string default_status = "Hey there! I'm using cryptor";
		string default_image = "icon.jpg";
		
		statusDB.insert(std::pair<string,string>(username,default_status));
		imageDB.insert(std::pair<string,string>(username,default_image));
		presenceDB.insert(std::pair<string,string>(username,"offline"));
		
		string date = ProtocolMessage::getGenerationDate();
		
		statusDateDB.insert(std::pair<string,string>(username,date));
		presenceDateDB.insert(std::pair<string,string>(username,date));
		
	UNLOCK;
}

//SYNCHRONIZED
bool ServerData::isLogged(string username){
	LOCK;
		
		bool is_logged;
		is_logged = (std::find(loggedDB.begin(), loggedDB.end(), username)!=loggedDB.end());
	
	UNLOCK;
	
	return is_logged;
}

/*************************************************************************************
 BLOCKING USERS
**************************************************************************************/

//SYNCHRONIZED
void ServerData::block(string from, string who){
	LOCK;
		
		bool has_blocked_someone;
		
		bool uwho_is_contact_of_ufrom;
		bool ufrom_is_contact_of_uwho;
		
		has_blocked_someone = this->hasBlockedSomeone(from);
	
		if(has_blocked_someone){
			vector<string>& users_blocked = this->getUsersBlockedBy(from);
			users_blocked.push_back(who);
		}
		else{
			vector<string> users_blocked;
			
			users_blocked.push_back(who);
			blockedDB.insert( std::pair<string,vector<string>>(from,users_blocked) );
		}
		
		uwho_is_contact_of_ufrom = this->isContact(who,from);
		if(uwho_is_contact_of_ufrom){
			this->delContact(who,from);
		}
		
		ufrom_is_contact_of_uwho = this->isContact(from,who);
		if(ufrom_is_contact_of_uwho){
			this->delContact(from,who);
		}
		
		cout << from << "HA BLOQUEADO A" << blockedDB[from].at(0) << endl;
		
	UNLOCK;
}


// SYNCHRONIZED
void ServerData::tryUnblock(string from, string who){
	LOCK;
	
		bool has_blocked_someone;
		bool blocked_him;
		vector<string>* users_blocked;
	
		has_blocked_someone = this->hasBlockedSomeone(from);
	
		if(has_blocked_someone){
			blocked_him = this->hasBlocked(from,who);
			if(blocked_him){
				vector<string>& users_blocked = this->getUsersBlockedBy(from);
				users_blocked.erase(std::remove(
					users_blocked.begin(), users_blocked.end(), who),
						users_blocked.end());
				
				if (users_blocked.size()==0){
					blockedDB.erase(from);
				}
			}
		}
		
	UNLOCK;
}

//SYNCHRONIZED
bool ServerData::hasBlocked(string from, string who){
	LOCK;
	
		bool has_blocked_someone;
		bool is_blocked;
	
		has_blocked_someone = this->hasBlockedSomeone(from);
	
		if(has_blocked_someone){
			vector<string>& users_blocked = this->getUsersBlockedBy(from);
			
			cout << "BLOQUEADOS POR "+from+ users_blocked.at(0)<<endl;
			
			is_blocked = ((std::find(users_blocked.begin(), users_blocked.end(), who))!=users_blocked.end());
		}
		else{
			is_blocked = false;
		}
	
		cout << from << (is_blocked?"SI":"NO") << " HA BLOQUEADO A " << who << endl;
		
	UNLOCK;
	return is_blocked;
}

//ACCESED FROM SYNCHRONIZED FUNCTIONS
bool ServerData::hasBlockedSomeone(string from){
	bool has_blocked_someone;
	
	has_blocked_someone =( blockedDB.find(from) != blockedDB.end() );
	
	return has_blocked_someone;
}

//ACCESED FROM SYNCHRONIZED FUNCTIONS
vector<string>& ServerData::getUsersBlockedBy(string from){
	vector<string>& users_blocked = blockedDB[from];
	
	return users_blocked;
}


void ServerData::addContact(string publisher, string subscriber){
	LOCK;
		vector<string>& subscribers = subscriptionDB[publisher];
		
		subscribers.push_back(subscriber);
	
		printSubscribers(publisher);
		
	UNLOCK;
}

void ServerData::delContact(string publisher, string subscriber){
	LOCK;
		vector<string>& subscribers = subscriptionDB[publisher];
		
		subscribers.erase(std::remove(subscribers.begin(),subscribers.end(),subscriber),subscribers.end());
		
		printSubscribers(publisher);
	UNLOCK;
}

bool ServerData::isContact(string publisher, string subscriber){
	LOCK;
		vector<string>& subscribers = subscriptionDB[publisher];
		bool is_contact;
		
		is_contact = (std::find(subscribers.begin(),subscribers.end(),subscriber)!=(subscribers.end()));
		
		string res = ((is_contact)?"YES":"NO");
		
		cout << publisher + " is contact of" + subscriber + "? : " + res<< endl;
		
		printSubscribers(publisher);
		
	UNLOCK;
	return is_contact;
}

vector<string> ServerData::getSubscribers(string publisher){
	LOCK;
		vector<string> subscribers = subscriptionDB[publisher];
	UNLOCK;
	return subscribers;
}

void ServerData::printSubscribers(string publisher){
	LOCK;
		vector<string>& subscribers = subscriptionDB[publisher];
		int n_subscribers;
		
		n_subscribers = subscribers.size();
		
		cout<<"Users subscripted to "+publisher+":"<<endl;
		
		for(int i=0; i<n_subscribers; i++){
			cout<<"{"+subscribers.at(i)+"}"<<endl;
		}
		
	UNLOCK;
}

bool ServerData::isOnline(string username){
	LOCK;
		bool is_online;
		
		is_online = (std::find(loggedDB.begin(),loggedDB.end(),username)!=loggedDB.end());
		
		cout<<username<<" is online? "<<(is_online?"YES":"NO")<<endl;
		
	UNLOCK;
	return is_online;
}
    

void ServerData::queueMessage(ProtocolMessage* pm, string username){
	LOCK;
		vector<ProtocolMessage*>& ref_pendingDB = pendingDB[username];
		ProtocolMessage::MessageType m_type = ProtocolMessage::getMessageTypeOf(pm);
		ProtocolMessage* ptr_to_stored_pm;
		
		switch(m_type){
			case(ProtocolMessage::MessageType::msg):{
				PM_msg* ptr_msg = dynamic_cast<PM_msg*>(pm);
				ptr_to_stored_pm = new PM_msg((*ptr_msg));
				break;
			}
			case(ProtocolMessage::MessageType::addContactCom):{
				PM_addContactCom* ptr_com = dynamic_cast<PM_addContactCom*>(pm);
				ptr_to_stored_pm = new PM_addContactCom((*ptr_com));
				break;
			}
			case(ProtocolMessage::MessageType::updateStatus):{
				PM_updateStatus* ptr_upd = dynamic_cast<PM_updateStatus*>(pm);
				ptr_to_stored_pm = new PM_updateStatus((*ptr_upd));
				break;
			}
			default:{
				printf("\n\n\n\n********QUEUE ERROR**********\n\n\n\n");
				break;
			}
		}
		
		printf("Queue for %s : {%s}",username.c_str(),(pm->toString()).c_str());
		
		ref_pendingDB.push_back(ptr_to_stored_pm);
		
	UNLOCK;
}

vector<ProtocolMessage*> ServerData::getPendingDB(string username){
	LOCK;
		vector<ProtocolMessage*> ptr_pending;
		
		ptr_pending = pendingDB[username];
	UNLOCK;
	
	return ptr_pending;
}

void ServerData::restorePendingDB(string username){
	LOCK;
	vector<ProtocolMessage*>& ref_pm	=	pendingDB[username];
	
	int n_pm	 = ref_pm.size();				printf("pms : %d\n", n_pm);	
 	
	if(n_pm>0){
		ProtocolMessage* pm;
		for(int i=0; i<n_pm; i++){
			pm = ref_pm.at(i);
			delete pm;
		}
		
		ref_pm.clear();
		
		printf("erased all pms\n");
	}
	
	UNLOCK;
}

string ServerData::getStatus(string username){
	LOCK;
		string status = statusDB[username];
	UNLOCK;
	return status;
}

void ServerData::setStatus(string username, string status, string date){
	LOCK;
		statusDB[username] = status;
		this->setStatusDate(username,date);
	UNLOCK;
}

string ServerData::getStatusDate(string username){
	LOCK;
		string date = statusDateDB[username];
	UNLOCK;
	return date;
}

void ServerData::setStatusDate(string username, string date){
	LOCK;
		statusDateDB[username] = date;
	UNLOCK;
}

string ServerData::getImage(string username){
	LOCK;
		string image = imageDB[username];
	UNLOCK;
	return image;
}

void ServerData::setImage(string username, string image){
	LOCK;
		imageDB[username] = image; 
	UNLOCK;
}

string ServerData::getPresence(string username){
	LOCK;
		string presence = presenceDB[username];
	UNLOCK;
	return presence;
}

void ServerData::setPresence(string username, string presence, string date){
	LOCK;
		presenceDB[username] = presence;
		this->setPresenceDate(username,date);
	UNLOCK;
}

string ServerData::getPresenceDate(string username){
	LOCK;
		string date = presenceDateDB[username];
	UNLOCK;
	return date;
}

void ServerData::setPresenceDate(string username, string date){
	LOCK;
		presenceDateDB[username] = date;
	UNLOCK;
}

void ServerData::incrementNumberOfThreads(){
	LOCK;
		this->n_threads += 1;
	UNLOCK;
}
    
void ServerData::decrementNumberOfThreads(){
	LOCK;
		this->n_threads -= 1;
	UNLOCK;
}
    
int ServerData::getNumberOfThreads(){
	LOCK;
	int number_of_threads = this->n_threads;
	UNLOCK;
	return number_of_threads;
}

void ServerData::backup(){
	LOCK; 
		this->backupDB(credentialsDB,CREDENTIALS_PATH);
		this->backupDB(statusDB,STATUS_PATH);
		this->backupDB(statusDateDB,STATUS_DATE_PATH);
		this->backupDB(presenceDateDB,PRESENCE_DATE_PATH);
		this->backupDB(imageDB,IMAGE_PATH);
		this->backupDB(subscriptionDB,SUBSCRIPTION_PATH);
		this->backupDB(blockedDB,BLOCKED_PATH);
		this->backupDB(pendingDB,PENDING_PATH);
	UNLOCK;
}

void ServerData::load(){
	LOCK;
		this->loadDB(credentialsDB,CREDENTIALS_PATH);
		this->loadDB(statusDB,STATUS_PATH);
		this->loadDB(statusDateDB,STATUS_DATE_PATH);
		this->loadDB(presenceDateDB,PRESENCE_DATE_PATH);
		this->loadDB(imageDB,IMAGE_PATH);
		this->loadDB(subscriptionDB,SUBSCRIPTION_PATH);
		this->loadDB(blockedDB,BLOCKED_PATH);
		this->loadDB(pendingDB,PENDING_PATH);
	UNLOCK;
}

void ServerData::backupDB(map<string,string>& db, string PATH){
	std::ofstream file = std::ofstream(PATH);
	string line;
	string key;
	string value;
	
	for(auto& it : db){
		key = it.first;
		value = it.second;
		line = key + ":" + value + "\n";
		file << line;
	}
	
	file.close();
}

void ServerData::backupDB(map<string,vector<ProtocolMessage*>>& db, string PATH){
	std::ofstream file = std::ofstream(PATH);
	string line;
	string key;
	vector<ProtocolMessage*> value;
	vector<string>	values_str;
	int n_elem;
	string value_str;
	
	for(auto& it : db){
		key = it.first;
		value = it.second;
		n_elem = value.size();
		value_str = "";
		
		for(int i=0; i<n_elem; i++){
			value_str += (value.at(i)->toString());
			value_str.erase(std::remove(value_str.begin(), value_str.end(), '\n'), value_str.end());
			if(i!=(n_elem-1)){
				value_str+="|";
			}
		}
		
		line = key + ":" + value_str + "\n";
		file << line;
	}
	
	file.close();
}

void ServerData::backupDB(map<string,vector<string>>& db, string PATH){
	std::ofstream file = std::ofstream(PATH);
	string line;
	string key;
	vector<string> value;
	int n_elem;
	string elem;
	string value_str;
	
	for(auto& it : db){
		key = it.first;
		value = it.second;
		n_elem = value.size();
		value_str = "";
		
		for(int i=0; i<n_elem;i++){
			value_str += value.at(i);
			if(i!=(n_elem-1)){
				value_str+="|";
			}
		}
		
		line = key + ":" + value_str + "\n";
		file << line;
	}
	
	file.close();
}


void ServerData::loadDB(map<string,string>& db, string PATH){
	std::ifstream file = std::ifstream(PATH);
	string line;
	bool reached_eof = false;
	string key;
	string value;
	int pos_split;
	int init;
	int len;
	
	while(!reached_eof){
		reached_eof = (std::getline(file,line,'\n')).eof();
		if(!reached_eof){
			pos_split = line.find(":");
			
			init = 0;
			len = pos_split;
			key = line.substr(init,len);
			
			init = pos_split+1;
			len = line.length()-init;
			value = line.substr(init,len);
			
			db.insert(std::pair<string,string>(key,value));
		}
	}
	
}

void ServerData::loadDB(map<string,vector<ProtocolMessage*>>& db, string PATH){
	map<string,vector<string>> db_str;
	string key;
	vector<string> value_strs;
	vector<ProtocolMessage*> value;
	ProtocolMessage* pm;
	string pm_str;
	int n_elem;
	
	loadDB(db_str,PATH);
	
	for(auto& it : db_str){
		key = it.first;
		value_strs = it.second;
		n_elem = value_strs.size();
		
		for(int i=0; i<n_elem; i++){
			pm_str = value_strs.at(i);
			pm = ProtocolMessage::decode(pm_str);
			value.push_back(pm);
		}
		
		db.insert(std::pair<string,vector<ProtocolMessage*>>(key,value));
		value.clear();
	}
	
}

void ServerData::loadDB(map<string,vector<string>>& db, string PATH){
	std::ifstream file = std::ifstream(PATH);
	string line;
	bool reached_eof = false;
	string key;
	vector<string> value;
	int pos_split;
	int init;
	int len;
	string elem;
	string value_str_tmp;
	bool finished_line;
	
	while(!reached_eof){
		reached_eof = (std::getline(file,line,'\n')).eof();
		if(!reached_eof){
			pos_split = line.find(":");
			
			init = 0;
			len = pos_split;
			key = line.substr(init,len);
			
			init = pos_split+1;
			len = line.length()-init;
			value_str_tmp = line.substr(init,len);
			
			finished_line = false;
			
			cout << key << endl;
			
			while(!finished_line){
				pos_split = value_str_tmp.find("|"); 
				
				if(pos_split!=string::npos){
					init = 0;
					len = pos_split;
					elem = value_str_tmp.substr(init,len);
					
					init = pos_split + 1;
					len = value_str_tmp.length() - init;
					value_str_tmp = value_str_tmp.substr(init,len);
				}
				else{
					finished_line = true;
					elem = value_str_tmp;
					value_str_tmp = "";
				}
				
				cout << "-"+elem << endl;
				
				if(elem!=""){
					value.push_back(elem);
				}
			}
			db.insert(std::pair<string,vector<string>>(key,value));
			value.clear();
		}
	}
	
}

void ServerData::freeResources(){
	LOCK;
		vector<ProtocolMessage*> v_pms;
		ProtocolMessage* pm;
		int n_pms;
			  
		for(auto& it : pendingDB){
			v_pms = it.second;
			n_pms = v_pms.size();
			for(int i=0; i<n_pms; i++){
				pm = v_pms.at(i);
				delete pm;
			}
		}
	
	UNLOCK;
}

bool ServerData::shutdownActivated(){
	LOCK;
		bool is_activated = this->shutdown_activated;
	UNLOCK;
	return is_activated;
}

void ServerData::activateShutdown(){
	LOCK;
		this->shutdown_activated = true;
	UNLOCK;
}