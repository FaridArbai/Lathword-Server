/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Connection.cpp
 * Author: Faraday
 * 
 * Created on November 30, 2017, 8:36 PM
 */

#include "Connection.h"

const int Connection::BUFFER_SIZE = 1048576;
const char Connection::END_DELIM = '~';
const char Connection::SIGNATURE_DELIM = '#';
const int Connection::CLIENT_EXCHANGE_SIZE = 345;


Connection::Connection() {}

Connection::Connection(const Connection& orig){
	this->setSocket(orig.getSocket());
	cout << "ERROR : COPIA POR VALOR DE CONNECTION" << endl;
}

Connection::Connection(int socket) {
	this->setSocket(socket);
	this->initEngine();
}

Connection::~Connection() {
}

void Connection::sendPM(ProtocolMessage* pm){
	unique_lock<mutex> lock(this->send_mtx);
	
	int client_socket = this->getSocket();
   string payload; 
	string str_send;
    const char* buffer_encr;
    int encr_len;
    ProtocolMessage::MessageType m_type = ProtocolMessage::getMessageTypeOf(pm);

    str_send = pm->toString();

    this->clearDelims(str_send);

    payload = encryption_engine.encrypt(str_send);
	 
	 str_send = payload + Connection::SIGNATURE_DELIM +
				PrivateEngine::sign(payload) + Connection::END_DELIM;
	 
    buffer_encr = str_send.c_str();
    encr_len = str_send.length();

    send(client_socket, buffer_encr, encr_len, 0);
}

ProtocolMessage* Connection::recvPM(){
    int client_socket =	this->getSocket();
    char* buffer =	nullptr;

    buffer = (char*)malloc(Connection::BUFFER_SIZE*sizeof(char));

    for(int i=0; i<Connection::BUFFER_SIZE; i++){
        buffer[i] = '\0';
    }
	
    char* sub_buffer = buffer;
    int offset = 0;
    bool finished_recv = false;
    ssize_t n_bytes_chunk;
    ssize_t n_bytes_data;
    int sub_buffer_size;
    int pos_delim;
    bool found_delim = false;
    char* ptr_delim;
	 int buffer_size = Connection::BUFFER_SIZE;
	 
    while(!finished_recv){
        sub_buffer_size = buffer_size - offset;
		  
        n_bytes_chunk = recv(client_socket, sub_buffer, sub_buffer_size, MSG_PEEK);
		  
        if(n_bytes_chunk>0){
            ptr_delim = strchr(sub_buffer,Connection::END_DELIM);
				
            if(ptr_delim==nullptr){
                found_delim = false;
            }
            else{
                pos_delim = (int)(ptr_delim - sub_buffer);
                found_delim = true;
            }
				
            if(!found_delim){
               recv(client_socket, sub_buffer, n_bytes_chunk, 0);

               offset += n_bytes_chunk;
					 
					if(offset==buffer_size){
						buffer_size *= 2;
						buffer = (char*)realloc(buffer,buffer_size);
						for(int i=offset; i<buffer_size; i++){
                        buffer[i] = '\0';
                   }
					}
					 
               sub_buffer = &buffer[offset];
            }
            else{
                recv(client_socket, sub_buffer, (pos_delim+1), 0);
                n_bytes_data = offset + pos_delim + 1;
                finished_recv = true;
            }
        }
        else{
            finished_recv = true;
            n_bytes_data = n_bytes_chunk;
        }
   }
	 
   ProtocolMessage* pm;
	 
   if(n_bytes_data>0){
		buffer[n_bytes_data-1] = '\0';
		string str_recv = string(buffer);
		str_recv = this->encryption_engine.decrypt(str_recv);
		
		pm = ProtocolMessage::decode(str_recv);
		
		if(((ProtocolMessage::getMessageTypeOf(pm))==(ProtocolMessage::MessageType::undefined))){
			delete pm;
			pm = nullptr;
		}
   }
   else{
		pm = nullptr;
        //Client closed connection
   }

   free(buffer);
    
   return pm;
}

void Connection::clearDelims(string& orig){
   orig.erase(std::remove(orig.begin(), orig.end(), '\n'), orig.end());
   orig.erase(std::remove(orig.begin(), orig.end(), '\r'), orig.end());
}


string Connection::generateRandomString(int length){
    char* str_c = (char*)malloc(length*sizeof(char)+1);
    string str;
    int rand_int;
    char rand_char;

    for(int i=0; i<length; i++){
        do{
            rand_int = (std::rand()%256);
            rand_char = (char)rand_int;
        }while((rand_char=='\0')||(rand_char==SymmetricEngine::PAD_TOKEN));
        
        str_c[i] = (char)rand_int;
    }

    str_c[length] = '\0';

    str = string(str_c);
    free(str_c);
    return str;
}


unsigned int Connection::generateSeed(){
    auto time = std::chrono::system_clock::now();
    auto since_epoch = time.time_since_epoch();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(since_epoch);
    long long now = micros.count();
    long long one_second = 1000000;
    long long now_useconds = (now%one_second);
    unsigned int usec = (unsigned int)(now_useconds);
    unsigned int pid;
    unsigned int seed;

    pid = (unsigned int)getpid();
	 
    usec = usec & 0x0FFFFF;
    pid = pid & 0x0FFF;

    seed = (pid << 20) | (usec);

    return seed;
}

void Connection::initEngine(){
    unsigned int seed;
    string c_key_token;
    string c_iv_token;
    string s_key_token;
    string s_iv_token;
    string c_key_hi;
    string c_key_lo;
    string s_key_hi;
    string s_key_lo;
    string c_iv_hi;
    string c_iv_lo;
    string s_iv_hi;
    string s_iv_lo;
	 string s_token;
	 string server_exchange;
    int key_length = SymmetricEngine::KEY_LENGTH;
    int half = key_length/2;
    string client_exchange;
    const char* buffer_send;
    int send_len;
    int client_socket = this->getSocket();
    int n_bytes_recv;
    int n_bytes_recv_tot;
    char buffer_recv[Connection::CLIENT_EXCHANGE_SIZE];
    char* sub_buffer;
    int sub_len;
    string payload;
    string key_send;
    string key_recv;
    string iv_send;
    string iv_recv;
	 
	 //1. Receive and decrypt client tokens 
	n_bytes_recv = 0;
   n_bytes_recv_tot = 0;
   bool conn_error;
   bool finished_exchange;

   do{
        sub_buffer = &buffer_recv[n_bytes_recv_tot];
        sub_len = Connection::CLIENT_EXCHANGE_SIZE - n_bytes_recv_tot;
        n_bytes_recv = recv(client_socket,sub_buffer,sub_len,0);
        n_bytes_recv_tot += n_bytes_recv;

        conn_error = (n_bytes_recv<=0);
        finished_exchange = (n_bytes_recv_tot == Connection::CLIENT_EXCHANGE_SIZE);

   }while((!finished_exchange)&&(!conn_error));
	
	if(!conn_error){
		client_exchange = string(buffer_recv,(n_bytes_recv_tot-1));
		client_exchange = PrivateEngine::decrypt(client_exchange);
		int expected_length = 2*SymmetricEngine::KEY_LENGTH;
		int actual_length = client_exchange.length();
		
		if(actual_length==expected_length){
			//2. Get client tokens
			c_key_token = client_exchange.substr(0,key_length);
			c_iv_token = client_exchange.substr(key_length,key_length);

			c_key_hi = c_key_token.substr(0,half);
			c_key_lo = c_key_token.substr(half,half);
			c_iv_hi = c_iv_token.substr(0,half);
			c_iv_lo = c_iv_token.substr(half,half);

			//3. Generate random tokens and send them to the client
			seed = Connection::generateSeed();
			srand(seed);

			s_key_token = Connection::generateRandomString(key_length);
			s_iv_token = Connection::generateRandomString(key_length);

			s_token = s_key_token + s_iv_token;

			this->encryption_engine.init(c_key_token,c_iv_token,c_key_token,c_iv_token);

			payload = encryption_engine.encrypt(s_token);

			server_exchange = payload + Connection::SIGNATURE_DELIM +
									PrivateEngine::sign(payload) + Connection::END_DELIM;

			buffer_send = server_exchange.c_str();
			send_len = server_exchange.length();

			send(client_socket, buffer_send, send_len, 0);

			// 4. Generate Keys
			s_key_hi = s_key_token.substr(0,half);
			s_key_lo = s_key_token.substr(half,half);
			s_iv_hi = s_iv_token.substr(0,half);
			s_iv_lo = s_iv_token.substr(half,half);

			key_send = s_key_hi + c_key_lo;
			key_recv = c_key_hi + s_key_lo;

			iv_send = s_iv_hi + c_iv_lo;
			iv_recv = c_iv_hi + s_iv_lo;

			this->encryption_engine.init(key_send,iv_send,key_recv,iv_recv);
			this->conn_error = false;
		}
		else{
			this->conn_error = true;
		}
	}
	else{
		this->conn_error = true;
	}
}

bool Connection::connError(){
	return this->conn_error;
}