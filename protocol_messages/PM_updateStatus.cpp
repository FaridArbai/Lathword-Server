/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PM_updateStatus.cpp
 * Author: Faraday
 * 
 * Created on December 2, 2017, 7:24 PM
 */

#include "PM_updateStatus.h"

const string PM_updateStatus::name = "updateStatus";

PM_updateStatus::PM_updateStatus() {
}

PM_updateStatus::PM_updateStatus(const PM_updateStatus& orig) {
	this->setUsername(orig.getUsername());
	this->setNewStatus(orig.getNewStatus());
	this->setDate(orig.getDate());
	this->setType(orig.getType());
}

PM_updateStatus::~PM_updateStatus() {
}

PM_updateStatus::PM_updateStatus(string username, PM_updateStatus::StatusType type, string new_status){
	
	date = ProtocolMessage::getGenerationDate();
	
	this->setUsername(username);
	this->setNewStatus(new_status);
	this->setDate(date);
	this->setType(type);
}

PM_updateStatus::PM_updateStatus(string code){
	string username;
	string new_status_encoded;
	string new_status;
	string str_type;
	string date;
	int i0, iF;
	int pos_first_space;
	int pos_secnd_space;
	int pos_third_space;
	int l_code;
	PM_updateStatus::StatusType type;
	
	l_code = code.length();
	
	pos_first_space = code.find(" ");
	
	i0 = pos_first_space + 1;
	iF = l_code - i0;
	pos_secnd_space = (code.substr(i0,iF)).find(" ") + pos_first_space + 1;
	
	i0 = pos_secnd_space + 1;
	iF = l_code - i0;
	pos_third_space = (code.substr(i0,iF)).find(" ") + pos_secnd_space + 1;
	
	i0 = pos_first_space + 1;
	iF = pos_secnd_space - i0;
	username = code.substr(i0, iF);
	
	i0 = pos_secnd_space + 1;
	iF = pos_third_space - i0;
	date = code.substr(i0, iF);
	
	i0 = pos_third_space + 1;
	iF = l_code - i0;
	new_status_encoded = code.substr(i0, iF);
	
	i0 = new_status_encoded.find(">");
	str_type = new_status_encoded.substr(0,i0);
	
	iF = new_status_encoded.length()-(i0+1);
	new_status = new_status_encoded.substr(i0+1,iF);
	
	type = PM_updateStatus::typeOf(str_type);
	
	this->setUsername(username);		
	this->setDate(date);					
	this->setNewStatus(new_status);
	this->setType(type);					
}
	
string PM_updateStatus::toString(){
	string code = "";
	string head = PM_updateStatus::name;
	string username = this->getUsername();
	string new_status = this->getNewStatus();
	string date = this->getDate();
	string str_type = PM_updateStatus::stringOf(this->getType());
	
	// updateStatus Farid 08/12/17-19:57 status>Making the world a better place
	// updateStatus Farid 08/12/17-19:57 image>jpg.japoudha98d298hd83d93uhd398f
	// updateStatus Farid 08/12/17-19:57 presence>online
	
	code += head + " ";
	code += username + " ";
	code += date + " ";
	code += str_type + ">";
	code += new_status + "\n";
	
	return code;
}
    
string PM_updateStatus::getUsername() const{
	return this->username;
}

void PM_updateStatus::setUsername(string username){
	this->username = username;
}
    
string PM_updateStatus::getNewStatus() const{
	return this->new_status;
}

void PM_updateStatus::setNewStatus(string new_status){
	this->new_status = new_status;
}

string PM_updateStatus::getDate() const{
	return this->date;
}

void PM_updateStatus::setDate(string date){
	this->date = date;
}

PM_updateStatus::StatusType PM_updateStatus::getType() const{
	return this->type;
}

void PM_updateStatus::setType(PM_updateStatus::StatusType type){
	this->type = type;
}

PM_updateStatus::StatusType PM_updateStatus::typeOf(string str_type){
	PM_updateStatus::StatusType type;
	
	if(str_type=="status"){
		type = PM_updateStatus::StatusType::status;
	}
	else if(str_type=="image"){
		type = PM_updateStatus::StatusType::image;
	}
	else if(str_type=="presence"){
		type = PM_updateStatus::StatusType::presence;
	}
	else{
		type = PM_updateStatus::StatusType::unknown;
	}
	
	return type;
}

string PM_updateStatus::stringOf(PM_updateStatus::StatusType type){
	string str_type;
	
	switch(type){
		case(PM_updateStatus::StatusType::status):{
			str_type = "status";
			break;
		}
		case(PM_updateStatus::StatusType::image):{
			str_type = "image";
			break;
		}
		case(PM_updateStatus::StatusType::presence):{
			str_type = "presence";
			break;
		}
		default:{
			str_type = "unknown";
			break;
		}
	}
			  
	return str_type;
}




























