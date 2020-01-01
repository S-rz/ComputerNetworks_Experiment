// StopWait.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "Global.h"
#include "RdtSender.h"
#include "RdtReceiver.h"
#include "StopWaitRdtSender.h"
#include "StopWaitRdtReceiver.h"
#include "GBNSender.h"
#include "GBNReceiver.h"
#include "SRRdtSender.h"
#include "SRRdtReceiver.h"
#include "TCPSender.h"
#include "TCPReceiver.h"


int main(int argc, char** argv[])
{
	//RdtSender *ps = new StopWaitRdtSender();
	//RdtReceiver * pr = new StopWaitRdtReceiver();
	//RdtSender *ps = new GBNSender();
	//RdtReceiver * pr = new GBNReceiver();
	//RdtSender *ps = new SRRdtSender();
	//RdtReceiver * pr = new SRRdtReceiver();
	RdtSender *ps = new TCPSender();
	RdtReceiver * pr = new TCPReceiver();
	pns->init();
	pns->setRtdSender(ps);
	pns->setRtdReceiver(pr);
	pns->setInputFile("D:\\CSIE1601\\My\\network\\shiyan\\rdt-win-student\\input.txt");
	pns->setOutputFile("D:\\CSIE1601\\My\\network\\shiyan\\rdt-win-student\\output.txt");
	pns->start();

	delete ps;
	delete pr;
	delete pUtils;									//ָ��Ψһ�Ĺ�����ʵ����ֻ��main��������ǰdelete
	delete pns;										//ָ��Ψһ��ģ�����绷����ʵ����ֻ��main��������ǰdelete
	
	return 0;
}

