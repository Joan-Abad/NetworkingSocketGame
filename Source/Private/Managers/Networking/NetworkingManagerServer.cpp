#include "Managers/Networking/NetworkingManagerServer.h"
#include <iostream>
#include <SFML/Network.hpp>
#include <json.h>
#include "Managers/GameManager.h"
#include "Player/Boat.h"
#include "Map/Map.h"

NetworkingManagerServer::NetworkingManagerServer() : serverManagementData(EServerManagementData::EWaitingPlayers), numPlayersToStartTheGame(2)
{

}

void NetworkingManagerServer::UpdateNetworkData()
{
	switch (serverManagementData)
	{
	case EServerManagementData::EWaitingPlayers:
		WaitForClientsToConnect();
		break;
	case EServerManagementData::EStartingTheMatch:
		std::cout << "Server: Starting the match\n";
		StartGameServerAndClients();
		break;
		//This case is if we want to add a countdown to start the match
	case EServerManagementData::EStartTheMatch:
		break;
	case EServerManagementData::EPlayMatch:
		RecieveGameDataFromClients();
		SendGameDataToClients();
		break;
	case EServerManagementData::EEndMatch:
		break;
	default:
		break;
	}
}

void NetworkingManagerServer::WaitForClientsToConnect()
{
	if (bDisplayMessageWaitingForClients)
	{
		std::cout << "Waiting for clients to connect...\n";
		bDisplayMessageWaitingForClients = false;
	}

	//Wait until the game has reached the number of players able to start the game
	if (players.size() != numPlayersToStartTheGame)
	{
		sf::Packet packet;
		sf::IpAddress clientAddress;
		unsigned short clientPort;

		//recieve packet from clients and check if they want to access the game 
		if (udpSocket.receive(packet, clientAddress, clientPort) == sf::Socket::Done)
		{
			std::string message;
			packet >> message;

			std::cout << "Server: Recieved a message from " << clientAddress << " on port " << clientPort << ": \n";
			std::cout << message;

			Json::Value root;
			Json::Reader reader;
			bool parsingSuccessful = reader.parse(message, root);
			if (parsingSuccessful)
			{
				//Add the player if has the right variables
				bool bWantToPlay = root[accessKey].asBool();
				if (bWantToPlay)
				{
					std::cout << "\nServer: Player " << players.size() << " connected\n";
					players["Player" + players.size()] = PlayerConnectionInfo(clientAddress, clientPort);

					//Send a response to the player
					sf::Packet ConfirmationToThePlayer;

					Json::Value replyRoot;
					replyRoot[connectionWithServer] = true;

					Json::StreamWriterBuilder writerBuilder;
					std::string acceptedByServer = Json::writeString(writerBuilder, replyRoot);
					ConfirmationToThePlayer << acceptedByServer;


					sf::Socket::Status status = udpSocket.send(ConfirmationToThePlayer, clientAddress, clientPort);
				}
				else
				{
					std::cerr << "Recieved some unnexpected data: " << message << std::endl;
				}

			}
		}
	}
	else
	{
		serverManagementData = EServerManagementData::EStartingTheMatch;
	}
}

void NetworkingManagerServer::StartGameServerAndClients()
{
	//
	int internalPlayerID = 0; 
	for (auto& player : players)
	{
		//Ommit sever send data to itself. 
		if (internalPlayerID != 0)
		{
			//Send the start the game flag to clients
			sf::Packet packet_StartTheGame;
			Json::Value root;
			root[startGameKey] = true;
			root[numPlayers] = numPlayersToStartTheGame;
			root[key_PlayerID] = internalPlayerID;
			Json::StreamWriterBuilder writerBuilder;
			std::string msgToSend = Json::writeString(writerBuilder, root);
			packet_StartTheGame << msgToSend;

			sf::Socket::Status status = udpSocket.send(packet_StartTheGame, player.second.ipAddress, player.second.port);
		}
		internalPlayerID++;
	}

	//Player Server initializes its own game
	serverManagementData = EServerManagementData::EPlayMatch;
	GameManager::GetGameManager()->InitGameWindow();
	GameManager* gm = GameManager::GetGameManager();
	
	gm->InitGameMap(gm->GetMap(gm->LakeMap), players.size());

}

void NetworkingManagerServer::SendGameDataToClients()
{
	if (!GetRootData().empty())
	{
		sf::Packet packet;

		//AddPacketHeader();
		Json::StreamWriterBuilder writerBuilder;
		std::string msgToSend = Json::writeString(writerBuilder, GetRootData());

		packet << msgToSend;
		//send a packet to all connected clients
		int i = 0; 
		for (auto& player : players)
		{
			if(i != 0)
				udpSocket.send(packet, player.second.ipAddress, player.second.port);

			i++;
		}

		ClearRootData();
	}
}

void NetworkingManagerServer::RecieveGameDataFromClients()
{
	sf::Packet packet;
	sf::IpAddress clientAddress;
	unsigned short clientPort;

	//recieve packet from clients and check if they want to access the game 
	if (udpSocket.receive(packet, clientAddress, clientPort) == sf::Socket::Done)
	{
		std::string message;
		packet >> message;

		std::cout << "Server: Recieved a message from " << clientAddress << " on port " << clientPort << ": \n";
		std::cout << message;

		Json::Value root;
		Json::Reader reader;
		bool parsingSuccessful = reader.parse(message, root);
		if (parsingSuccessful)
		{
			//Move the boat
			int boatID = -1;
			if (root.isMember(NetworkingManager::key_PlayerID))
			{
				boatID = root[Boat::key_AccelerateBoatID].asInt();
			}
			bool rotateLeft = false, rotateRight = false;
			if (root.isMember(Boat::key_RotateBoatLeftID))
			{
				rotateLeft = true;
			}
			if (root.isMember(Boat::key_RotateBoatRightID))
			{
				rotateRight = true;
			}

			if (boatID != -1)
			{
				//TODO: remove this code from here to its own class. 
				Player* player = GameManager::GetGameManager()->GetCurrentMap()->GetPlayers()[boatID];

				Boat* boat = dynamic_cast<Boat*>(player);
				if (boat)
				{
					boat->AccelerateBoat();
					if (rotateLeft)
						boat->RotateBoatLeft();
					if (rotateRight)
						boat->RotateBoatRight();

				}
			}
		}
	}
}

void NetworkingManagerServer::OnInit()
{
	udpSocket.bind(gamePort);

	//Set the server as its own player
	players["player0"] = PlayerConnectionInfo(sf::IpAddress::getLocalAddress(), gamePort);

	udpSocket.setBlocking(false);
}