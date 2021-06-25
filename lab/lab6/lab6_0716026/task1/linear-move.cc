#include "ns3/core-module.h"
#include "ns3/wifi-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/bridge-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("LinearMove");

void CheckLinkState (Ptr<Node> node);
void Movement (Ptr<Node> node, double stepsSize, double stepsTime);

int main (int argc, char *argv[])
{
  std::string phyMode ("DsssRate1Mbps");
  double frequency = 2.43*1e9; // center frequency, Hz
  bool verbose = false;
  uint32_t packetSize = 1024; 
  double interval = 1.0; 
  double stepsSize = 2.0;
  double stepsTime = 1.0;
  double txPower = 5; 
  double snrThreshold = 20.0;
  double ap1Position = 10.0, ap2Position = 50.0;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.AddValue ("packetSize", "size of application packet sent (bytes)", packetSize);
  cmd.AddValue ("interval", "interval (seconds) between packets", interval);
  cmd.AddValue ("txPower", "the transimission power of the access points", txPower);
  cmd.AddValue ("stepsSize", "the step size of echo client", stepsSize);
  cmd.AddValue ("stepsTime", "the period of echo client's movement", stepsTime);
  cmd.AddValue ("snrThreshold", "threshold of the signal quality (dB)", snrThreshold);
  cmd.AddValue ("ap1Position", "AP1 is at (ap1Position, 0.0, 2.5)", ap1Position);
  cmd.AddValue ("ap2Position", "AP1 is at (ap2Position, 0.0, 2.5)", ap2Position);
  cmd.Parse (argc, argv);

  // basic node setup
  // [index, node]: 
  // wirelessNode: [0, echo client], [1, ap1], [2, ap2]
  // csmaNode: [0, echo server], [1, ap1], [2, ap2]
  NodeContainer wirelessNodes;
  wirelessNodes.Create (3);
  NodeContainer csmaNodes;
  csmaNodes.Create (1);
  csmaNodes.Add (wirelessNodes.Get (1));
  csmaNodes.Add (wirelessNodes.Get (2));

  // ethernet setup
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));
  NetDeviceContainer csmaDevices;
  csmaDevices = csma.Install (csmaNodes);

  // wireless setup
  WifiHelper wifi;
  if (verbose)
  {
    wifi.EnableLogComponents ();  // Turn on all Wifi logging
  }
  wifi.SetStandard (WIFI_STANDARD_80211b);

  // physical layer setup
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  wifiPhy.Set ("TxPowerStart", DoubleValue(txPower));
  wifiPhy.Set ("TxPowerEnd", DoubleValue(txPower));
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel",
                                  "Frequency",DoubleValue (frequency),
                                  "SystemLoss",DoubleValue(10.0));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // mac layer setup
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode",StringValue (phyMode),
                                "ControlMode",StringValue (phyMode));
  Ssid ssid = Ssid ("wifi-default");
  wifiMac.SetType ("ns3::StaWifiMac",
                  "Ssid", SsidValue (ssid),
                  "SNR_Threshold", DoubleValue(snrThreshold), 
                  "ActiveProbing", BooleanValue (true) 
                  );
  
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, wirelessNodes.Get (0));
  NetDeviceContainer wirelessDevices = staDevice;
  
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, wirelessNodes.Get (1));
  wirelessDevices.Add (apDevice);
  
  NetDeviceContainer apDevice2 = wifi.Install (wifiPhy, wifiMac, wirelessNodes.Get (2));
  wirelessDevices.Add (apDevice2);

  BridgeHelper bridge;
  NetDeviceContainer bridgeDev,bridgeDev2;
  bridgeDev = bridge.Install (wirelessNodes.Get (1), NetDeviceContainer (apDevice, csmaDevices.Get (1)));
  bridgeDev2 = bridge.Install (wirelessNodes.Get (2), NetDeviceContainer (apDevice2, csmaDevices.Get (2)));
  
  // set nodes' position 
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (ap2Position, 0.0, 0.0));
  positionAlloc->Add (Vector (ap1Position, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wirelessNodes);
  
  // tcp/ip setup
  InternetStackHelper internet;
  internet.Install (wirelessNodes.Get(0));
  internet.Install (csmaNodes.Get(0));
  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer csmaInterfaces = address.Assign (csmaDevices.Get(0));
  Ipv4InterfaceContainer wirelessInterfaces = address.Assign (wirelessDevices.Get(0));

  // echo service setup
  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (0));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (60.0));
  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (2000));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (interval)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (packetSize));
  ApplicationContainer clientApps = echoClient.Install (wirelessNodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (60.0));

  // simulation setup
  wifiPhy.EnablePcap ("linear-move", wirelessDevices);

  Simulator::Schedule (Seconds (stepsTime), &Movement, wirelessNodes.Get (0), stepsSize, stepsTime);
  Simulator::Stop (Seconds (60.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}


void CheckLinkState (Ptr<Node> node)
{
  Ptr<NetDevice> wifidevice = node->GetDevice(0);
  Ptr<WifiMac> clientMac = DynamicCast<WifiNetDevice> (wifidevice)->GetMac ();
  std::cout << Simulator::Now().As(Time::S) << " " ;
  if( DynamicCast<StaWifiMac>(clientMac)->IsAssociated() )
  {
    std::cout << "Client connects to AP " << DynamicCast<RegularWifiMac>(clientMac)->GetBssid() << std::endl;
  }
  else
  {
    std::cout << "Client does not connect to AP" << std::endl;
  }
  return;
}

void Movement (Ptr<Node> node, double stepsSize, double stepsTime)
{
  // [TODO]: 
  // 1. Get latest position
  // 2. Update position based on stepsSize
  // 3. Print current postion (to stdout)
  Ptr<MobilityModel> mob = node->GetObject<MobilityModel>();
  Vector m_position = mob->GetPosition();
  m_position.x += 2;
  mob->SetPosition(m_position);
  double x = mob->GetPosition().x;
  double y = mob->GetPosition().y;
  double z = mob->GetPosition().z;
  std::cout << "Current position: ( " << x << ", " << y << ", " << z << " )" << std::endl;
  
  CheckLinkState(node); 
  Simulator::Schedule (Seconds (stepsTime), &Movement, node, stepsSize, stepsTime);
}
