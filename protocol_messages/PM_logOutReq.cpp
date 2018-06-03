/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   PM_logOutReq.cpp
 * Author: Faraday
 * 
 * Created on December 2, 2017, 7:27 PM
 */

#include "PM_logOutReq.h"

const string PM_logOutReq::name = "logOutReq";

PM_logOutReq::PM_logOutReq() {
}

PM_logOutReq::PM_logOutReq(const PM_logOutReq& orig) {
}

PM_logOutReq::~PM_logOutReq() {
}

string PM_logOutReq::toString(){
	string code = PM_logOutReq::name + " " + "\n";
	return code;
}
