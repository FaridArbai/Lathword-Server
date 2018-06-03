
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <stdlib.h>
#include <sstream>
#include <csignal>

#include "Connection.h"
#include "ServerData.h"
#include "ProtocolMessage.h"
#include "PM_logReq.h"
#include "PM_logRep.h"
#include "PM_regReq.h"
#include "PM_regRep.h"
#include "PM_updateStatus.h"
#include "PM_addContactReq.h"
#include "PM_addContactRep.h"
#include "PM_blockContact.h"
#include "PM_msg.h"
#include "PM_updateStatus.h"
#include "PM_logOutReq.h"
#include "PM_logOutRep.h"
#include "PM_logOutCom.h"
#include "PM_delContact.h"
#include "HostingSocket.h"

using namespace std;

#define L_PORT 25255
#define L_IP "0.0.0.0"
#define MAX_QUEUED_ALLOWED 10
#define TIME_TO_CHECK_SHUTDOWN 30
#define BACKUP_INTERVAL std::chrono::seconds(1800)	// backup every half an hour

enum class State{
	unlogged,
	logged,
	logging_out,
	
};

// Condition variables to synchronize a safe shutdown
// on the server when requested by the admin with a kill
// signal
std::condition_variable START_SHUTDOWN;			// signalHandler  -> mgmt_thread
std::condition_variable CLIENT_THREAD_SHUTTED_DOWN;
std::condition_variable BACKUP_BEFORE_SHUTDOWN;	// mgmt_thread		-> backup_thread
std::condition_variable SHUTDOWN_FINISHED;		// mgmt_thread		-> signalHandler
std::condition_variable FINISH_MAIN;				// signalHandler	->	main_thread

//HEADERS

void serverHostingTask(ServerData* data);
void serverStateMachineTask(int client_socket, ServerData* data);
void backupTask(ServerData* data);
void managementTask(ServerData* data, thread* hosting_thread, thread* backup_thread);
void signalHandler(int signum);

PM_logRep generateLogRep(string username, string password, ServerData* data);
PM_regRep generateRegRep(string username, string password, ServerData* data);
PM_addContactRep generateAddContactRep(string contact,string from, ServerData* data);
PM_addContactCom generateAddContactCom(string username,string contact, ServerData* data);
void sendPendingMessages(string username, Connection* client_conn, ServerData* data);
void updateStatus(ProtocolMessage* pm_upd, ServerData* data);
void forwardStatus(ProtocolMessage* pm_upd, ServerData* data);
PM_updateStatus generatePresence(string username, string presence_str);


int main(int argc, char** argv);

//IMPLEMS

int main(int argc, char** argv) {
	ServerData data = ServerData();
	
	thread hosting_thread	=	thread(serverHostingTask,&data);
	thread backup_thread		=	thread(backupTask,&data);
	thread mgmt_thread		=	thread(managementTask,&data,&hosting_thread,&backup_thread);

	mgmt_thread.detach();
	
	std::signal(SIGTERM,signalHandler);
	std::signal(SIGINT,signalHandler);
	std::signal(SIGHUP,signalHandler);
	
	std::mutex finish_main_mutex;
	std::unique_lock<std::mutex> finish_main_lock(finish_main_mutex);
	
	FINISH_MAIN.wait(finish_main_lock);
	
	return 0;
}

void signalHandler(int signum){
	std::mutex shutdown_finished_mutex;
	std::unique_lock<std::mutex> shutdown_finished_lock(shutdown_finished_mutex);
	
	START_SHUTDOWN.notify_all();
	
	SHUTDOWN_FINISHED.wait(shutdown_finished_lock);
	
	FINISH_MAIN.notify_all();
	
}

void managementTask(ServerData* data, thread* hosting_thread, thread* backup_thread){
	std::mutex start_shutdown_mutex;
	std::unique_lock<std::mutex> start_shutdown_lock(start_shutdown_mutex);
	
	
	START_SHUTDOWN.wait(start_shutdown_lock);
	
	std::this_thread::sleep_for(std::chrono::seconds(10));
	
	data->activateShutdown();
	
	hosting_thread->join();
	
	int n_client_threads = data->getNumberOfThreads();
	
	while(n_client_threads>0){
		std::this_thread::sleep_for(std::chrono::seconds(TIME_TO_CHECK_SHUTDOWN));
		n_client_threads = data->getNumberOfThreads();
	}
	
	BACKUP_BEFORE_SHUTDOWN.notify_all();
	
	backup_thread->join();
	
	SHUTDOWN_FINISHED.notify_all();
}

void backupTask(ServerData* data){
	std::mutex backup_before_shutdown_mutex;
	std::unique_lock<std::mutex> backup_before_shutdown_lock(backup_before_shutdown_mutex);
	std::cv_status waiting_result;
	
	bool shutdown_backup_task = false;
	
	do{
		BACKUP_BEFORE_SHUTDOWN.wait_for(backup_before_shutdown_lock,BACKUP_INTERVAL);
		
		data->backup();
		
		shutdown_backup_task = (data->shutdownActivated());
	}while(!shutdown_backup_task);
	
}

void serverHostingTask(ServerData* data){
	HostingSocket hosting_socket = HostingSocket(L_IP, L_PORT, MAX_QUEUED_ALLOWED);	
	TimingSocket::WaitResult waiting_result;
	
	int client_socket;
	bool shutdown_activated = false; 
	
	while(!shutdown_activated){
		waiting_result = hosting_socket.listenFor(TIME_TO_CHECK_SHUTDOWN);
		
		shutdown_activated = data->shutdownActivated();
		
		if(shutdown_activated){
			hosting_socket.close();
		}
		else{
			if(waiting_result==TimingSocket::WaitResult::received_data){
			
				client_socket = hosting_socket.acceptConnections();
				
				thread(serverStateMachineTask, client_socket, data).detach();
				
				data->incrementNumberOfThreads();
			}
		}
		
	}	
	
}

void serverStateMachineTask(int client_socket, ServerData* data){
	State state = State::unlogged;
	Connection client_conn = Connection(client_socket);
	ProtocolMessage* pm_recv;
	bool is_closed = false;
	bool is_undefined = false;
	bool keep_connection = (!client_conn.connError());
	ProtocolMessage::MessageType m_type;
	Connection::WaitResult wait_result;
	bool listen_timed_out	=	false;
	bool shutdown_activated	=	false;
	bool listen		=	true;
	
	string client_name;
	
	struct timeval tv;
	tv.tv_sec = TIME_TO_CHECK_SHUTDOWN;
	tv.tv_usec = 0;
	
	if(!keep_connection){
		client_conn.close();
	}
	
	while(keep_connection){
		
		do{
			wait_result = client_conn.listenFor(TIME_TO_CHECK_SHUTDOWN);
			listen_timed_out = (wait_result == Connection::WaitResult::timed_out);
			shutdown_activated = data->shutdownActivated();
			listen = (!shutdown_activated)&&(listen_timed_out);
		}while(listen);
		
		if(!shutdown_activated){
			pm_recv = client_conn.recvPM();

			is_closed = (pm_recv==nullptr);

			if(!is_closed){
				m_type = ProtocolMessage::getMessageTypeOf(pm_recv);
				is_undefined = (m_type==ProtocolMessage::MessageType::undefined);
			}

			keep_connection = (!is_closed)&&(!is_undefined);
		}
		else{
			keep_connection = false;
		}
		
		if (keep_connection){
			//Switch between the states
			switch(state){				
				
				case (State::unlogged):{
					switch(m_type){
						case ProtocolMessage::MessageType::logReq :{
							string username;
							string password;
							PM_logRep pm_send;
							bool is_correct;

							username = ((PM_logReq*)pm_recv)->getUsername();
							password = ((PM_logReq*)pm_recv)->getPassword();

							pm_send = generateLogRep(username,password,data);

							is_correct = pm_send.getResult();

							if(is_correct){
								state = State::logged;
								client_name = username;
								data->log(client_name, &client_conn);
							}
							else{
								state = State::unlogged;
							}

							client_conn.sendPM(&pm_send);
							
							if(is_correct){
								//1. Send pending message queued for this user
									sendPendingMessages(client_name,&client_conn,data);
									
								//2. Update presence and forward it to each person that has this user as contact
									
									PM_updateStatus presence_update = generatePresence(client_name, "online");
									
									updateStatus(&presence_update,data);
									forwardStatus(&presence_update,data);
							}

							break;
						}
						case ProtocolMessage::MessageType::regReq :{
							string username;
							string password;
							PM_regRep pm_send;

							username = ((PM_regReq*)pm_recv)->getUsername();
							password = ((PM_regReq*)pm_recv)->getPassword();

							pm_send = generateRegRep(username, password, data);

							client_conn.sendPM(&pm_send);

							break;
						}
						default:{
							cout<< "Unexpected message in state (unlogged)" <<endl;
							break;
						}
					}
					break;
				}
				case(State::logged):{
					switch(m_type){
						case(ProtocolMessage::MessageType::addContactReq) :{
							string contact;
							string from;
							PM_addContactRep pm_send;
							bool has_been_added;
							
							contact = ((PM_addContactReq*)pm_recv)->getContact();
							from = ((PM_addContactReq*)pm_recv)->getFrom();
							
							pm_send = generateAddContactRep(contact,from,data);
							
							client_conn.sendPM(&pm_send);
							break;
						}
						case(ProtocolMessage::MessageType::msg) :{
							string from;
							string to;
							PM_msg pm_send;
							bool receipt_is_online;
							bool receipt_has_blocked_sender;
							bool receipt_has_sender_as_contact;
							
							from = ((PM_msg*)pm_recv)->getFrom();
							to = ((PM_msg*)pm_recv)->getTo();
							
							receipt_is_online = data->isOnline(to);					
							receipt_has_blocked_sender = data->hasBlocked(to,from);
							
							if(!receipt_has_blocked_sender){
								// 1. Force receipt to add sender
								receipt_has_sender_as_contact = (data->isContact(from,to));
								
								if(!receipt_has_sender_as_contact){
									PM_addContactCom command = generateAddContactCom(to,from,data);
									ProtocolMessage* pm_command = &command;
									
									if(!receipt_is_online){
										data->queueMessage(pm_command,to);
									}
									else{
										Connection* receipt_conn = (data->getConnection(to));
										receipt_conn->sendPM(pm_command);
									}
								}
								
								if(!receipt_is_online){
									data->queueMessage(pm_recv,to);
								}
								else{
									Connection* receipt_conn = data->getConnection(to);
									receipt_conn->sendPM(pm_recv);
								}
							}
							break;
						}
						case(ProtocolMessage::MessageType::delContact):{
							string from = ((PM_delContact*)pm_recv)->getFrom();
							string to = ((PM_delContact*)pm_recv)->getTo();
							bool has_blocked_you = data->hasBlocked(to,from);
							
							if(!has_blocked_you){
								data->delContact(to,from);
							}
							break;
						}
						case(ProtocolMessage::MessageType::blockContact) :{
							string from;
							string contact;
							
							from = ((PM_blockContact*)pm_recv)->getFrom();
							contact = ((PM_blockContact*)pm_recv)->getContact();
							
							data->block(from,contact);
							break;
						}
						case(ProtocolMessage::MessageType::updateStatus) :{
							
							updateStatus(pm_recv,data);
							forwardStatus(pm_recv,data);
							
							break;
						}
						case(ProtocolMessage::MessageType::logOutReq) :{
							PM_logOutRep pm_send = PM_logOutRep();
							
							client_conn.sendPM(&pm_send);
							
							state = State::logging_out;
							break;
						}
						default:{
							break;
						}
					}
					break;
				}
				default:{
					break;
				}
			} //end switch(state)
		
			//Delete the allocated memory for the pm_recv
			delete pm_recv;
			
		}//end if(keep_connection)
		
		
		// Handle the end of the connection, wether because the user ended it up
		// 
		
		if(keep_connection==false){
			if(is_closed){
			}
			else if(is_undefined){
				
				string err_msg =
						  "Received unknown encoded-message. Please report this behaviour to user Farid";
				
				PM_logOutCom pm_send = PM_logOutCom(err_msg);
				
				client_conn.sendPM(&pm_send);
				client_conn.close();
			}
			else if(shutdown_activated){
				
				string err_msg = 
						  "Server has gone down for maintenance, please check back soon! :)";
				PM_logOutCom pm_send = PM_logOutCom(err_msg);
				
				client_conn.sendPM(&pm_send);
				client_conn.close();
			}
		}
		
		bool logged_out = (state==State::logging_out)||((keep_connection==false)&&(state==State::logged));
		
		if(logged_out){
				data->unlog(client_name);
				PM_updateStatus pm_presence = generatePresence(client_name, "offline");
				updateStatus(&pm_presence, data);
				forwardStatus(&pm_presence, data);
		}
		
		if(state==State::logging_out){
			state = State::unlogged;
		}
	
	}// end while(keep_connection);
	
	data->decrementNumberOfThreads();
	
}

PM_logRep generateLogRep(string username, string password, ServerData* data){
	PM_logRep pm_logRep;
	bool is_registered;
	bool is_correct;
	string err_str;
	
	is_registered = data->isRegistered(username);
	
	if(is_registered){
		string actual_password = data->getPassword(username);
		
		if(actual_password == password){
			bool is_logged;
			
			is_logged = data->isLogged(username);
			
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

PM_regRep generateRegRep(string username, string password, ServerData* data){
	PM_regRep pm_regRep;
	bool is_registered;
	string err_str;
	bool is_correct;
	
	is_registered = data->isRegistered(username);
	
	if(is_registered==false){
		is_correct = true;
		err_str = "-";
				
		data->addCredentials(username,password);
	}
	else{
		is_correct = false;
		err_str = "User {" + username + "} already exists!";
	}
	
	pm_regRep = PM_regRep(is_correct,err_str);
	
	return pm_regRep;
}

PM_addContactRep generateAddContactRep(string contact,string from, ServerData* data){
	PM_addContactRep pm;
	bool result;
	string err_str;
	string status;
	string status_date;
	string image;
	string presence;
	string presence_date;
	bool is_registered;
	bool has_blocked_you;
	
	is_registered = data->isRegistered(contact);
	
	if(!is_registered){
		result = false;
		err_str = "Username {"+contact+"} does not exist!";
	}
	else{
		data->tryUnblock(from,contact);
		has_blocked_you = data->hasBlocked(contact,from);
		if(has_blocked_you){
			result=false;
			err_str = "Username {"+contact+"} has blocked you";			
		}
		else{
			result = true;
			err_str = "Contact {"+contact+"} has been added succesfully!";
			data->addContact(contact,from);
			status = data->getStatus(contact);
			status_date = data->getStatusDate(contact);
			image = data->getImage(contact);
			presence = data->getPresence(contact);
			presence_date = data->getPresenceDate(contact);
		}
	}
	
	if(result==false){
		pm = PM_addContactRep(false,err_str);
	}
	else{
		pm = PM_addContactRep(status,status_date,image,presence,presence_date);
	}
	
	return pm;
}

PM_addContactCom generateAddContactCom(string username,string contact, ServerData* data){
	PM_addContactCom pm;
	string status;
	string status_date;
	string image;
	string presence;
	string presence_date;
	
	data->addContact(contact,username);
	
	status = data->getStatus(contact);
	status_date = data->getStatusDate(contact);
	image = data->getImage(contact);
	presence = data->getPresence(contact);
	presence_date = data->getPresenceDate(contact);
	
	pm = PM_addContactCom(contact,status,status_date,image,presence,presence_date);
	
	return pm;
}

void sendPendingMessages(string username, Connection* client_conn, ServerData* data){
	vector<ProtocolMessage*> pendingDB = data->getPendingDB(username);
	int n_pending = pendingDB.size();
	
	
	if(n_pending>0){
		ProtocolMessage* pm;
		
		for(int i=0; i<n_pending; i++){
			pm = pendingDB.at(i);
		}
		
		data->restorePendingDB(username);
	}
}

PM_updateStatus generatePresence(string username, string presence_str){
	PM_updateStatus pm_update = PM_updateStatus(username,PM_updateStatus::StatusType::presence,presence_str);
	return pm_update;
}

void updateStatus(ProtocolMessage* pm_upd, ServerData* data){
	string status;
	string username;
	string date;
	PM_updateStatus::StatusType type;
							
	username = ((PM_updateStatus*)pm_upd)->getUsername();
	status = ((PM_updateStatus*)pm_upd)->getNewStatus();
	date = ((PM_updateStatus*)pm_upd)->getDate();
	type = ((PM_updateStatus*)pm_upd)->getType();
							
	switch(type){
		case(PM_updateStatus::StatusType::status):{
			data->setStatus(username,status,date);
			break;
		}
		case(PM_updateStatus::StatusType::image):{
			data->setImage(username,status);
			break;
		}
		case(PM_updateStatus::StatusType::presence):{
			data->setPresence(username,status,date);
			break;
		}
	}
}

void forwardStatus(ProtocolMessage* pm_upd, ServerData* data){
	string originator = ((PM_updateStatus*)pm_upd)->getUsername();
	vector<string> subscribers = data->getSubscribers(originator);
	int n_subscribers = subscribers.size();
	string subscriber;
	bool is_online;
	Connection* conn;
	
	for(int i=0; i<n_subscribers; i++){
		subscriber = subscribers.at(i);
		is_online = data->isOnline(subscriber);
		if(is_online){
			conn = data->getConnection(subscriber);
			conn->sendPM(pm_upd);
		}
		else{
			data->queueMessage(pm_upd,subscriber);
		}
	}
}


