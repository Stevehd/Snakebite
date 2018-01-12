//
//  NetworkTransport.h
//  Snakebite
//
//  Created by Stephen Harrison-Daly on 16/02/2017.
//  Copyright © 2017 Stephen Harrison-Daly. All rights reserved.
//

#pragma once

#include <steam_api.h>
#include <stdint.h>

namespace shd
{
	class NetworkTransport
	{
	public:

		// The size of the receive buffer
		static const int NET_TRANSPORT_SEND_BUFF_SIZE = 16 * 1024;
		static const int NET_TRANSPORT_RECV_BUFF_SIZE = 4 * 1024;
		static const int NET_TRANSPORT_FRAGMENT_DATA_MAX_SIZE = 1024;
		static const int NET_TRANSPORT_MAX_FRAGMENTS = 4;

		struct NetPackedBall
		{
			float posX;
			float posY;
			float velocityX;
			float velocityY;
		};

		NetworkTransport();
		bool sendStartGameHandshake();
		bool recvStartGameHandshake();
		bool sendHandshakeAck();
		bool recvHandshakeAck();
		bool sendSpecialEvents();
		bool sendData();
		bool receiveData();
		inline int64_t getTotalBytesSent() { return m_bytesSent; }
		inline int64_t getTotalBytesReceived() { return m_bytesReceived; }
		void resetSendReceiveCounters();
		void reset();

	private:

		enum MessageType
		{
			MESSAGE_TYPE_NOT_SET = 0,
			MESSAGE_TYPE_START_GAME_HANDSHAKE,
			MESSAGE_TYPE_START_GAME_ACK,
			MESSAGE_TYPE_GAME_PACKET_STANDARD,
			MESSAGE_TYPE_GAME_PACKET_FULL_STATE_UPDATE,
			MESSAGE_TYPE_GAME_PACKET_FRAGMENT,
			MESSAGE_TYPE_GAME_EVENT_GOAL_TOP,
			MESSAGE_TYPE_GAME_EVENT_GOAL_BOTTOM,
			MESSAGE_TYPE_GAME_EVENT_WEAPON_THROW,
			MESSAGE_TYPE_GAME_EVENT_HALFTIME,
			MESSAGE_TYPE_GAME_EVENT_REMATCH,
			MESSAGE_TYPE_GAME_EVENT_REVIVE_HOME,
			MESSAGE_TYPE_GAME_EVENT_REVIVE_AWAY,
			MESSAGE_TYPE_MAX
		};

		struct MessageHeader
		{
			uint8_t		messageType;				// What type of message is this
			uint8_t		fragmentDetails;			// If the packet is fragment, first 4 bytes are number of fragments. Second 4 bytes is the fragment index
			uint16_t	packetSequenceNum;			// What number is this in the packet sequence
			uint16_t	lastInputSequenceReceived;	// Used to ACK the last input that was received
		};

		struct GameStartHandshake
		{
			uint32_t	verification;			// Some prefixed value that can be used for verification
			uint32_t	gameType;				// TODO - some game type data
			uint8_t		teamColourPrimary;		// The clients primary team colour
			uint8_t		teamColourSecondary;	// The clients primary team colour
		};

		struct GameStartHandshakeAck
		{
			uint8_t		teamColourPrimary;		// The clients primary team colour
			uint8_t		teamColourSecondary;	// The clients secondary team colour
		};

		// Callback used when initial connection is made. Asks if we should accept a connection with the other user
		STEAM_CALLBACK(NetworkTransport, onP2PSessionRequest, P2PSessionRequest_t, m_callbackP2PSessionRequest);

		// Callback used when a connection error occurs
		STEAM_CALLBACK(NetworkTransport, onP2PSessionConnectFail, P2PSessionConnectFail_t, m_callbackP2PSessionConnectFail);

		// The send buffer
		char m_sendBuffer[NET_TRANSPORT_SEND_BUFF_SIZE];

		// The receive buffer
		char m_recvBuffer[NET_TRANSPORT_RECV_BUFF_SIZE];

		// A counter for the number of packets sent
		uint16_t m_packetsSent;

		// The last packet number received. Drop a packet if we received a newer one
		volatile uint32_t m_lastPacketNumReceived;

		// Number of bytes sent and received
		volatile int64_t m_bytesSent;
		volatile int64_t m_bytesReceived;
	};
}