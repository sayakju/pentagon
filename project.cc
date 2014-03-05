/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/error-model.h"
#include "ns3/tcp-header.h"
#include "ns3/udp-header.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/mpi-interface.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>



#define Distance 50
#define couleur(param) printf("\033[%sm",param)

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("ProjectScript");

int 
main (int argc, char *argv[])
{
  //uint32_t mtu_bytes = 40000;
  uint32_t nWifi = 12;
  std::string animFile = "project-animation.xml" ;
	
 // Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
	//		UintegerValue (100));  //enable RTS/CTS

  Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
			UintegerValue (2200));  //disable RTS/CTS
 
  CommandLine cmd;
  cmd.Parse (argc,argv);

  LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);

  if (nWifi > 18)
    {
      std::cout << "Number of wifi nodes " << nWifi << 
                   " specified exceeds the mobility bounding box" << std::endl;
      exit (1);
    }

  // Calculate the ADU size
  Header* temp_header = new Ipv4Header();
  uint32_t ip_header = temp_header->GetSerializedSize();
  NS_LOG_LOGIC ("IP Header size is: " << ip_header);
  delete temp_header;
  temp_header = new TcpHeader();
  uint32_t tcp_header = temp_header->GetSerializedSize();
  NS_LOG_LOGIC ("TCP Header size is: " << tcp_header);
  delete temp_header;
  //uint32_t tcp_adu_size = mtu_bytes - (ip_header + tcp_header);
  //NS_LOG_LOGIC ("TCP ADU size is: " << tcp_adu_size);

  NodeContainer p2pNodes;
  p2pNodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("1ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  NodeContainer csmaNodes;
  csmaNodes.Add (p2pNodes.Get (1));

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  NodeContainer wifiApNode = p2pNodes.Get (0);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetStandard(WIFI_PHY_STANDARD_80211g);
  //wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate54Mbps"));
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  QosWifiMacHelper mac = QosWifiMacHelper::Default ();

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

/*
 //Playing with Position of wifi nodes

    float deltaAngle = 2* M_PI / nWifi;
    float angle = 0.0;
    double x = 0.0;
    double y = 0.0;
std::vector<Ptr<ConstantPositionMobilityModel> > constant;
    Ptr<ListPositionAllocator> positionAllocSta;
    //Ptr<ListPositionAllocator> positionAllocAp;
    positionAllocSta = CreateObject<ListPositionAllocator>();
    //positionAllocAp = CreateObject<ListPositionAllocator>();

    //positionAllocAp->Add(Vector3D(0.0, 0.0, 0.0));

    

    constant.resize(nWifi + 1);
    for (int i = 0; i < nWifi + 1; i++) {
        constant[i] = CreateObject<ConstantPositionMobilityModel>();
    }
    for (int i = 0; i < nWifi; i++) {
        x = cos(angle) * Distance;
        y = sin(angle) * Distance;

        positionAllocSta->Add(Vector3D(x, y, 0.0));

        mobility.SetPositionAllocator(positionAllocSta);
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
       // mobilitySta.Install(m_wifiQSta.Get(i));
        mobility.Install(wifiStaNodes.Get(i));
        if (i==0)
            constant[i]->SetPosition(Vector3D(0.0, 0.0, 0.0));
        else
            constant[i]->SetPosition(Vector3D(x, y, 0.0));

        angle += deltaAngle;
    }

// end position of wifi nodes */

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (10.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (10),
                                 "LayoutType", StringValue ("RowFirst"));

 mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                           "Bounds", RectangleValue (Rectangle (-200, 200, -200, 200)));
 mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  //mobility.Install (csmaNodes);
  mobility.Install (wifiApNode);

  InternetStackHelper stack;
  stack.Install (csmaNodes);
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  Ipv4AddressHelper address;

  address.SetBase ("10.129.5.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  address.SetBase ("10.129.26.0", "255.255.255.0");

  Ipv4InterfaceContainer wifiInterfaces;
  wifiInterfaces = address.Assign (staDevices);

  address.Assign (apDevices);


  uint16_t port = 9;  // well-known echo port number

  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  
  ApplicationContainer sourceApps;
  ApplicationContainer sinkApps[12];
  for (int i=1;i<=12;i++)
  {
     BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (wifiInterfaces.GetAddress (nWifi - i), port));
     //source.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]")); //in seconds
    // source.SetAttribute ("OffTime",  StringValue ("ns3::ConstantRandomVariable[Constant=0]"));      
     //source.SetAttribute ("MaxBytes", UintegerValue (9638761));
     source.SetAttribute ("MaxBytes", UintegerValue (9416657));
     //source.SetAttribute ("MaxBytes", UintegerValue (6000000));
   //  source.SetAttribute("DataRate", StringValue("600kbps"));
    // source.SetAttribute("PacketSize", UintegerValue (1500));

     sourceApps = source.Install (p2pNodes.Get(1));

     if(i==1)   
     {
       sourceApps.Start (Seconds (13.66));
     }
     else if (i==2)
     {
       sourceApps.Start (Seconds (5.66));
     }
     else if (i==3)
     {
       sourceApps.Start (Seconds (11.76));
     }
     else if (i==4)
     {
       sourceApps.Start (Seconds (11.36));
     }
     else if (i==5)
     {
       sourceApps.Start (Seconds (5.66));
     }
     else if (i==6)
     {
       sourceApps.Start (Seconds (1));
     }
     else if (i==7)
     {
       sourceApps.Start (Seconds (6.08));
     }
     else if (i==8)
     {
       sourceApps.Start (Seconds (13.96));
     }
     else if (i==9)
     {
       sourceApps.Start (Seconds (2.38));
     }
     else if (i==10)
     {
       sourceApps.Start (Seconds (20.52));
     }
     else if (i==11)
     {
       sourceApps.Start (Seconds (14.88));
     }
     else
     {
       sourceApps.Start (Seconds (24.4));
     } 


     sourceApps.Stop (Seconds (500.0)); 
     sinkApps[i-1] = sink.Install (wifiStaNodes.Get (nWifi - i)); 



     sinkApps[i-1].Start (Seconds (0.0));
     sinkApps[i-1].Stop (Seconds (500.0));
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  Simulator::Stop (Seconds (500.0));

  pointToPoint.EnablePcapAll ("project");

  AnimationInterface anim (animFile);
  AnimationInterface::SetConstantPosition (p2pNodes.Get(1), 10, 10);

  Simulator::Run ();

  // Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
  int c = 0;
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
    {
	  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

      if (t.sourceAddress == Ipv4Address("10.129.5.2"))
         {
 	  std::cout << "Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress << std::endl;
          //std::cout << "MeanDelay: " << NanoSeconds((iter->second.delaySum)/(iter->second.rxPackets)) << std::endl;
    	  std::cout << "Tx Packets = " << iter->second.txPackets << std::endl;
    	  std::cout << "Rx Packets = " << iter->second.rxPackets << std::endl;
    	  std::cout << "Number of bytes received = " << iter->second.rxBytes << std::endl;
          std::cout << "Lost Packets: " << iter->second.lostPackets << std::endl;
          couleur("33");
          std::cout<<"Download time: "<<(iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds())<<std::endl;
    	  std::cout << "Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps" << std::endl;
          couleur("0");
        if (iter->second.rxBytes > 0)
			c++;
          std::cout << "-------------------------------------------" << "\n";
         }
          

      }    

	std::cout << "  Number of receiving nodes  :   " << c << "\n";


    Simulator::Destroy ();

  return 0;
}
