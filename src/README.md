# Navigate around the code
The code stack is designed to have maximum modularity and freedom when designing a node and its role in a complex communication network. We have focussed on compartmentalizing Sending and Receiving packets, thus making packet Forwarding a disjoint process from a coding POV, while still maintaining traditional routing mechanism. The scripts consists of three classes:
* SendMessage Class
* ReceiveMessage Class
* MessageTransfer Class

## SendMessage
* This class handles one way message transfer between local (sender) and remote (receiver/forwarder) IP adresses.
* Handles for both, generating as well as relaying already received packets from local to remote.
* Input arguments in order - Local IP, Local Mac, Remote IP, Remote Mac.
* Function for generating packets - `GenerateMessage()`.
* Function for relaying packets - `RelayMessage()`.

## ReceiveMessage
* This class handles one way message transfer between remote (sender) and local (receiver) IP adresses.
* Handles for only receiving packets sent from remote to local.
* Input arguments in order - Remote IP, Remote Mac, Local IP, Local Mac.
* Function for receiving packets - `ReceivePacket()`.

## MessageTransfer
* Executes all node functions including sending, receiving and forwarding packets.
* Input arguments in order - object of ReceiveMessage class, object of SendMessage class.
* Any sort of message transfer functionality has to go through this class. So both arguments are optional as long as atleast one functionailty is mentioned.
* Function for carrying out message transfer operation - `Execute()`. Function is called inside class constructor itself so explicit call in main is to be avoided.
* For sole purpose of receiving packets, function call should look like - MessageTransfer(ReceiveMessage object)
* For sole purpose of generating and sending packets, function call should look like - MessageTransfer(SendMessage object)
* For sole purpose of relaying packets, function call should look like - MessageTransfer(ReceiveMessage object, SendMessage object). Order of argument types is important here.
