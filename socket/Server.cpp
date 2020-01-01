#pragma once
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "Server.h"
#include "WinsockEnv.h"
#include "Config.h"
#include <winsock2.h>
#include <algorithm>
#include "ChatRoomClientGUI.h"
#include "ChatRoomClientGUIDlg.h"

#pragma comment(lib, "Ws2_32.lib")

#pragma warning(disable:4996)


Server::Server(string ip, int port, string addr)
{
	this->ip = ip;
	this->port = port;
	this->addr = addr;
	this->close = 0;

	this->recvBuf = new char[Config::BUFFERLENGTH]; //��ʼ�����ܻ�����
	memset(this->recvBuf, '\0', Config::BUFFERLENGTH);

	this->rcvedMessages = new list<string>();
	this->sessions = new list<SOCKET>();
	this->closedSessions = new list<SOCKET>();
	this->clientAddrMaps = new map<SOCKET, string>();
}

Server::~Server(void)
{
	//�ͷŽ��ܻ�����
	if (this->recvBuf != NULL) {
		delete this->recvBuf;
		this->recvBuf = NULL;
	}


	//�ر�server socket
	if (this->srvSocket != NULL) {
		closesocket(this->srvSocket);
		this->srvSocket = NULL;
	}

	WSACleanup(); //����winsock ���л���
}

//��ʼ��Winsock
int Server::WinsockStartup() {
	if (WinsockEnv::Startup() == -1) return -1;	//��ʼ��Winsock
	return 0;
}

//��ʼ��Server����������sockect���󶨵�IP��PORT
int Server::ServerStartup() {
	//���� TCP socket
	this->srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->srvSocket == INVALID_SOCKET) {
		cout << "Server socket creare error !\n";
		WSACleanup();
		return -1;
	}
	cout << "Server socket create ok!\n";
	//���÷�����IP��ַ�Ͷ˿ں�
	this->srvAddr.sin_family = AF_INET;
	this->srvAddr.sin_port = htons(this->port);//���ö˿�
	this->srvAddr.sin_addr.S_un.S_addr = inet_addr(this->ip.c_str()); //����IP

	int rtn = bind(this->srvSocket, (LPSOCKADDR)&(this->srvAddr), sizeof(this->srvAddr));
	if (rtn == SOCKET_ERROR) {
		cout << "Server socket bind error!\n";
		closesocket(this->srvSocket);
		WSACleanup();
		return -1;
	}

	cout << "Server socket bind ok!\n";
	return 0;
}

//��ʼ����,�ȴ��ͻ�����������
int Server::ListenStartup() {
	int rtn = listen(this->srvSocket, Config::MAXCONNECTION);
	if (rtn == SOCKET_ERROR) {
		cout << "Server socket listen error!\n";
		closesocket(this->srvSocket);
		WSACleanup();
		return -1;
	}

	cout << "Server socket listen ok!\n";
	return 0;
}

//���յ��Ŀͻ�����Ϣ���浽��Ϣ����
void Server::AddRecvMessage(string str) {
	if (!str.empty())
		this->rcvedMessages->insert(this->rcvedMessages->end(), str);

}

//���µĻỰSOCKET�������
void Server::AddSession(SOCKET session) {
	if (session != INVALID_SOCKET) {
		this->sessions->insert(this->sessions->end(), session);
	}
}

//��ʧЧ�ĻỰSOCKET�������
void Server::AddClosedSession(SOCKET session) {
	if (session != INVALID_SOCKET) {
		this->closedSessions->insert(this->closedSessions->end(), session);
	}
}

//��ʧЧ��SOCKET�ӻỰSOCKET����ɾ��
void Server::RemoveClosedSession(SOCKET closedSession) {
	if (closedSession != INVALID_SOCKET) {
		list<SOCKET>::iterator itor = find(this->sessions->begin(), this->sessions->end(), closedSession);
		if (itor != this->sessions->end())
		{
			this->sessions->erase(itor);
			closesocket(closedSession);
		}
	}
}

//��ʧЧ��SOCKET�ӻỰSOCKET����ɾ��
void Server::RemoveClosedSession() {
	for (list<SOCKET>::iterator itor = this->closedSessions->begin(); itor != this->closedSessions->end(); itor++) {
		this->RemoveClosedSession(*itor);
	}
}

//��SOCKET s������Ϣ
void Server::recvMessage(SOCKET socket) {
	int receivedBytes = recv(socket, this->recvBuf, Config::BUFFERLENGTH, 0);
	if (receivedBytes == SOCKET_ERROR || receivedBytes == 0) {//�������ݴ��󣬰Ѳ�������ĻỰsocekt����sessionsClosed����
		this->AddClosedSession(socket);
		//string s("����" + this->GetClientAddress(this->clientAddrMaps,socket) + "���ο��뿪��������,�����������������(��)�ı�Ӱ...\n");
		//this->AddRecvMessage(s);
		//cout << s;
	}
	else {
		recvBuf[receivedBytes] = '\0';
		string s("����" + this->GetClientAddress(this->clientAddrMaps, socket) + "���ο�˵:" + recvBuf + "\n");
		this->AddRecvMessage(s); //���յ�����Ϣ���뵽��Ϣ����
		cout << s;
		memset(this->recvBuf, '\0', Config::BUFFERLENGTH);//������ܻ�����
	}
}

//��SOCKET s������Ϣ
void Server::sendMessage(SOCKET socket, string msg) {
	int rtn = send(socket, msg.c_str(), msg.length(), 0);
	if (rtn == SOCKET_ERROR) {//�������ݴ��󣬰Ѳ�������ĻỰsocekt����sessionsClosed����
		string s("����" + this->GetClientAddress(this->clientAddrMaps, socket) + "���ο��뿪��������,�����������������(��)�ı�Ӱ...\n");
		this->AddRecvMessage(s);
		this->AddClosedSession(socket);
		cout << s;
	}
}

//�������ͻ�ת����Ϣ
void Server::ForwardMessage() {
	if (this->numOfSocketSignaled > 0) {
		if (!this->rcvedMessages->empty()) {//�����Ϣ���в�Ϊ��
			for (list<string>::iterator msgItor = this->rcvedMessages->begin(); msgItor != this->rcvedMessages->end(); msgItor++) {//����Ϣ�����е�ÿһ����Ϣ
				for (list<SOCKET>::iterator sockItor = this->sessions->begin(); sockItor != this->sessions->end(); sockItor++) {//�ԻỰsocket�����е�ÿ��socket

				}
			}
		}
		this->rcvedMessages->clear(); //�������ͻ�ת����Ϣ�������Ϣ����
	}
}


int Server::AcceptRequestionFromClient() {
	//���srvSocket�Ƿ��յ��û���������
	if (this->numOfSocketSignaled > 0) {
		if (FD_ISSET(this->srvSocket, &rfds)) {  //�пͻ�����������
			this->numOfSocketSignaled--;
			HANDLE thread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mythread, (LPVOID)(this), 0, 0);
			CloseHandle(thread1);
		}
	}
	return 0;
}



void Server::ReceieveMessageFromClients() {
	if (this->numOfSocketSignaled > 0) {
		//�����Ự�б��е�����socket������Ƿ������ݵ���
		for (list<SOCKET>::iterator itor = this->sessions->begin(); itor != this->sessions->end(); itor++) {
			if (FD_ISSET(*itor, &rfds)) {  //ĳ�Ựsocket�����ݵ���
										   //��������
				this->recvMessage(*itor);
			}
		}//end for
	}
}

//�õ��ͻ���IP��ַ
string  Server::GetClientAddress(SOCKET s) {
	string clientAddress; //clientAddress�Ǹ����ַ����� clientAddress.empty() is true
	sockaddr_in clientAddr;
	int nameLen, rtn;

	nameLen = sizeof(clientAddr);
	rtn = getsockname(s, (LPSOCKADDR)&clientAddr, &nameLen);
	if (rtn != SOCKET_ERROR) {
		clientAddress += inet_ntoa(clientAddr.sin_addr);
		clientAddress += ':';
		clientAddress += to_string(clientAddr.sin_port);
	}

	return clientAddress;
}

//�õ��ͻ���IP��ַ
string  Server::GetClientAddress(map<SOCKET, string> *maps, SOCKET s) {
	map<SOCKET, string>::iterator itor = maps->find(s);
	if (itor != maps->end())
		return (*itor).second;
	else {
		return string("");
	}

}

//���ܿͻ��˷�������������ݲ�ת��
int Server::Loop() {
	if (this->close)
		return 1;
	u_long blockMode = Config::BLOCKMODE;//��srvSock��Ϊ������ģʽ�Լ����ͻ���������
	int rtn;

	if ((rtn = ioctlsocket(this->srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO��������ֹ�׽ӿ�s�ķ�����ģʽ��
		cout << "ioctlsocket() failed with error!\n";
		return -1;
	}
	cout << "ioctlsocket() for server socket ok!Waiting for client connection and data\n";

	while (true) {
		//Prepare the read and write socket sets for network I/O notification.
		FD_ZERO(&this->rfds);

		//��srvSocket���뵽rfds���ȴ��û���������
		FD_SET(this->srvSocket, &this->rfds);


		//�ȴ��û�����������û����ݵ�����Ựsocke�ɷ�������
		if ((this->numOfSocketSignaled = select(0, &this->rfds, NULL, NULL, NULL)) == SOCKET_ERROR) { //select���������пɶ����д��socket��������������rtn��.���һ�������趨�ȴ�ʱ�䣬��ΪNULL��Ϊ����ģʽ
			cout << "select() failed with error!\n";
			return -1;
		}
		if (this->AcceptRequestionFromClient() != 0) return -1;
	}
	return 0;
}


void mythread(Server *server) {
	if (!server) return;
	sockaddr_in clientAddr;		//�ͻ���IP��ַ
	int nAddrLen = sizeof(clientAddr);
	u_long blockMode = Config::BLOCKMODE;//��session socket��Ϊ������ģʽ�Լ����ͻ���������
										 //�����Ựsocket
	SOCKET newSession = accept(server->srvSocket, (LPSOCKADDR)&(clientAddr), &nAddrLen);
	string result = string("�ͻ���IP:") + string(inet_ntoa(clientAddr.sin_addr)) + "   ,�ͻ���PortΪ��" + to_string(ntohs(clientAddr.sin_port));
	if (newSession == INVALID_SOCKET) {
		return;
	}
	string mrecv;
	char mbuf[1000];
	while (1) {
		int receivedBytes = recv(newSession, mbuf, Config::BUFFERLENGTH, 0);
		//�������ݴ��󣬰Ѳ�������ĻỰsocekt�����ɾ��
		if (receivedBytes == SOCKET_ERROR || receivedBytes == 0) {
			closesocket(newSession);
			break;
		}
		else {
			mbuf[receivedBytes] = '\0';
			mrecv = mbuf;
			if (mrecv.find("Accept-Language") == string::npos && mrecv.find("Host") != string::npos && mrecv.find("favicon.ico") == string::npos)
			{
				//string se = server->mySendString(mrecv);
				int x = mrecv.find("GET /") + 5;
				int y = mrecv.find(" HTTP") - 5;
				string str0 = mrecv.substr(x, y);
				string str1 = mrecv.substr(0, y + 10);
				string out = "����������:" + str1;
				string data;
				string state;
				fstream in((server->addr + str0), ios::binary | ios::in);
				if (in.is_open()) {
					ostringstream buffer;
					buffer << in.rdbuf();
					data = buffer.str();
					in.close();
					//����״̬��
					state = "HTTP/1.1 200 OK\r\nContent-Type: ";
					if (str0.find(".jpg") != std::string::npos || str0.find(".gif") != std::string::npos) {
						state = state + "image/gif\r\n\r\n";
					}
					else if (str0.find(".txt") != std::string::npos) {
						state = state + "text/html\r\n\r\n";
					}
					else if (str0.find(".pdf") != std::string::npos) {
						state = state + "application/pdf\r\n\r\n";
					}
					data = state + data;
				}
				else {
					fstream error_in("D:\\error.txt");
					ostringstream buffer;
					buffer << error_in.rdbuf();
					data = buffer.str();
					data = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + data;
					error_in.close();
				}
				CChatRoomClientGUIDlg *dlg = (CChatRoomClientGUIDlg *)(AfxGetApp()->GetMainWnd());
				dlg->m_List.AddString(CString(result.c_str()));
				dlg->m_List.AddString(CString(out.c_str()));
				int rtn = send(newSession, data.c_str(), data.length(), 0);
				dlg->m_List.AddString(CString(string("���ͳɹ���").c_str()));
				dlg->m_List.AddString(CString(string("").c_str()));
			}
			else
			{
				closesocket(newSession);
				break;
			}
		}
	}
}

string Server::mySendString(string msg) {
	int x = msg.find("GET /") + 5;
	int y = msg.find(" HTTP") - 5;
	string str_0 = msg.substr(x, y);
	string str_1 = msg.substr(0, y + 10);
	cout << "HTTP����������:" << str_1 << endl;
	string data;
	string state;
	fstream in(("D:\\" + str_0), ios::binary | ios::in);
	if (in.is_open()) {
		ostringstream buffer;
		buffer << in.rdbuf();
		data = buffer.str();
		in.close();
		//��������״̬��
		state = "HTTP/1.1 200 OK\r\nContent-Type: ";
		if (str_0.find(".jpg") != std::string::npos || str_0.find(".gif") != std::string::npos) {
			state = state + "image/gif\r\n\r\n";
		}
		else if (str_0.find(".txt") != std::string::npos) {
			state = state + "text/html\r\n\r\n";
		}
		else if (str_0.find(".pdf") != std::string::npos) {
			state = state + "application/pdf\r\n\r\n";
		}
		data = state + data;
	}
	else {
		fstream error_in("D:\\error.txt");
		ostringstream buffer;
		buffer << error_in.rdbuf();
		data = buffer.str();
		data = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n" + data;
		error_in.close();
	}

	return data;
}
