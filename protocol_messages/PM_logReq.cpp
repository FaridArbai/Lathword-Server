/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PM_logReq.cpp
 * Author: Faraday
 * 
 * Created on December 1, 2017, 1:32 AM
 */

#include "PM_logReq.h"
#include "Connection.h"

const string PM_logReq::name = "logReq";

PM_logReq::PM_logReq() {
}

PM_logReq::PM_logReq(string username, string password){
	this->setUsername(username);
	this->setPassword(password);
}

PM_logReq::PM_logReq(string code){
    string username;
    string password;
	int i0,iF;
	int pos_first_space;
	int pos_secnd_space;
	int l_code;

	l_code = code.length();
	
	pos_first_space = code.find(" ");
	
	i0 = pos_first_space + 1;
	iF = l_code - i0;
	pos_secnd_space = (code.substr(i0,iF)).find(" ") + pos_first_space + 1;
	
	i0 = pos_first_space + 1;
	iF = pos_secnd_space - i0;
    username = code.substr(i0,iF);
	
	i0 = pos_secnd_space + 1;
	iF = l_code - i0;
    password= code.substr(i0,iF);

	this->setUsername(username);	cout << "logReq:: username set to ->" + username <<endl;
	this->setPassword(password);	cout << "logReq:: password set to ->" + password <<endl;
}

PM_logReq::PM_logReq(const PM_logReq& orig) {
	this->setUsername(orig.getUsername());
	this->setPassword(orig.getPassword());
}

PM_logReq::~PM_logReq() {
}

string PM_logReq::toString(){
	string str = "";
	string username = this->getUsername();
	string password = this->getPassword();
			  
	str += PM_logReq::name + " ";
	str += username + " ";
	str += password + "\n";
	
	return str;
}


void PM_logReq::setUsername(string username){
	this->username = username;
	cout << "username set to : " + username << endl;
}

string PM_logReq::getUsername() const{
	return this->username;
}

void PM_logReq::setPassword(string password){
	this->password = password;
	cout << "password set to : " + password << endl;
}
		
string PM_logReq::getPassword() const{
	return this->password;
}












































