//
// Generated file, do not edit! Created by nedtool 5.0 from fat_tree_pkt.msg.
//

#ifndef __FAT_TREE_PKT_M_H
#define __FAT_TREE_PKT_M_H

#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0500
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



/**
 * Class generated from <tt>fat_tree_pkt.msg:15</tt> by nedtool.
 * <pre>
 * //
 * // This program is free software: you can redistribute it and/or modify
 * // it under the terms of the GNU Lesser General Public License as published by
 * // the Free Software Foundation, either version 3 of the License, or
 * // (at your option) any later version.
 * // 
 * // This program is distributed in the hope that it will be useful,
 * // but WITHOUT ANY WARRANTY; without even the implied warranty of
 * // MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * // GNU Lesser General Public License for more details.
 * // 
 * // You should have received a copy of the GNU Lesser General Public License
 * // along with this program.  If not, see http://www.gnu.org/licenses/.
 * //
 * packet FatTreePkt
 * {
 *     //以下为head flit具有的信息
 *     int flitCount;//如果为head flit，则该项记录包括head flit在内的flit个数
 *     int src_ppid; //源processor的physical id
 *     int dst_ppid; //目标processor的physical id
 * 
 *     //以下为每个flit都具有的信息
 *     bool isHead; //判断是否为head flit
 *     int vc_id; //virtual channel id
 * 
 *     int hopCount = 0;
 *     int from_router_port; //记录从当前路由器的哪一个端口收到该msg，由上一个路由器计算出
 * 
 *     long packageGenTime; //package的产生时间
 * 	//long flitGenTime; //flit的产生时间
 * 
 * }
 * </pre>
 */
class FatTreePkt : public ::omnetpp::cPacket
{
  protected:
    int flitCount;
    int src_ppid;
    int dst_ppid;
    bool isHead;
    int vc_id;
    int hopCount;
    int from_router_port;
    long packageGenTime;

  private:
    void copy(const FatTreePkt& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const FatTreePkt&);

  public:
    FatTreePkt(const char *name=nullptr, int kind=0);
    FatTreePkt(const FatTreePkt& other);
    virtual ~FatTreePkt();
    FatTreePkt& operator=(const FatTreePkt& other);
    virtual FatTreePkt *dup() const {return new FatTreePkt(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b);

    // field getter/setter methods
    virtual int getFlitCount() const;
    virtual void setFlitCount(int flitCount);
    virtual int getSrc_ppid() const;
    virtual void setSrc_ppid(int src_ppid);
    virtual int getDst_ppid() const;
    virtual void setDst_ppid(int dst_ppid);
    virtual bool getIsHead() const;
    virtual void setIsHead(bool isHead);
    virtual int getVc_id() const;
    virtual void setVc_id(int vc_id);
    virtual int getHopCount() const;
    virtual void setHopCount(int hopCount);
    virtual int getFrom_router_port() const;
    virtual void setFrom_router_port(int from_router_port);
    virtual long getPackageGenTime() const;
    virtual void setPackageGenTime(long packageGenTime);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const FatTreePkt& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, FatTreePkt& obj) {obj.parsimUnpack(b);}


#endif // ifndef __FAT_TREE_PKT_M_H

