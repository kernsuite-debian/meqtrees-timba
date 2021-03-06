//
//% $Id$ 
//
//
// Copyright (C) 2002-2007
// The MeqTree Foundation & 
// ASTRON (Netherlands Foundation for Research in Astronomy)
// P.O.Box 2, 7990 AA Dwingeloo, The Netherlands
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>,
// or write to the Free Software Foundation, Inc., 
// 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <DMI/Record.h>
#include <DMI/Vec.h>
#include <DMI/ContainerIter.h>

#include "EchoWP.h"
    
#include <sys/time.h>
    
namespace Octopussy 
{
  
using namespace DMI;

const HIID MsgPing("Ping"),MsgPong("Pong"),MsgHelloEchoWP(MsgHello|"EchoWP.*");

EchoWP::EchoWP (int pingcount)
    : WorkProcess(AidEchoWP),pcount(pingcount)
{
  blocksize = 64;
  pipeline = 1;
  process = 1;
  threads = 0;
  fill = 1000;
  msgcount = bytecount = 0;
  timecount = 0;
  ts = Timestamp::now();
#ifdef ENABLE_LATENCY_STATS
  nping = npong = 0;
  ping_ts = pong_ts = Timestamp::now();
#endif
}


//##ModelId=3DB936790173
EchoWP::~EchoWP()
{
}



//##ModelId=3C7F884A007D
void EchoWP::init ()
{
  WorkProcess::init();
  
  config.get("bs",blocksize);
  lprintf(0,"blocksize = %d KB\n",blocksize);
  blocksize *= 1024/sizeof(double);
 
  config.get("pc",pcount); 
  lprintf(0,"pingcount = %d\n",pcount);

  config.get("pipe",pipeline);  
  lprintf(0,"pipeline = %d\n",pipeline);

  config.get("fill",fill);  
  lprintf(0,"fill = %d\n",fill);
  
  config.get("process",process);
  lprintf(0,"process = %d\n",(int)process);
  
  config.get("mt",threads);
  lprintf(0,"mt = %d\n",threads);

  if( !pcount )
    subscribe("Ping.*");
  else 
    subscribe(MsgHelloEchoWP);
}

//##ModelId=3C7E4AC70261
bool EchoWP::start ()
{
  WorkProcess::start();
//  if( pcount>0 )
//    sendPing();
#ifdef USE_THREADS
  for( int i=0; i<threads; i++ )
    createWorker();
#endif  
  return false;
}

//##ModelId=3C7E49AC014C
int EchoWP::receive (Message::Ref& mref)
{
  Timestamp now;
  lprintf(4,"received %s\n",mref.debug(10));
  if( mref->id()[0] == MsgPing && mref->from() != address() )
  {
    if( pcount>0 && !--pcount )
      dsp()->stopPolling();
#ifdef ENABLE_LATENCY_STATS
    LatencyVector lat = mref->latency;
    lat.measure("<PROC");
#endif
    Message & msg = mref;
    lprintf(3,"ping(%d) from %s\n",msg["Count"].as<int>(),mref->from().toString().c_str());
    // timestamp the reply
    Timestamp ts = msg["Timestamp"].as<Timestamp>();
    msg["Timestamp"] = msg["Reply.Timestamp"].as<Timestamp>();
    msg["Reply.Timestamp"] = now;
    // process the data block if it's there
    if( msg["Process"].as<bool>() )
    {
      ContainerIter<double> data(msg["Data"]);
      lprintf(4,"sqrting data block of %d doubles\n",data.size());
      while( !data.end() )
        data.next( sqrt(*data) );
    }
    int sz = msg["Data"].size();
    int pc = msg["Count"];
    msg["Count"] = ++pc;
#ifdef ENABLE_LATENCY_STATS
    msg.latency.clear();
    lat.measure("PROC1");
#endif
    lprintf(3,"replying with ping(%d)\n",pc);
    msg.setId(MsgPing|pc);
#ifdef ENABLE_LATENCY_STATS
    lat.measure("PROC2");
#endif
    bool roundtrip = msg.to() == address();
    if( roundtrip )
      stepCounters(sz*sizeof(double),ts);
    send(mref,msg.from());
    
#ifdef ENABLE_LATENCY_STATS
    if( roundtrip ) // if round-trip
    {
      lat.measure("PROC>");
      pinglat += lat;
      nping++;

      if( now.seconds() - ping_ts >= 10 )
      {
        pinglat /= nping;
        lprintf(0,"%d pings, latencies: %s\n",nping,pinglat.toString().c_str());
        pinglat.clear(); 
        nping = 0;
        ping_ts = now;
      }
    }
#endif
  }
  else if( mref->id().matches(MsgHelloEchoWP) )
  {
    if( mref->from() != address() ) // not our own?
    {
      lprintf(0,"found a friend: %s\n",mref->from().toString().c_str());
      bytecount = 0;
      msgcount = 0;
      ts = now;
#ifdef ENABLE_LATENCY_STATS
      ping_ts = pong_ts = now;
#endif
      for( int i=0; i<pipeline; i++ )
        sendPing(i);
//      addTimeout(1.0);
    }
  }
  return Message::ACCEPT;
}

//##ModelId=3C98CB600343
int EchoWP::timeout (const HIID &)
{
  return Message::ACCEPT;
}

// Additional Declarations
//##ModelId=3DB93679023B
void EchoWP::stepCounters ( size_t sz,const Timestamp &stamp )
{
  msgcount++;
  bytecount += (sz/1024)*2;
  double ts1 = Timestamp::now();
  timecount += ts1 - (double)stamp;
  if( ts1 - ts > 10 )
  {
    lprintf(0,"%.2f seconds elapsed since last report\n",ts1-ts);
    lprintf(0,"%ldKB round-trip data (%.2f MB/s)\n",
            bytecount,bytecount/(1024*(ts1-ts)));
    lprintf(0,"%ld round-trips (%.1f /s)\n",
            msgcount,msgcount/(ts1-ts));
    lprintf(0,"%.3f ms average round-trip time\n",timecount/msgcount*1000);
    
    bytecount = msgcount = 0;
    ts = ts1;
    timecount = 0;
  }
}
    
    
//##ModelId=3DB9367903B8
void EchoWP::sendPing (int pc)
{
  Message::Ref ref;
  Message &msg = ref <<= new Message(MsgPing|pc,new DMI::Record);
  msg["Timestamp"] = Timestamp();
  msg["Reply.Timestamp"] = Timestamp();
  msg["Process"] = process;
  msg["Data"] <<= new DMI::Vec(Tpdouble,blocksize);
  msg["Count"] = pcount;
  if( fill )
  {
    ContainerIter<double> data(msg["Data"]);
    lprintf(4,"filling %d doubles\n",data.size());
    while( !data.end() )
      data.next(fill);
  }
  lprintf(4,"ping %d, publishing %s\n",pcount,msg.debug(1));
  lprintf(3,"sending ping(%d)\n",msg["Count"].as<int>());
  publish(ref);
}

};
