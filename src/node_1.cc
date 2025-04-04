#include <ctime>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <future>
#include <iomanip>
#include <fstream>
#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/fd-net-device-module.h"

#include "ros/ros.h"
#include "std_msgs/String.h"
#include "mavros_msgs/State.h"
#include "mavros_msgs/SetMode.h"
#include "mavros_msgs/CommandTOL.h"
#include "mavros_msgs/CommandBool.h"
#include "geometry_msgs/PoseStamped.h"
#include "geometry_msgs/TwistStamped.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("EmuFdNetDeviceSaturationExample");

mavros_msgs::State current_state;

void state_cb(const mavros_msgs::State::ConstPtr& msg)
{
    current_state.connected = msg->connected;
    current_state.mode = msg->mode;
}


class ReceiveMessage
{
	private:
		string receivedData;
		string msg;
        
    public:
        string remote_sender_ip;
        string remote_sender_mac;
        string local_receiver_ip;
        string local_receiver_mac;
        queue<Ptr<Packet>> packetQueue;
        ros::NodeHandle nh;
        ros::Publisher modePublisher;
        ros::Subscriber state_sub;

        ReceiveMessage() {};
        ReceiveMessage(string, string, string, string);
        void ReceivePacket(Ptr<Socket>);
};


ReceiveMessage::ReceiveMessage(string remote_sender_ip, string remote_sender_mac, string local_receiver_ip, string local_receiver_mac)
{
    this->remote_sender_ip = remote_sender_ip;
    this->remote_sender_mac = remote_sender_mac;
    this->local_receiver_ip = local_receiver_ip;
    this->local_receiver_mac = local_receiver_mac;

    // string topic_name = "mode_of_UAV" + string(1, this->remote_sender_ip.back());  
    string topic_name = "received_mode";
    this->modePublisher = nh.advertise<std_msgs::String>(topic_name, 10);
    this->state_sub = nh.subscribe<mavros_msgs::State>("/mavros/state", 10, &state_cb);
    ros::Rate rate(1);
}


void ReceiveMessage::ReceivePacket(Ptr<Socket> socket)
{
    mavros_msgs::SetMode custom_set_mode;
	while (Ptr<Packet> msg = socket->Recv())
    {
        uint8_t* buffer = new uint8_t[msg->GetSize()];
        msg->CopyData(buffer, msg->GetSize());
        receivedData = string ((char*)buffer);
       
        auto now =chrono::system_clock::now();
        auto now_in_seconds = chrono::system_clock::to_time_t(now);
        auto duration = now.time_since_epoch();
        auto milliseconds = chrono::duration_cast<chrono::milliseconds>(duration);
        auto nanoseconds = chrono::duration_cast<chrono::nanoseconds>(duration);
        tm* time_info = localtime(&now_in_seconds);
        ostringstream timestamp;
        timestamp << put_time(time_info, "%Y-%m-%d %H:%M:%S");
        timestamp << "." << setw(3) << setfill('0') << milliseconds.count() % 1000 << "." << setw(9) << setfill('0') << nanoseconds.count() % 1000000000;

        ros::spinOnce();

        string deviceData = string("\nNODE-1:") + "\nTimestamp: " + timestamp.str() + "\nUAV Mode: " + current_state.mode;
        cout << "\n----------------------------------------------------------------------------------";
        cout << deviceData + receivedData << endl;
        cout << "----------------------------------------------------------------------------------";

        istringstream stream(receivedData);
        string line;
        vector<string> lines;

        while (getline(stream, line)) {
            lines.push_back(line);
        }

        if (lines.size() >= 2) {
            string secondLastLine = lines[lines.size() - 2];
            string prefix = "Mode: ";
            if (secondLastLine.find(prefix) != string::npos) {
                string extracted_mode =  secondLastLine.substr(prefix.size());
                std_msgs::String ros_extracted_mode;
                ros_extracted_mode.data = extracted_mode;
                this->modePublisher.publish(ros_extracted_mode);
            }
        }

        string modifiedMessage = deviceData + receivedData;
        Ptr<Packet> newPacket = Create<Packet>((uint8_t*)modifiedMessage.c_str(), modifiedMessage.size());
        this->packetQueue.push(newPacket);
    }
}


class SendMessage
{
	private:
        static string msg;
    public:
        string local_sender_ip;
        string local_sender_mac;
        string remote_receiver_ip;
        string remote_receiver_mac;
        ros::NodeHandle nh;
        ros::Subscriber state_sub;

        SendMessage() {};
        SendMessage(string, string, string, string);
        void RelayMessage(Ptr<Socket>, Ptr<Packet>);
        static void GenerateMessage(Ptr<Socket>, uint32_t, uint32_t, ns3::Time);
};

string SendMessage::msg = "";

SendMessage::SendMessage(string local_sender_ip, string local_sender_mac, string remote_receiver_ip,  string remote_receiver_mac)
{
    this->local_sender_ip = local_sender_ip;
    this->local_sender_mac = local_sender_mac;
    this->remote_receiver_ip = remote_receiver_ip;
    this->remote_receiver_mac = remote_receiver_mac;

    this->state_sub = nh.subscribe<mavros_msgs::State>("/mavros/state", 10, &state_cb);
    ros::Rate rate(1);
}


void SendMessage::RelayMessage(Ptr<Socket> socket, Ptr<Packet> packet)
{
	socket->Send(packet);
}


void SendMessage::GenerateMessage(Ptr<Socket> socket, uint32_t pktSize, uint32_t pktcount, ns3::Time pktInterval)
{
   if (pktcount > 0 && pktcount <31)
    {
        string pktcount_str = to_string(pktcount);
        Ptr<Packet> packet = Create<Packet>((uint8_t*)SendMessage::msg.c_str(),SendMessage::msg.length());
        socket->Send(packet);

        ros::spinOnce();

        auto now = chrono::system_clock::now();
        auto now_in_seconds = chrono::system_clock::to_time_t(now);
        auto duration = now.time_since_epoch();
        auto milliseconds = chrono::duration_cast<chrono::milliseconds>(duration);
        auto nanoseconds = chrono::duration_cast<chrono::nanoseconds>(duration);
        tm* time_info = localtime(&now_in_seconds);
        ostringstream timestamp;
        timestamp << put_time(time_info, "%Y-%m-%d %H:%M:%S");
        timestamp << "." << setw(3) << setfill('0') << milliseconds.count() % 1000 << "." << setw(9) << setfill('0') << nanoseconds.count() % 1000000000;
        string timeString = timestamp.str();

        if(current_state.connected){
                SendMessage::msg = string("\nNODE-1: ") + "\nTimestamp: " + timeString + "\nUAV Mode: " + current_state.mode + "\nPacket Number: " + pktcount_str + "_";
        }
        else{
                SendMessage::msg = string("\nNODE-1: ") + "\nTimestamp: " + timeString + "\nUAV Mode: Not Connected" + "\nPacket Number: " + pktcount_str + "_";
        }

        cout << "-----------------------------------------------------------------------------\n";
        cout << msg << endl;
        cout << "-----------------------------------------------------------------------------\n";
        Simulator::Schedule(pktInterval, &GenerateMessage, socket, pktSize, pktcount - 1, pktInterval);
    }
    else
    {       
        cout << "Closing Connection" << endl;
        ros::spin();
    }
}


class MessageTransfer{
    private:

    public:
        bool acting_as_sender;
        bool acting_as_forwarder;
        bool acting_as_receiver;

        uint32_t packetSize;
        uint32_t pktcount;
        ns3::Time interPacketInterval;
        Ptr<Socket> common_socket;
        Ptr<Packet> common_packet;
        ReceiveMessage receive_msg;
        SendMessage send_msg;
        InetSocketAddress local = InetSocketAddress("du.m.m.y", 100);
        InetSocketAddress remote = InetSocketAddress("du.m.m.y", 100);

        MessageTransfer(ReceiveMessage);
        MessageTransfer(SendMessage);
        MessageTransfer(ReceiveMessage, SendMessage);
        void Execute();
};


MessageTransfer::MessageTransfer(ReceiveMessage receive_msg)
{
    this->acting_as_sender = false;
    this->acting_as_forwarder = false;
    this->acting_as_receiver = true;
    this->receive_msg = receive_msg;

    uint16_t sinkPort = 80;
    string dataRate("1000Mb/s");
    string deviceName ("wlan0");
    string netmask ("255.255.255.0");
    
    Ipv4Address remote_sender_ip;
    Ipv4Address local_ip;
    Mac48AddressValue local_mac;
    
    local_ip = Ipv4Address (this->receive_msg.local_receiver_ip.c_str ());
    local_mac = Mac48AddressValue (this->receive_msg.local_receiver_mac.c_str ());
    remote_sender_ip = Ipv4Address (this->receive_msg.remote_sender_ip.c_str ());
    
    Ipv4Mask localMask (netmask.c_str ());
    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
    
    NS_LOG_INFO ("Create Node");
    Ptr<Node> node = CreateObject<Node> ();
    
    NS_LOG_INFO ("Create Device");
    EmuFdNetDeviceHelper emu;
    emu.SetDeviceName (deviceName);
    NetDeviceContainer devices = emu.Install (node);
    Ptr<NetDevice> device = devices.Get (0);
    device->SetAttribute ("Address", local_mac);

    NS_LOG_INFO ("Add Internet Stack");
    InternetStackHelper internetStackHelper;
    internetStackHelper.SetIpv4StackInstall(true);
    internetStackHelper.Install (node);
    
    NS_LOG_INFO ("Create IPv4 Interface");
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    uint32_t interface = ipv4->AddInterface (device);
    Ipv4InterfaceAddress address = Ipv4InterfaceAddress (local_ip, localMask);
    ipv4->AddAddress (interface, address);
    ipv4->SetMetric (interface, 1);
    ipv4->SetUp (interface);
    
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    this->common_socket = Socket::CreateSocket(node, tid);
    this->local = InetSocketAddress(local_ip, sinkPort);
    this->Execute();

    AsciiTraceHelper ascii;
    emu.EnableAsciiAll(ascii.CreateFileStream("UDP-receiver-forwarder.tr"));
    emu.EnablePcap ("receiver_final", device);
  
    Simulator::Stop (Seconds (60.0));
    Simulator::Run ();
    Simulator::Destroy ();
}


MessageTransfer::MessageTransfer(SendMessage send_msg)
{
    this->acting_as_sender = true;
    this->acting_as_forwarder = false;
    this->acting_as_receiver = false;
    this->send_msg = send_msg;

    uint16_t sinkPort = 80;
    double interval = 1.0;
    this->packetSize = 1000;
    this->pktcount = 30;
    this->interPacketInterval = Seconds(interval);
    string dataRate("1000Mb/s");
    string deviceName ("wlan0");
    string netmask ("255.255.255.0");
    
    Ipv4Address remote_receiver_ip;
    Ipv4Address local_ip;
    Mac48AddressValue local_mac;
    
    local_ip = Ipv4Address (this->send_msg.local_sender_ip.c_str ());
    local_mac = Mac48AddressValue (this->send_msg.local_sender_mac.c_str ());
    remote_receiver_ip = Ipv4Address (this->send_msg.remote_receiver_ip.c_str ());
    
    Ipv4Mask localMask (netmask.c_str ());
    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
    
    NS_LOG_INFO ("Create Node");
    Ptr<Node> node = CreateObject<Node> ();
    
    NS_LOG_INFO ("Create Device");
    EmuFdNetDeviceHelper emu;
    emu.SetDeviceName (deviceName);
    NetDeviceContainer devices = emu.Install (node);
    Ptr<NetDevice> device = devices.Get (0);
    device->SetAttribute ("Address", local_mac);

    NS_LOG_INFO ("Add Internet Stack");
    InternetStackHelper internetStackHelper;
    internetStackHelper.SetIpv4StackInstall(true);
    internetStackHelper.Install (node);
    
    NS_LOG_INFO ("Create IPv4 Interface");
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    uint32_t interface = ipv4->AddInterface (device);
    Ipv4InterfaceAddress address = Ipv4InterfaceAddress (local_ip, localMask);
    ipv4->AddAddress (interface, address);
    ipv4->SetMetric (interface, 1);
    ipv4->SetUp (interface);
    
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    this->common_socket = Socket::CreateSocket(node, tid);
    this->local = InetSocketAddress(local_ip, sinkPort);
    this->remote = InetSocketAddress(remote_receiver_ip, sinkPort);
    this->Execute();

    AsciiTraceHelper ascii;
    emu.EnableAsciiAll(ascii.CreateFileStream("UDP-receiver-forwarder.tr"));
    emu.EnablePcap ("sender_final", device);

    Simulator::Stop (Seconds (60.0));
    Simulator::Run ();
    Simulator::Destroy ();
}


MessageTransfer::MessageTransfer(ReceiveMessage receive_msg, SendMessage send_msg)
{
    this->acting_as_sender = false;
    this->acting_as_forwarder = true;
    this->acting_as_receiver = false;
    this->send_msg = send_msg;
    this->receive_msg = receive_msg;

    uint16_t sinkPort = 80;
    double interval = 1.0;
    this->packetSize = 1000;
    this->pktcount = 30;
    this->interPacketInterval = Seconds(interval);
    string dataRate("1000Mb/s");
    string deviceName ("wlan0");
    string netmask ("255.255.255.0");
    
    Ipv4Address remote_sender_ip, remote_receiver_ip;
    Ipv4Address local_ip;
    Mac48AddressValue local_mac;
    
    local_ip = Ipv4Address (this->receive_msg.local_receiver_ip.c_str ());
    local_mac = Mac48AddressValue (this->receive_msg.local_receiver_mac.c_str ());
    remote_sender_ip = Ipv4Address (this->receive_msg.remote_sender_ip.c_str ());
    remote_receiver_ip = Ipv4Address (this->send_msg.remote_receiver_ip.c_str ());
    
    Ipv4Mask localMask (netmask.c_str ());
    GlobalValue::Bind ("SimulatorImplementationType", StringValue ("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
    
    NS_LOG_INFO ("Create Node");
    Ptr<Node> node = CreateObject<Node> ();
    
    NS_LOG_INFO ("Create Device");
    EmuFdNetDeviceHelper emu;
    emu.SetDeviceName (deviceName);
    NetDeviceContainer devices = emu.Install (node);
    Ptr<NetDevice> device = devices.Get (0);
    device->SetAttribute ("Address", local_mac);

    NS_LOG_INFO ("Add Internet Stack");
    InternetStackHelper internetStackHelper;
    internetStackHelper.SetIpv4StackInstall(true);
    internetStackHelper.Install (node);
    
    NS_LOG_INFO ("Create IPv4 Interface");
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4> ();
    uint32_t interface = ipv4->AddInterface (device);
    Ipv4InterfaceAddress address = Ipv4InterfaceAddress (local_ip, localMask);
    ipv4->AddAddress (interface, address);
    ipv4->SetMetric (interface, 1);
    ipv4->SetUp (interface);
    
    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    this->common_socket = Socket::CreateSocket(node, tid);
    this->local = InetSocketAddress(local_ip, sinkPort);
    this->remote = InetSocketAddress(remote_receiver_ip, sinkPort);
    this->Execute();

    AsciiTraceHelper ascii;
    emu.EnableAsciiAll(ascii.CreateFileStream("UDP-receiver-forwarder.tr"));
    emu.EnablePcap ("forwarder_final", device);

    Simulator::Stop (Seconds (60.0));
    Simulator::Run ();
    Simulator::Destroy ();
}


void MessageTransfer::Execute()
{
    if (this->acting_as_sender == true)
    {
        static bool isSocketBound = false;

        if (!isSocketBound) {
            this->common_socket->Bind(this->local);
            this->common_socket->Connect(this->remote);
            isSocketBound = true;
        }
        Simulator::Schedule(Seconds(1.0), &send_msg.GenerateMessage, this->common_socket, this->packetSize, this->pktcount, this->interPacketInterval);
    }

    else if (this->acting_as_forwarder == true)
    {
        static bool isSocketBound = false;

        if (!isSocketBound) {
            this->common_socket->Bind(this->local);
            this->common_socket->SetRecvCallback(MakeCallback(&ReceiveMessage::ReceivePacket, &receive_msg));
            this->common_socket->Connect(this->remote);
            isSocketBound = true;
        }

        while (!this->receive_msg.packetQueue.empty())
        {
            this->common_packet = this->receive_msg.packetQueue.front();
            this->receive_msg.packetQueue.pop();

            if (this->common_packet) {
                send_msg.RelayMessage(this->common_socket, this->common_packet);
            }
        }
        Simulator::Schedule(Seconds(0.1), &MessageTransfer::Execute, this);
    }

    else if (this->acting_as_receiver == true)
    {
        static bool isSocketBound = false;

        if (!isSocketBound) {
            this->common_socket->Bind(this->local);
            this->common_socket->SetRecvCallback(MakeCallback(&ReceiveMessage::ReceivePacket, &receive_msg));
            this->common_socket->Connect(this->remote);
            isSocketBound = true;
        }
    }

    else
    {
        cerr << "Cant perform multiple roles in same thread!" << endl;
    }
}


int main (int argc, char *argv[])
{
    ros::init(argc, argv, "UAV1");

    //------------------------------------------------------------------------------
    //-------------------------------- 3-NODE-2-HOP --------------------------------
    //------------------------------------------------------------------------------
    // string NODE_1_IP = "10.1.1.1";
    // string NODE_1_MAC = "e8:4e:06:32:17:a6";
    // string NODE_2_IP = "10.1.1.4";
    // string NODE_2_MAC = "e8:4e:06:32:28:dd";
    // string NODE_3_IP = "10.1.1.3";
    // string NODE_3_MAC = "e8:4e:06:32:22:99";

    //----------------------------- TO (FORWARD 1-4-3) -----------------------------
    // ReceiveMessage rec_msg_1(NODE_1_IP, NODE_1_MAC, NODE_2_IP, NODE_2_MAC);
    // SendMessage send_msg_1(NODE_2_IP, NODE_2_MAC, NODE_3_IP, NODE_3_MAC);
    // MessageTransfer msg_tran_1(rec_msg_1, send_msg_1);

    //---------------------------- FRO (FORWARD 3-4-1) -----------------------------
    // ReceiveMessage rec_msg_1(NODE_3_IP, NODE_3_MAC, NODE_2_IP, NODE_2_MAC);
    // SendMessage send_msg_1(NODE_2_IP, NODE_2_MAC, NODE_1_IP, NODE_1_MAC);
    // MessageTransfer msg_tran_1(rec_msg_1, send_msg_1);


    //##############################################################################
    //##############################################################################


    //------------------------------------------------------------------------------
    //-------------------------------- 2-NODE-1-HOP --------------------------------
    //------------------------------------------------------------------------------
    // string NODE_1_IP = "10.1.1.1";
    // string NODE_1_MAC = "e8:4e:06:32:17:a6";
    // string NODE_2_IP = "10.1.1.4";
    // string NODE_2_MAC = "e8:4e:06:32:28:dd";
    // string NODE_3_IP = "10.1.1.3";
    // string NODE_3_MAC = "e8:4e:06:32:22:99";

    //-------------------------------- RECEIVE 4-1 ---------------------------------
    // ReceiveMessage rec_msg_1(NODE_2_IP, NODE_2_MAC, NODE_1_IP, NODE_1_MAC);
    // MessageTransfer msg_tran_1(rec_msg_1);

    //---------------------------------- SEND 1-4 ----------------------------------
    // SendMessage send_msg_1(NODE_1_IP, NODE_1_MAC, NODE_2_IP, NODE_2_MAC);
    // MessageTransfer msg_tran_1(send_msg_1);

    //-------------------------------- RECEIVE 3-1 ---------------------------------
    // ReceiveMessage rec_msg_1(NODE_3_IP, NODE_3_MAC, NODE_1_IP, NODE_1_MAC);
    // MessageTransfer msg_tran_1(rec_msg_1);

    //---------------------------------- SEND 1-3 ----------------------------------
    // SendMessage send_msg_1(NODE_1_IP, NODE_1_MAC, NODE_3_IP, NODE_3_MAC);
    // MessageTransfer msg_tran_1(send_msg_1);

    //------------------------------------------------------------------------------

    return 0;
}
