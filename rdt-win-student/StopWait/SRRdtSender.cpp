#include "stdafx.h"
#include "Global.h"
#include "SRRdtSender.h"

SRRdtSender::SRRdtSender() :size(4), base(0), expectSequenceNumberSend(0), waitingState(false)
{
}

SRRdtSender::~SRRdtSender()
{
}

bool SRRdtSender::getWaitingState() {
	if (this->swindows_packets.size() == 4)
	{
		this->waitingState = true;
	}
	else
	{
		this->waitingState = false;
	}
	return waitingState;
}

bool SRRdtSender::send(Message &message) {
	if (this->getWaitingState()) { //发送方处于等待确认状态
		return false;
	}

	this->packetWaitingAck.acknum = -1; //忽略该字段
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);
	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);

	pns->startTimer(SENDER, Configuration::TIME_OUT, this->packetWaitingAck.seqnum);			//启动发送方定时器

	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	this->swindows_packets.push_back(this->packetWaitingAck);
	//this->expectSequenceNumberSend++;
	this->expectSequenceNumberSend = ((this->expectSequenceNumberSend + 1) % 8);
	//this->waitingState = true;
	return true;
}

void SRRdtSender::receive(Packet &ackPkt) {
	int checkSum = pUtils->calculateCheckSum(ackPkt);
	//如果校验和正确
	if (checkSum == ackPkt.checksum)
	{
		//int flag = 0;
		pUtils->printPacket("发送方收到对应报文的确认：", ackPkt);
		pns->stopTimer(SENDER,ackPkt.acknum);
		list<Packet>::iterator it;                                          //定义数据便于windows_packets中存储的报文
		for (it = this->swindows_packets.begin(); it != this->swindows_packets.end(); it++)
		{
			if (it->seqnum == ackPkt.acknum)
			{
				this->swindows_packets.erase(it);
				break;
			}
		}
		
		cout << "此时发送方窗口的base值为：" << this->base << endl << "发送方缓存区中报文有：" << endl;
		for (it = this->swindows_packets.begin(); it != this->swindows_packets.end(); it++)
		{
			pUtils->printPacket("    ", *it);
		}

		//滑动窗口
		if (!this->swindows_packets.size())
		{
			this->base = this->expectSequenceNumberSend;
		}
		else
		{
			this->base = this->swindows_packets.front().seqnum;
		}
	}
}

void SRRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	pns->stopTimer(SENDER, seqNum);										//首先关闭定时器
	pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);			//重新启动发送方定时器
	list<Packet>::iterator it;                                          //定义数据便于输出windows_packets中存储的报文
	for (it = this->swindows_packets.begin(); it != this->swindows_packets.end(); it++)
	{
		if (it->seqnum == seqNum)
		{
			pUtils->printPacket("发送方定时器时间到，重新发送此报文", *it);
			pns->sendToNetworkLayer(RECEIVER, *it);			//重新发送数据包
			break;
		}
	}
}