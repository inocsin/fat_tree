/*
 * processor.cc
 *
 *  Created on: 2016年7月30日
 *      Author: Vincent
 */


#include <stdio.h>
#include <string.h>
#include <omnetpp.h>
#include "fat_tree_m.h"
#include "fat_tree.h"
#include "buffer_info_m.h"

using namespace omnetpp;

/**
 * This model is exciting enough so that we can collect some statistics.
 * We'll record in output vectors the hop count of every message upon arrival.
 * Output vectors are written into the omnetpp.vec file and can be visualized
 * with the Plove program.
 *
 * We also collect basic statistics (min, max, mean, std.dev.) and histogram
 * about the hop count which we'll print out at the end of the simulation.
 */


// Processor
class Processor : public cSimpleModule
{
  private:
    cMessage *selfMsgGenMsg;
    cMessage *selfMsgBufferInfoP;//Processor的bufer更新定时信号，告诉与它相连的router，所有vc通道都为avail

    long numSent;
    long numReceived;
    cLongHistogram hopCountStats;
    cOutVector hopCountVector;

    bool BufferAvailCurrentP[VC];//当前Processor的buffer状态，默认都为available
    bool BufferAvailConnectP[VC];//相连Processor端口的Router的buffer状态

    int FlitCir; //用来循环产生Head Flit和Body Flit
    int preHeadFlitVCID; //用来存放上一个产生的Head Flit的VCID，body flit不保存vcid
    FatTreeMsg* OutBuffer[FlitLength]; //一个package的大小，用于存放flit，有可能产生body flit时，下个router的buffer已满，因此需要缓冲一下

  public:
    Processor();
    virtual ~Processor();
  protected:
    virtual FatTreeMsg *generateMessage(bool isHead, int count);
    virtual void forwardMessage(FatTreeMsg *msg);
    virtual void forwardBufferInfoMsgP(BufferInfoMsg *msg);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual int ppid2plid(int ppid);
    virtual int plid2ppid(int plid);
    virtual int getNextRouterPortP(); //计算与processor相连的router的端口


    // The finish() function is called by OMNeT++ at the end of the simulation:
    virtual void finish() override;
};

Define_Module(Processor);

Processor::Processor(){
    selfMsgGenMsg=nullptr;
    selfMsgBufferInfoP=nullptr;
}


Processor::~Processor(){
    cancelAndDelete(selfMsgGenMsg);
    cancelAndDelete(selfMsgBufferInfoP);
}

void Processor::initialize()
{
    // Initialize variables
    numSent = 0;
    numReceived = 0;
    //初始化行和列的参数
    WATCH(numSent);
    WATCH(numReceived);

    hopCountStats.setName("hopCountStats");
    hopCountStats.setRangeAutoUpper(0, 10, 1.5);
    hopCountVector.setName("HopCount");

    selfMsgGenMsg = new cMessage("selfMsgGenMsg");
    scheduleAt(Sim_Start_Time, selfMsgGenMsg);
    selfMsgBufferInfoP = new cMessage("selfMsgBufferInfoP");
    scheduleAt(Buffer_Info_Sim_Start, selfMsgBufferInfoP);

    for(int i=0;i<VC;i++){
        BufferAvailCurrentP[i]=true;
        BufferAvailConnectP[i]=false;
    }

    FlitCir = 0; //从0开始循环

    for(int i=0;i<FlitLength;i++)
        OutBuffer[i]=nullptr;


    // Module 0 sends the first message
    // 选定某个ppid的processor来产生数据
    /*
    if (getIndex()%2 == 0) {
        // Boot the process scheduling the initial message as a self-message.
        FatTreeMsg *msg = generateMessage();
        //延迟发送0.1s
        scheduleAt(1, msg);
    }
    */
}

void Processor::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        //********************发送新数据的自定时消息********************
        if(msg==selfMsgGenMsg){
            if(getIndex()==0){ //processor产生msg的模式,需要改进

                //**********************产生flit*********************************
                if(FlitCir == 0 && OutBuffer[0] == nullptr){ //要产生新的head flit，同时buffer又有空间来存储剩余的body flit

                    EV << "<<<<<<<<<<Processor: "<<getIndex()<<"("<<ppid2plid(getIndex())<<") is generating Head Flit>>>>>>>>>>\n";
                    FatTreeMsg *newmsg = generateMessage(true, FlitCir); //Head Flit
                    EV << newmsg << endl;
                    preHeadFlitVCID = newmsg->getVc_id();
                    OutBuffer[0] = newmsg;
                    FlitCir++;

                }else if(FlitCir != 0 ){
                    EV << "<<<<<<<<<<Processor: "<<getIndex()<<"("<<ppid2plid(getIndex())<<") is generating Body/Tail Flit>>>>>>>>>>\n";
                    FatTreeMsg *newmsg = generateMessage(false, FlitCir); // Body or Tail Flit
                    EV << newmsg << endl;
                    for(int i=0;i<FlitLength;i++){
                        if(OutBuffer[i] == nullptr){
                            OutBuffer[i] = newmsg;
                            break;
                        }
                    }
                    FlitCir = (FlitCir+1) % FlitLength;

                }

                //int vc_id=newmsg->getVc_id();//获取vcid
                     /*
                     bool nextRouterHasBuffer = false;
                     int nextVCID = -1;
                     for(int i=0;i<VC;i++){
                         if(BufferAvailConnectP[i]==true){
                             nextVCID = i;
                             nextRouterHasBuffer = true;
                             break;
                         }

                     }
                     if(nextRouterHasBuffer){//转发新的fatTreeMsg
                         newmsg->setVc_id(nextVCID);
                         forwardMessage(newmsg);
                         if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                             EV<<"Processor Generating and Forwarding New FatTreeMsg >> PROCESSOR: "<<getIndex()<<"("<<ppid2plid(getIndex())<<"), VCID: "
                                     <<nextVCID<<"\n";
                         }
                         //bubble("Starting new one!");
                     }else{//buffer已满，dropping msg
                         if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                             EV<<"Next Router's Buffer is Full, Dropping FatTreeMsg >> PROCESSOR: "<<getIndex()<<"("<<ppid2plid(getIndex())<<")\n";
                         }
                         delete newmsg;

                     }
                     */
                //****************************转发flit**************************
                if(OutBuffer[0] != nullptr && BufferAvailConnectP[preHeadFlitVCID] == true){ //下一个节点有buffer接受此flit
                    FatTreeMsg* current_forward_msg = OutBuffer[0];
                    forwardMessage(current_forward_msg);
                    numSent++;
                    for(int i=0;i<FlitLength-1;i++){
                        OutBuffer[i] = OutBuffer[i+1];
                    }
                    OutBuffer[FlitLength-1] = nullptr;

                }


                //simtime_t delay = par("delayTime");
                //simtime_t sendNext = par("sendNext");
                //scheduleAt(simTime()+0.1, msg);

                scheduleAt(simTime()+CLK_CYCLE,selfMsgGenMsg);




            }
        }else{
            //******************更新buffer信息定时**********************
            scheduleAt(simTime()+Buffer_Info_Update_Interval,selfMsgBufferInfoP);


            //产生bufferInfo信号并向相应端口发送信号

            int from_port=getNextRouterPortP();
            BufferInfoMsg *bufferInfoMsg = new BufferInfoMsg("bufferInfoMsg");//消息名字不要改
            bufferInfoMsg->setFrom_port(from_port);
            bufferInfoMsg->setBufferAvailArraySize(VC);//arraySize的大小为VC的数量
            //设置buffer信息
            for(int j=0;j<VC;j++){
                bool avail=BufferAvailCurrentP[j];
                bufferInfoMsg->setBufferAvail(j, avail);
            }
            //发送bufferInfoMsg
            forwardBufferInfoMsgP(bufferInfoMsg);


        }

    }else{
        //************************收到buffer更新消息******************
        if(strcmp("bufferInfoMsg", msg->getName()) == 0){
            //收到的消息为buffer状态消息，更新BufferAvailConnect[PortNum][VC]


            BufferInfoMsg *bufferInfoMsg = check_and_cast<BufferInfoMsg *>(msg);
            int from_port=bufferInfoMsg->getFrom_port();
            //更新BufferAvailConnect[PortNum][VC]
            for(int j=0;j<VC;j++){
                BufferAvailConnectP[j]=bufferInfoMsg->getBufferAvail(j);
            }
            if (Verbose >= VERBOSE_BUFFER_INFO_MESSAGES) {
                EV<<"Receiving bufferInfoMsg, Updating buffer state >> PROCESSOR: "<<getIndex()<<"("<<ppid2plid(getIndex())<<"), INPORT: "<<from_port<<
                    ", Received MSG: { "<<bufferInfoMsg<<" }\n";
            }
            delete bufferInfoMsg;


        }else{
        //***********************收到FatTreeMsg消息*******************
            FatTreeMsg *ftmsg = check_and_cast<FatTreeMsg *>(msg);
            //EV<<"Message goto "<<getRow()<<","<<getCol()<<"\n";
            //int src_ppid=ftmsg->getSrc_ppid();

            // Message arrived
            int current_ppid=getIndex();
            int hopcount = ftmsg->getHopCount();
            if (Verbose >= VERBOSE_DEBUG_MESSAGES) {
                EV << ">>>>>>>>>>Message {" << ftmsg << " } arrived after " << hopcount <<
                        " hops at node "<<current_ppid<<"("<<ppid2plid(current_ppid)<<")<<<<<<<<<<\n";
            }
            //bubble("ARRIVED!");

            // update statistics.
            numReceived++;
            hopCountVector.record(hopcount);
            hopCountStats.collect(hopcount);

            delete ftmsg;

            /*
            int dst_ppid=ftmsg->getDst_ppid();
            int current_ppid=getIndex();
            if (current_ppid==dst_ppid) {
                // Message arrived
                int hopcount = ftmsg->getHopCount();
                EV << "Message {" << ftmsg << " } arrived after " << hopcount <<
                        " hops at node "<<current_ppid<<"("<<ppid2plid(current_ppid)<<")\n";
                bubble("ARRIVED!");

                // update statistics.
                numReceived++;
                hopCountVector.record(hopcount);
                hopCountStats.collect(hopcount);

                delete ftmsg;

                // Generate another one.
                //EV << "Generating another message: ";
                //FatTreeMsg *newmsg = generateMessage();
                //EV << newmsg << endl;
                //forwardMessage(newmsg);
                //numSent++;
            }
            else {
                //*******************收到错误消息************************
                // We need to forward the message.
                //forwardMessage(ftmsg);
                EV << "Received the wrong message: {" << ftmsg << " }\n";
                delete ftmsg;

            }
            */

        }

    }
}

FatTreeMsg* Processor::generateMessage(bool isHead, int count)
{

    if(isHead){
        // Head Flit

        // Produce source and destination addresse
        int current_ppid=getIndex();
        int n = getVectorSize();//processor的数量
        //EV<<n<<"\n";
        int dst_ppid = intuniform(0, n-2); //均匀流量模型
        //EV<<dst_ppid<<"\n";
        if (dst_ppid >= getIndex())
            dst_ppid++;//保证不取到current_ppid

        int vc_id = intuniform(0,VC-1); //随机分配vc通道, 根据下一跳router的vc buffer情况来选择合适的vcid
        for(int i=0;i<VC;i++){
            if(BufferAvailConnectP[i] == true){
                vc_id = i;
                break;
            }
        }



        int current_plid=ppid2plid(current_ppid);
        int dst_plid=ppid2plid(dst_ppid);

        char msgname[200];//初始分配的空间太小导致数据被改变!!!!!!!
        sprintf(msgname, "From processor node %d(%d) to node %d(%d), Head Flit No.%d", current_ppid,current_plid,dst_ppid,dst_plid,count);

        // Create message object and set source and destination field.
        FatTreeMsg *msg = new FatTreeMsg(msgname);
        msg->setSrc_ppid(current_ppid);//设置发出的processor编号
        msg->setDst_ppid(dst_ppid);//设置接收的processor编号
        msg->setFrom_router_port(getNextRouterPortP());//设置收到该msg的Router端口
        msg->setVc_id(vc_id);//设置VC ID
        msg->setIsHead(isHead); //设置isHead flag
        msg->setFlitCount(FlitLength); // 设置Flit Count


        if (Verbose >= VERBOSE_DETAIL_DEBUG_MESSAGES) {
            EV<<"From Processor::generateMessage, flit count: "<<msg->getFlitCount()<<", FlitLength: "<<FlitLength<<"\n";
        }
        //EV<<current_ppid<<" "<<dst_ppid<<"\n";
        //EV<<msg->getSrc_ppid()<<" "<<msg->getDst_ppid()<<"\n";

        return msg;
    }else{
        char msgname[200];//初始分配的空间太小导致数据被改变!!!!!!!
        sprintf(msgname, "Body/Tail Flit");

        // Create message object and set source and destination field.
        FatTreeMsg *msg = new FatTreeMsg(msgname);
        msg->setSrc_ppid(-1);//设置发出的processor编号
        msg->setDst_ppid(-1);//设置接收的processor编号
        msg->setFrom_router_port(getNextRouterPortP());//设置收到该msg的Router端口
        msg->setVc_id(-1);//设置VC ID
        msg->setIsHead(isHead); //设置isHead flag

        return msg;

    }


}

//processor转发的路由算法,processor只有一个prot,直接转发出去即可
void Processor::forwardMessage(FatTreeMsg *msg)
{

    msg->setFrom_router_port(getNextRouterPortP());// 设置收到该信息路由器的端口号
    send(msg,"port$o");
    EV << "Forwarding message { " << msg << " } from processor to router, VCID = "<<preHeadFlitVCID<<"\n";

}
void Processor::forwardBufferInfoMsgP(BufferInfoMsg *msg){
    send(msg,"port$o");
    int cur_ppid=getIndex();//当前路由器的id
    int cur_plid=ppid2plid(cur_ppid);
    if (Verbose >= VERBOSE_BUFFER_INFO_MESSAGES) {
        EV << "Forwarding BufferInfoMsg { " << msg << " } from processor "<<cur_ppid<<"("<<cur_plid<<")\n";
    }
}


int Processor::getNextRouterPortP(){
    int current_ppid=getIndex();
    int plid=ppid2plid(current_ppid);
    int port=plid%10;
    return port; //返回和processor相连的router的端口
}


//从ppid计算plid
int Processor::ppid2plid(int ppid){
    int idtmp=ppid;
    int idfinal=0;
    int mul=1;
    for(int i=0;i<LevelNum-1;i++){
        idfinal=idfinal+idtmp%(PortNum/2)*mul;
        mul=mul*10;
        idtmp=(int)(idtmp/(PortNum/2));
    }
    idfinal=idfinal+idtmp*mul;
    return idfinal;
}
//从plid计算ppid
int Processor::plid2ppid(int plid){
    int tmp=plid;
    int mul=1;
    int IDtmp=0;
    for(int i=0;i<LevelNum;i++){
        IDtmp=IDtmp+mul*(tmp%10);
        mul=mul*(PortNum/2);
        tmp=tmp/10;
    }
    return IDtmp;
}
void Processor::finish()
{
    // This function is called by OMNeT++ at the end of the simulation.
    EV << "Sent:     " << numSent << endl;
    EV << "Received: " << numReceived << endl;
    EV << "Hop count, min:    " << hopCountStats.getMin() << endl;
    EV << "Hop count, max:    " << hopCountStats.getMax() << endl;
    EV << "Hop count, mean:   " << hopCountStats.getMean() << endl;
    EV << "Hop count, stddev: " << hopCountStats.getStddev() << endl;

    recordScalar("#sent", numSent);
    recordScalar("#received", numReceived);

    hopCountStats.recordAs("hop count");
}




