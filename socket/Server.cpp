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

	this->recvBuf = new char[Config::BUFFERLENGTH]; //初始化接受缓冲区
	memset(this->recvBuf, '\0', Config::BUFFERLENGTH);

	this->rcvedMessages = new list<string>();
	this->sessions = new list<SOCKET>();
	this->closedSessions = new list<SOCKET>();
	this->clientAddrMaps = new map<SOCKET, string>();
}

Server::~Server(void)
{
	//释放接受缓冲区
	if (this->recvBuf != NULL) {
		delete this->recvBuf;
		this->recvBuf = NULL;
	}


	//关闭server socket
	if (this->srvSocket != NULL) {
		closesocket(this->srvSocket);
		this->srvSocket = NULL;
	}

	WSACleanup(); //清理winsock 运行环境
}

//初始化Winsock
int Server::WinsockStartup() {
	if (WinsockEnv::Startup() == -1) return -1;	//初始化Winsock
	return 0;
}

//初始化Server，包括创建sockect，绑定到IP和PORT
int Server::ServerStartup() {
	//创建 TCP socket
	this->srvSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (this->srvSocket == INVALID_SOCKET) {
		cout << "Server socket creare error !\n";
		WSACleanup();
		return -1;
	}
	cout << "Server socket create ok!\n";
	//设置服务器IP地址和端口号
	this->srvAddr.sin_family = AF_INET;
	this->srvAddr.sin_port = htons(this->port);//设置端口
	this->srvAddr.sin_addr.S_un.S_addr = inet_addr(this->ip.c_str()); //设置IP

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

//开始监听,等待客户的连接请求
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

//将收到的客户端消息保存到消息队列
void Server::AddRecvMessage(string str) {
	if (!str.empty())
		this->rcvedMessages->insert(this->rcvedMessages->end(), str);

}

//将新的会话SOCKET加入队列
void Server::AddSession(SOCKET session) {
	if (session != INVALID_SOCKET) {
		this->sessions->insert(this->sessions->end(), session);
	}
}

//将失效的会话SOCKET加入队列
void Server::AddClosedSession(SOCKET session) {
	if (session != INVALID_SOCKET) {
		this->closedSessions->insert(this->closedSessions->end(), session);
	}
}

//将失效的SOCKET从会话SOCKET队列删除
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

//将失效的SOCKET从会话SOCKET队列删除
void Server::RemoveClosedSession() {
	for (list<SOCKET>::iterator itor = this->closedSessions->begin(); itor != this->closedSessions->end(); itor++) {
		this->RemoveClosedSession(*itor);
	}
}

//从SOCKET s接受消息
void Server::recvMessage(SOCKET socket) {
	int receivedBytes = recv(socket, this->recvBuf, Config::BUFFERLENGTH, 0);
	if (receivedBytes == SOCKET_ERROR || receivedBytes == 0) {//接受数据错误，把产生错误的会话socekt加入sessionsClosed队列
		this->AddClosedSession(socket);
		//string s("来自" + this->GetClientAddress(this->clientAddrMaps,socket) + "的游客离开了聊天室,我们深深地凝望着他(她)的背影...\n");
		//this->AddRecvMessage(s);
		//cout << s;
	}
	else {
		recvBuf[receivedBytes] = '\0';
		string s("来自" + this->GetClientAddress(this->clientAddrMaps, socket) + "的游客说:" + recvBuf + "\n");
		this->AddRecvMessage(s); //将收到的消息加入到消息队列
		cout << s;
		memset(this->recvBuf, '\0', Config::BUFFERLENGTH);//清除接受缓冲区
	}
}

//向SOCKET s发送消息
void Server::sendMessage(SOCKET socket, string msg) {
	int rtn = send(socket, msg.c_str(), msg.length(), 0);
	if (rtn == SOCKET_ERROR) {//发送数据错误，把产生错误的会话socekt加入sessionsClosed队列
		string s("来自" + this->GetClientAddress(this->clientAddrMaps, socket) + "的游客离开了聊天室,我们深深地凝望着他(她)的背影...\n");
		this->AddRecvMessage(s);
		this->AddClosedSession(socket);
		cout << s;
	}
}

//向其他客户转发信息
void Server::ForwardMessage() {
	if (this->numOfSocketSignaled > 0) {
		if (!this->rcvedMessages->empty()) {//如果消息队列不为空
			for (list<string>::iterator msgItor = this->rcvedMessages->begin(); msgItor != this->rcvedMessages->end(); msgItor++) {//对消息队列中的每一条消息
				for (list<SOCKET>::iterator sockItor = this->sessions->begin(); sockItor != this->sessions->end(); sockItor++) {//对会话socket队列中的每个socket

				}
			}
		}
		this->rcvedMessages->clear(); //向其他客户转发消息后，清除消息队列
	}
}


int Server::AcceptRequestionFromClient() {
	//检查srvSocket是否收到用户连接请求
	if (this->numOfSocketSignaled > 0) {
		if (FD_ISSET(this->srvSocket, &rfds)) {  //有客户连接请求到来
			this->numOfSocketSignaled--;
			HANDLE thread1 = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mythread, (LPVOID)(this), 0, 0);
			CloseHandle(thread1);
		}
	}
	return 0;
}



void Server::ReceieveMessageFromClients() {
	if (this->numOfSocketSignaled > 0) {
		//遍历会话列表中的所有socket，检查是否有数据到来
		for (list<SOCKET>::iterator itor = this->sessions->begin(); itor != this->sessions->end(); itor++) {
			if (FD_ISSET(*itor, &rfds)) {  //某会话socket有数据到来
										   //接受数据
				this->recvMessage(*itor);
			}
		}//end for
	}
}

//得到客户端IP地址
string  Server::GetClientAddress(SOCKET s) {
	string clientAddress; //clientAddress是个空字符串， clientAddress.empty() is true
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

//得到客户端IP地址
string  Server::GetClientAddress(map<SOCKET, string> *maps, SOCKET s) {
	map<SOCKET, string>::iterator itor = maps->find(s);
	if (itor != maps->end())
		return (*itor).second;
	else {
		return string("");
	}

}

//接受客户端发来的请求和数据并转发
int Server::Loop() {
	if (this->close)
		return 1;
	u_long blockMode = Config::BLOCKMODE;//将srvSock设为非阻塞模式以监听客户连接请求
	int rtn;

	if ((rtn = ioctlsocket(this->srvSocket, FIONBIO, &blockMode) == SOCKET_ERROR)) { //FIONBIO：允许或禁止套接口s的非阻塞模式。
		cout << "ioctlsocket() failed with error!\n";
		return -1;
	}
	cout << "ioctlsocket() for server socket ok!Waiting for client connection and data\n";

	while (true) {
		//Prepare the read and write socket sets for network I/O notification.
		FD_ZERO(&this->rfds);

		//把srvSocket加入到rfds，等待用户连接请求
		FD_SET(this->srvSocket, &this->rfds);


		//等待用户连接请求或用户数据到来或会话socke可发送数据
		if ((this->numOfSocketSignaled = select(0, &this->rfds, NULL, NULL, NULL)) == SOCKET_ERROR) { //select函数返回有可读或可写的socket的总数，保存在rtn里.最后一个参数设定等待时间，如为NULL则为阻塞模式
			cout << "select() failed with error!\n";
			return -1;
		}
		if (this->AcceptRequestionFromClient() != 0) return -1;
	}
	return 0;
}


void mythread(Server *server) {
	if (!server) return;
	sockaddr_in clientAddr;		//客户端IP地址
	int nAddrLen = sizeof(clientAddr);
	u_long blockMode = Config::BLOCKMODE;//将session socket设为非阻塞模式以监听客户连接请求
										 //产生会话socket
	SOCKET newSession = accept(server->srvSocket, (LPSOCKADDR)&(clientAddr), &nAddrLen);
	string result = string("客户端IP:") + string(inet_ntoa(clientAddr.sin_addr)) + "   ,客户端Port为：" + to_string(ntohs(clientAddr.sin_port));
	if (newSession == INVALID_SOCKET) {
		return;
	}
	string mrecv;
	char mbuf[1000];
	while (1) {
		int receivedBytes = recv(newSession, mbuf, Config::BUFFERLENGTH, 0);
		//接受数据错误，把产生错误的会话socekt则进行删除
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
				string out = "请求命令行:" + str1;
				string data;
				string state;
				fstream in((server->addr + str0), ios::binary | ios::in);
				if (in.is_open()) {
					ostringstream buffer;
					buffer << in.rdbuf();
					data = buffer.str();
					in.close();
					//构建状态行
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
				dlg->m_List.AddString(CString(string("发送成功！").c_str()));
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
	cout << "HTTP请求命令行:" << str_1 << endl;
	string data;
	string state;
	fstream in(("D:\\" + str_0), ios::binary | ios::in);
	if (in.is_open()) {
		ostringstream buffer;
		buffer << in.rdbuf();
		data = buffer.str();
		in.close();
		//构建报文状态行
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
