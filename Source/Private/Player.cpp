#include "Player.h"
#include <iostream>
#include <SFML/Network.hpp>
#include "NetworkingManager.h"
#include "NetworkingManagerServer.h"
#include "NetworkingManagerClient.h"
#include "ApplicationHelper.h"
#include <json.h>
#include <vector>

sf::Vector2f calculateForwardVectorDegrees(float angleDegrees) {
	float angleRadians = angleDegrees * 3.14159265358979323846f / 180.f; // Convert degrees to radians
	return sf::Vector2f(std::cos(angleRadians), std::sin(angleRadians));
}

sf::Vector2f normalize(const sf::Vector2f& vector) {
	float length = std::sqrt(vector.x * vector.x + vector.y * vector.y);

	if (length != 0) {
		return sf::Vector2f(vector.x / length, vector.y / length);
	}
	else {
		// Handle division by zero or zero-length vectors
		return sf::Vector2f(0.f, 0.f);
	}
}

Player::Player(sf::Window& window, bool PlayerPlayable, const char* texturePath) : 
	playerLifes(3), boatAcceleration({0.f,0.f}), speed(0), angleBoatSpeedEachSecond(360)
{
	this->window = &window;

	//Player Input Actions
	this->IsPlayerPlayable = PlayerPlayable;
	if (PlayerPlayable)
	{
		//Test space
		action_Space.SetKey(sf::Keyboard::Key::Space);
		auto address = &Player::PrintPressed;
		action_Space.OnKeyTriggered = BindAction(&Player::PrintPressed, this);
		action_Space.OnKeyOnGoing = BindAction(&Player::PrintRndomMessage, this);
		action_Space.OnKeyReleased = BindAction(&Player::PrintReleased, this);

		//Test networking
		action_P.SetKey(sf::Keyboard::Key::P);
		action_P.OnKeyTriggered = BindAction(&Player::SendTestPacket, this);

		//Boat actions
		action_W.SetKey(sf::Keyboard::Key::W);
		action_W.OnKeyOnGoing = BindAction(&Player::AccelerateBoat, this);

		action_S.SetKey(sf::Keyboard::Key::S);
		action_S.OnKeyOnGoing = BindAction(&Player::DecelerateBoat, this);

		//Rotate boat left
		action_RotateLeft.SetKey(sf::Keyboard::Key::Q);
		action_RotateLeft.OnKeyOnGoing = BindAction(&Player::RotateBoatLeft, this);

		//Rotate boat right
		action_RotateRight.SetKey(sf::Keyboard::Key::E);
		action_RotateRight.OnKeyOnGoing = BindAction(&Player::RotateBoatRight, this);
	}

	forwardVector = sf::Vector2f(0.f, 1.0f);

	if (!playerTexture.loadFromFile(texturePath))
		std::cerr << "BackgroundPath error\n";

	playerSprite.setTexture(playerTexture);

	// Set the origin to the center of the sprite
	playerSprite.setOrigin(playerSprite.getLocalBounds().width / 2, playerSprite.getLocalBounds().height / 2);
	if (!IsPlayerPlayable)
	{
//		NetworkingManager* manager;
//#if _SERVER
//		manager = &NetworkingManager::GetNetworkingManager<NetworkingManagerServer>();
//#elif _CLIENT
//		manager = &NetworkingManager::GetNetworkingManager<NetworkingManagerClient>();
//#endif
//		//manager->.push_back()
//		manager->AddNetworkDataCallback(&Player::UpdaetPlayerInfo, this);
	}
	root.clear();

}

void Player::HandlePlayerInput()
{
	sf::Event inputEvent;
	while (window->pollEvent(inputEvent))
	{
		if (inputEvent.type == sf::Event::Closed)
			window->close();
	}
	
	CheckKeyPressed(action_Space);
	CheckKeyPressed(action_P);
	CheckKeyPressed(action_W);
	CheckKeyPressed(action_S);
	CheckKeyPressed(action_RotateLeft);
	CheckKeyPressed(action_RotateRight);
}

void Player::CheckKeyPressed(InputAction& inputAction)
{
	bool bIsSpacePressedThisFrame = sf::Keyboard::isKeyPressed(inputAction.GetKey());

	if (bIsSpacePressedThisFrame)
	{
		//On Trigger
		if (!inputAction.GetIsKeyPressed())
		{
			inputAction.SetIsKeyPressed(true);
			if (inputAction.OnKeyTriggered)
				inputAction.OnKeyTriggered(); 
		}
		//On Going
		if(inputAction.OnKeyOnGoing)
			inputAction.OnKeyOnGoing();
	}
	else
	{
		//On Released
		if (inputAction.GetIsKeyPressed())
		{
			if (inputAction.OnKeyReleased)
				inputAction.OnKeyReleased();
		}
		inputAction.SetIsKeyPressed(false);
	}
}

void Player::Draw(sf::RenderWindow& window)
{
	window.draw(playerSprite);
}

float Player::GetRotation()
{
	return playerSprite.getRotation();
}

void Player::Update()
{
	if (!root.empty() && IsPlayerPlayable)
	{		
		Json::StreamWriterBuilder writerBuilder;
		std::string msgToSend = Json::writeString(writerBuilder, root);;

		sf::Packet packet;
		packet << msgToSend;

		NetworkingManager* netManager;
//
//#if _SERVER
//		netManager = &NetworkingManager::GetNetworkingManager<NetworkingManagerServer>();
//#elif _CLIENT
//		netManager = &NetworkingManager::GetNetworkingManager<NetworkingManagerClient>();
//#endif
//		netManager->AddPacketToSend(packet);
		root.clear();
	}
}

void Player::SetPosition(sf::Vector2f newPosition)
{
	playerSprite.setPosition(newPosition);
	Json::Value floatArray; 
	floatArray.append(newPosition.x);
	floatArray.append(newPosition.y);

	root["playerPosition"] = floatArray;
}

void Player::SetRotation(float angle)
{
	forwardVector = ApplicationHelper::rotateVector(sf::Vector2f(0.0f, 1.0f), angle);
	playerSprite.setRotation(angle);
	root["playerAngle"] = angle;
}

void Player::PrintPressed()
{
	std::cout << "On pressed\n";
}

void Player::PrintReleased()
{
	std::cout << "On released\n";
}

void Player::SendTestPacket()
{
	sf::Packet packet;
	std::string msgToSend = "Bon dia";
	packet << msgToSend;

	//NetworkingManager* netManager = &NetworkingManager::GetNetworkingManager<NetworkingManagerClient>();
	//netManager->AddPacketToSend(packet);
}

void Player::AccelerateBoat()
{
	std::cout << "Accelerating Boat\n";
	//sf::Vector2f position = ApplicationHelper::Normalize(position);
	sf::Vector2f position = playerSprite.getPosition();

	speed = 500 * ApplicationHelper::GetDeltaTime();

	position += forwardVector * speed;

	//position.y -= 100 * ApplicationHelper::GetDeltaTime();
	SetPosition(position);
}

void Player::DecelerateBoat()
{
	std::cout << "Accelerating Boat\n";
	//sf::Vector2f position = ApplicationHelper::Normalize(position);
	sf::Vector2f position = playerSprite.getPosition();
	position.y += 100 * ApplicationHelper::GetDeltaTime();
	playerSprite.setPosition(position);
}

void Player::RotateBoatLeft()
{
	float tickRotation = -angleBoatSpeedEachSecond * ApplicationHelper::GetDeltaTime();
	float previousRotation = GetRotation();
	float newAngleRotation = previousRotation + tickRotation;
	SetRotation(newAngleRotation);
}

void Player::RotateBoatRight()
{
	float tickRotation = angleBoatSpeedEachSecond * ApplicationHelper::GetDeltaTime();
	float previousRotation = GetRotation();
	float newAngleRotation = previousRotation + tickRotation;
	SetRotation(newAngleRotation );
}

void Player::UpdaetPlayerInfo(const std::string& NetworkData)
{
	if (!IsPlayerPlayable)
	{
		//Extract the data and update player info
		Json::Value root;
		Json::Reader reader;
		bool parsingSuccessful = reader.parse(NetworkData, root);
		Json::Value coordinatesArray = root["playerPosition"];
		float a = coordinatesArray[0].asFloat();
		float b = coordinatesArray[1].asFloat();
		float angle = root["playerAngle"].asFloat();
		SetPosition(sf::Vector2f(a,b));
		SetRotation(angle);
	}
}

void Player::PrintRndomMessage()
{
	std::cout << "Print random mensaje brou\n";
}


InputAction::InputAction()
{

}

InputAction::InputAction(sf::Keyboard::Key key)
{
	this->key = key;
}

void InputAction::SetKey(sf::Keyboard::Key key)
{
	this->key = key;
}

void InputAction::SetIsKeyPressed(bool keyPressed)
{
	bIsKeyPressed = keyPressed;
}
