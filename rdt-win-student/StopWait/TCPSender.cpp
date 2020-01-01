#include "stdafx.h"
#include "Global.h"
#include "TCPSender.h"


TCPSender::TCPSender() :size(3), base(0), count(0), previousAck(0), expectSequenceNumberSend(0), waitingState(false)
{
}


TCPSender::~TCPSender()
{
}

bool TCPSender::getWaitingState() {
	if (this->windows_packets.size() == 3)
	{
		this->waitingState = true;
	}
	else
	{
		this->waitingState = false;
	}
	return waitingState;
}

bool TCPSender::send(Message &message) {
	if (this->getWaitingState()) { //发送方处于等待确认状态
		return false;
	}

	this->packetWaitingAck.acknum = -1; //忽略该字段
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);
	if (this->base == this->expectSequenceNumberSend)
	{
		pns->startTimer(SENDER, Configuration::TIME_OUT, 0);			//启动发送方定时器
	}

	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	this->windows_packets.push_back(this->packetWaitingAck);
	//this->expectSequenceNumberSend++;
	this->expectSequenceNumberSend = ((this->expectSequenceNumberSend + 1) % 4);
	//this->waitingState = true;
	return true;
}

void TCPSender::receive(Packet &ackPkt) {
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//如果校验和正确
	if (checkSum == ackPkt.checksum)
	{
		if (this->previousAck == ackPkt.acknum)
		{
			this->count++;
			if (this->count == 3) //快速重传
			{
				this->count = 0;
				//遍历缓存区进行快速重传
				list<Packet>::iterator it;                                          //定义数据便于输出windows_packets中存储的报文
				for (it = this->windows_packets.begin(); it != this->windows_packets.end(); it++)
				{
					if (it->seqnum == (ackPkt.acknum+1)%4 )
					{
						pUtils->printPacket("对于该报文进行快速重传：", *it);
						pns->sendToNetworkLayer(RECEIVER, *it);			//重新发送数据包
					}
				}
				return;
			}
		}
		else
		{
			this->previousAck = ackPkt.acknum;
			this->count = 0;
		}
		//int flag = 0;
		while (this->base != ((ackPkt.acknum + 1) % 4))
		{
			this->windows_packets.pop_front(); //滑动窗口
			this->base = (this->base + 1) % 4;
		}
		this->base = ((ackPkt.acknum + 1) % 4);
		list<Packet>::iterator it;                                          //定义数据便于输出windows_packets中存储的报文
		cout << "此时窗口的base值为：" << this->base << endl << "缓存区中报文有：" << endl;
		for (it = this->windows_packets.begin(); it != this->windows_packets.end(); it++)
		{
			pUtils->printPacket("    ", *it);
		}
		if (this->base == this->expectSequenceNumberSend)
		{
			pUtils->printPacket("发送方正确收到确认", ackPkt);
			pns->stopTimer(SENDER, 0);		//关闭定时器
		}
		else
		{
			pns->stopTimer(SENDER, 0);									//首先关闭定时器
			pns->startTimer(SENDER, Configuration::TIME_OUT, 0);			//重新启动发送方定时器
		}
	}
}

void TCPSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	pUtils->printPacket("发送方定时器时间到，重新发送此报文之后的报文", this->packetWaitingAck);
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	list<Packet>::iterator it;                                          //定义数据便于输出windows_packets中存储的报文
	for (it = this->windows_packets.begin(); it != this->windows_packets.end(); it++)
	{
		pns->sendToNetworkLayer(RECEIVER, *it);			//重新发送数据包
	}
}
