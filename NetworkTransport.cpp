//
//  NetworkTransport.cpp
//  Snakebite
//
//  Created by Stephen Harrison-Daly on 16/02/2017.
//  Copyright © 2017 Stephen Harrison-Daly. All rights reserved.
//

#include "NetworkTransport.h"
#include "Application.h"

#define SHD_HANDSHAKE_VERIFICATION 0x19881337

using namespace shd;

NetworkTransport::NetworkTransport() :	m_callbackP2PSessionRequest(this, &NetworkTransport::onP2PSessionRequest),
										m_callbackP2PSessionConnectFail(this, &NetworkTransport::onP2PSessionConnectFail),
										m_packetsSent(0),
										m_lastPacketNumReceived(0),
										m_bytesReceived(0),
										m_bytesSent(0)
{

}

bool NetworkTransport::sendStartGameHandshake()
{
	// First, reset the send buffer
	memset(m_sendBuffer, 0, NET_TRANSPORT_SEND_BUFF_SIZE);

	bool ret;
	size_t bytesToSend = 0;
	MessageHeader * msgHeader = nullptr;
	GameStartHandshake * msgBody = nullptr;
	
	// Set the positions in the send buffer or the header and the body
	msgHeader = (MessageHeader *)m_sendBuffer;
	bytesToSend += sizeof(MessageHeader);

	msgBody = (GameStartHandshake *)(m_sendBuffer + bytesToSend);
	bytesToSend += sizeof(GameStartHandshake);

	msgHeader->messageType = MESSAGE_TYPE_START_GAME_HANDSHAKE;
	msgBody->verification = SHD_HANDSHAKE_VERIFICATION;
	msgBody->gameType = 0; // TODO
	msgBody->teamColourPrimary = Application::getInstance().globalSettings.teamColourPrimary;
	msgBody->teamColourSecondary = Application::getInstance().globalSettings.teamColourSecondary;

	if (bytesToSend > NET_TRANSPORT_SEND_BUFF_SIZE)
	{
		SHD_ASSERT(false);
		return false;
	}

	ret = SteamNetworking()->SendP2PPacket(	Application::getInstance().networkThread.getLobby().getOpponentID(),
											m_sendBuffer,
											bytesToSend,
											k_EP2PSendReliable);
	if (ret == false)
	{
		SHD_PRINTF("SendP2PPacket failed.\n");
	}

	return ret;
}

bool NetworkTransport::recvStartGameHandshake()
{
	bool ret = false;
	uint32_t msgSize = 0;
	uint32_t bytesRead = 0;
	CSteamID steamIDRemote;
	MessageHeader * msgHeader = nullptr;
	GameStartHandshake * msgBody = nullptr;

	while (SteamNetworking()->IsP2PPacketAvailable(&msgSize))
	{
		SHD_PRINTF("Received a packet of size: %d!\n", msgSize);

		if (msgSize > NET_TRANSPORT_RECV_BUFF_SIZE)
		{
			SHD_PRINTF("Receive buffer too small!\n");
			SHD_ASSERT(false);
			return false;
		}

		memset(m_recvBuffer, 0, NET_TRANSPORT_RECV_BUFF_SIZE);

		if (SteamNetworking()->ReadP2PPacket(m_recvBuffer, msgSize, &bytesRead, &steamIDRemote))
		{
			msgHeader = (MessageHeader *)m_recvBuffer;
			msgBody = (GameStartHandshake *)(m_recvBuffer + sizeof(MessageHeader));

			SHD_PRINTF("Received handshake from other guy\n");

			if (msgHeader->messageType == MESSAGE_TYPE_START_GAME_HANDSHAKE && msgBody->verification == SHD_HANDSHAKE_VERIFICATION)
			{
				ret = true;
				SHD_PRINTF("And it's all good. Message type is %d\n", msgBody->gameType);

				if (Application::getInstance().globalSettings.teamColourPrimary == (Team::TeamColour)msgBody->teamColourPrimary)
				{
					Application::getInstance().globalSettings.teamColourPrimaryOnline = Application::getInstance().globalSettings.teamColourPrimary;
					Application::getInstance().globalSettings.teamColourSecondaryOnline = Application::getInstance().globalSettings.teamColourSecondary;

				}
				else
				{
					Application::getInstance().globalSettings.teamColourPrimaryOnline = (Team::TeamColour)msgBody->teamColourPrimary;
					Application::getInstance().globalSettings.teamColourSecondaryOnline = Application::getInstance().globalSettings.teamColourPrimary;
				}
			}
			else
			{
				SHD_PRINTF("The message wasn't what we expected! Error!\n");
				SHD_ASSERT(false);
			}
		}
	}

	return ret;
}

bool NetworkTransport::sendHandshakeAck()
{
	// First, reset the send buffer
	memset(m_sendBuffer, 0, NET_TRANSPORT_SEND_BUFF_SIZE);

	bool ret = false;
	size_t bytesToSend = 0;
	MessageHeader * msgHeader = nullptr;
	GameStartHandshakeAck * msgBody = nullptr;

	msgHeader = (MessageHeader *)m_sendBuffer;
	bytesToSend += sizeof(MessageHeader);
	
	msgBody = (GameStartHandshakeAck *)(m_sendBuffer + bytesToSend);
	bytesToSend += sizeof(GameStartHandshakeAck);

	msgHeader->messageType = MESSAGE_TYPE_START_GAME_ACK;
	msgBody->teamColourPrimary = Application::getInstance().globalSettings.teamColourPrimary;
	msgBody->teamColourSecondary = Application::getInstance().globalSettings.teamColourSecondary;

	ret = SteamNetworking()->SendP2PPacket(	Application::getInstance().networkThread.getLobby().getOpponentID(),
											m_sendBuffer,
											bytesToSend,
											k_EP2PSendReliable);
	if (ret == false)
	{
		SHD_PRINTF("SendP2PPacket in sendHandshakeAck() failed.\n");
	}

	return ret;
}

bool NetworkTransport::recvHandshakeAck()
{
	bool ret = false;
	uint32_t msgSize = 0;
	uint32_t bytesRead = 0;
	CSteamID steamIDRemote;
	MessageHeader * msgHeader = nullptr;
	GameStartHandshakeAck * msgBody = nullptr;

	while (SteamNetworking()->IsP2PPacketAvailable(&msgSize))
	{
		SHD_PRINTF("Received a packet of size: %d!\n", msgSize);

		if (msgSize > NET_TRANSPORT_RECV_BUFF_SIZE)
		{
			SHD_PRINTF("Receive buffer too small!\n");
			SHD_ASSERT(false);
			return false;
		}

		memset(m_recvBuffer, 0, NET_TRANSPORT_RECV_BUFF_SIZE);

		if (SteamNetworking()->ReadP2PPacket(m_recvBuffer, msgSize, &bytesRead, &steamIDRemote))
		{
			msgHeader = (MessageHeader *)m_recvBuffer;
			msgBody = (GameStartHandshakeAck *)(m_recvBuffer + sizeof(MessageHeader));

			SHD_PRINTF("Received ACK from other guy\n");

			if (msgHeader->messageType == MESSAGE_TYPE_START_GAME_ACK)
			{
				SHD_PRINTF("And it's all good.\n");
				ret = true;

				if (Application::getInstance().globalSettings.teamColourPrimary == (Team::TeamColour)msgBody->teamColourPrimary)
				{
					Application::getInstance().globalSettings.teamColourSecondary = (Team::TeamColour)msgBody->teamColourSecondary;
				}
				else
				{
					Application::getInstance().globalSettings.teamColourSecondary = (Team::TeamColour)msgBody->teamColourPrimary;
				}
			}
			else
			{
				SHD_PRINTF("The message wasn't what we expected! Error!\n");
			}
		}
	}

	return ret;
}

bool NetworkTransport::sendSpecialEvents()
{
	bool ret = false;
	size_t bytesToSend = 0;
	MessageHeader * msgHeader = nullptr;
	uint8_t * msgBody = nullptr;
	NetworkInputBuffer::SpecialEventData specialEvent;

	bool hasFullStateUpdate = false;

	// Set the positions in the send buffer or the header and the body
	msgHeader = (MessageHeader *)m_sendBuffer;
	msgBody = (uint8_t *)(m_sendBuffer + sizeof(MessageHeader));

	while (Application::getInstance().networkThread.getNetInputBuffer()->popSpecialEvent(&specialEvent))
	{
		bytesToSend = 0;
		memset(m_sendBuffer, 0, NET_TRANSPORT_SEND_BUFF_SIZE);

		bytesToSend += sizeof(MessageHeader);
		SHD_PRINTF("Sending special event: %i\n", specialEvent.type);

		// Pack extra data in the fragmentDetails member of the header
		switch (specialEvent.type)
		{
		case NetworkInputBuffer::NET_SPECIAL_EVENT_GOAL_TOP:
			msgHeader->messageType = MESSAGE_TYPE_GAME_EVENT_GOAL_TOP;
			msgHeader->fragmentDetails = specialEvent.nextTeamToKickoff;
			break;

		case NetworkInputBuffer::NET_SPECIAL_EVENT_GOAL_BOTTOM:
			msgHeader->messageType = MESSAGE_TYPE_GAME_EVENT_GOAL_BOTTOM;
			msgHeader->fragmentDetails = specialEvent.nextTeamToKickoff;
			break;

		case NetworkInputBuffer::NET_SPECIAL_EVENT_WEAPON_THROW:
			msgHeader->messageType = MESSAGE_TYPE_GAME_EVENT_WEAPON_THROW;
			((NetworkInputBuffer::PackedThrownWeapon *)msgBody)->index = specialEvent.weaponIndex;
			((NetworkInputBuffer::PackedThrownWeapon *)msgBody)->posX = specialEvent.weaponPos.x * 50;
			((NetworkInputBuffer::PackedThrownWeapon *)msgBody)->posY = specialEvent.weaponPos.y * 50;
			((NetworkInputBuffer::PackedThrownWeapon *)msgBody)->velocityX = specialEvent.weaponVel.x * 100;
			((NetworkInputBuffer::PackedThrownWeapon *)msgBody)->velocityY = specialEvent.weaponVel.y * 100;
			bytesToSend += sizeof(NetworkInputBuffer::PackedThrownWeapon);

			break;

		case NetworkInputBuffer::NET_SPECIAL_EVENT_HALFTIME:
			msgHeader->messageType = MESSAGE_TYPE_GAME_EVENT_HALFTIME;
			break;

		case NetworkInputBuffer::NET_SPECIAL_EVENT_REMATCH:
			msgHeader->messageType = MESSAGE_TYPE_GAME_EVENT_REMATCH;
			break;

		case NetworkInputBuffer::NET_SPECIAL_EVENT_REVIVE_HOME:
			msgHeader->messageType = MESSAGE_TYPE_GAME_EVENT_REVIVE_HOME;
			msgHeader->fragmentDetails = specialEvent.reviveHome;
			break;

		case NetworkInputBuffer::NET_SPECIAL_EVENT_REVIVE_AWAY:
			msgHeader->messageType = MESSAGE_TYPE_GAME_EVENT_REVIVE_AWAY;
			msgHeader->fragmentDetails = specialEvent.reviveAway;
			break;

		default:
			SHD_ASSERT(false);
			break;
		}

		ret = SteamNetworking()->SendP2PPacket(Application::getInstance().networkThread.getLobby().getOpponentID(),
			m_sendBuffer,
			bytesToSend,
			k_EP2PSendReliable);
		if (ret == false)
		{
			SHD_PRINTF("SendP2PPacket in sendSpecialEvents() failed.\n");
		}
	}

	return ret;
}

bool NetworkTransport::sendData()
{
	sendSpecialEvents();

	// First, reset the send buffer
	memset(m_sendBuffer, 0, NET_TRANSPORT_SEND_BUFF_SIZE);

	bool ret;
	size_t bytesWritten = 0;
	size_t bytesToSend = 0;
	MessageHeader * msgHeader = nullptr;
	uint8_t * msgBody = nullptr;
	bool hasFullStateUpdate = false;
	
	// Set the positions in the send buffer or the header and the body
	msgHeader = (MessageHeader *)m_sendBuffer;
	bytesToSend += sizeof(MessageHeader);
	msgBody = (uint8_t *)(m_sendBuffer + bytesToSend);

	Application::getInstance().networkThread.getNetInputBuffer()->fillSendBuffer(msgBody, NET_TRANSPORT_SEND_BUFF_SIZE - bytesToSend, &bytesWritten, &hasFullStateUpdate);
	if (bytesWritten == 0)
	{
		// Nothing to send!
		return false;
	}

	bytesToSend += bytesWritten;
	msgHeader->packetSequenceNum = m_packetsSent++;
	msgHeader->lastInputSequenceReceived = Application::getInstance().networkThread.getNetInputBuffer()->getLastReceivedSeqNum();
	msgHeader->messageType = (hasFullStateUpdate) ? MESSAGE_TYPE_GAME_PACKET_FULL_STATE_UPDATE : MESSAGE_TYPE_GAME_PACKET_STANDARD;

	// Also check if bytesToSend is greater than 1200 bytes - the Steam UDP max send size
	if (bytesToSend > NET_TRANSPORT_SEND_BUFF_SIZE || bytesToSend > 1200)
	{
		SHD_ASSERT(false);
		return false;
	}

	ret = SteamNetworking()->SendP2PPacket(	Application::getInstance().networkThread.getLobby().getOpponentID(),
											m_sendBuffer,
											bytesToSend,
											k_EP2PSendUnreliable);
	if (ret == false)
	{
		SHD_PRINTF("SendP2PPacket failed.\n");
	}

	shd::Atomic::add64(&m_bytesSent, bytesToSend);

	return ret;
}

bool NetworkTransport::receiveData()
{
	bool ret = false;
	uint32_t msgSize = 0;
	uint32_t bytesRead = 0;
	CSteamID steamIDRemote;
	MessageHeader * msgHeader = nullptr;
	void * msgBody = nullptr;

	while (SteamNetworking()->IsP2PPacketAvailable(&msgSize))
	{
		if (msgSize > NET_TRANSPORT_RECV_BUFF_SIZE)
		{
			SHD_PRINTF("Receive buffer too small!\n");
			SHD_ASSERT(false);
			return false;
		}

		shd::Atomic::add64(&m_bytesReceived, msgSize);
		memset(m_recvBuffer, 0, NET_TRANSPORT_RECV_BUFF_SIZE);

		if (SteamNetworking()->ReadP2PPacket(m_recvBuffer, msgSize, &bytesRead, &steamIDRemote))
		{
			// If the message was received from someone who isn't our opponent, ignore it
			if (steamIDRemote != Application::getInstance().networkThread.getLobby().getOpponentID())
			{
				continue;
			}

			msgHeader = (MessageHeader *)m_recvBuffer;
			msgBody = (void *)(m_recvBuffer + sizeof(MessageHeader));


			switch (msgHeader->messageType)
			{
			case MESSAGE_TYPE_START_GAME_HANDSHAKE:
				break;
			case MESSAGE_TYPE_START_GAME_ACK:
				break;
			case MESSAGE_TYPE_GAME_PACKET_STANDARD:
			case MESSAGE_TYPE_GAME_PACKET_FULL_STATE_UPDATE:

				// If there was a gap in received packets (maybe the counter wrapped around), reset it
				if ((int)m_lastPacketNumReceived - (int)msgHeader->packetSequenceNum > 60 ||
					(int)m_lastPacketNumReceived - (int)msgHeader->packetSequenceNum < -60)
				{
					SHD_PRINTF("Reset m_lastPacketNumReceived\n");
					m_lastPacketNumReceived = msgHeader->packetSequenceNum;
				}

				if (msgHeader->packetSequenceNum < m_lastPacketNumReceived)
				{
					SHD_PRINTF("Received out of order packet: %d, last was: %i\n", msgHeader->packetSequenceNum, m_lastPacketNumReceived);
					continue;
				}

				ret = true;
				m_lastPacketNumReceived = msgHeader->packetSequenceNum;
				
				if (msgHeader->messageType == MESSAGE_TYPE_GAME_PACKET_STANDARD)
					Application::getInstance().networkThread.getNetInputBuffer()->parseRecvBuffer(msgBody, false);
				else
					Application::getInstance().networkThread.getNetInputBuffer()->parseRecvBuffer(msgBody, true);

				break;
			case MESSAGE_TYPE_GAME_PACKET_FRAGMENT:
				break;
			case MESSAGE_TYPE_GAME_EVENT_GOAL_TOP:
				shd::Atomic::exchange32(&Application::getInstance().matchState.nextTeamToKickoff, msgHeader->fragmentDetails);
				shd::Atomic::exchange32(&Application::getInstance().networkThread.specialEvents.goalTop, 1);
				break;
			case MESSAGE_TYPE_GAME_EVENT_GOAL_BOTTOM:
				shd::Atomic::exchange32(&Application::getInstance().matchState.nextTeamToKickoff, msgHeader->fragmentDetails);
				shd::Atomic::exchange32(&Application::getInstance().networkThread.specialEvents.goalBottom, 1);
				break;
			case MESSAGE_TYPE_GAME_EVENT_WEAPON_THROW:
			{
				NetworkInputBuffer::PackedThrownWeapon * weapon = (NetworkInputBuffer::PackedThrownWeapon *)msgBody;

				if (weapon->index < 0 || weapon->index >= WeaponManager::MAX_WEAPONS)
				{
					break;
				}
				
				Application::getInstance().networkThread.packedWeapons[weapon->index] = *weapon;
				shd::Atomic::exchange32(&Application::getInstance().networkThread.specialEvents.weaponThrown[weapon->index], 1);
				
				break;
			}
			case MESSAGE_TYPE_GAME_EVENT_HALFTIME:
				break;
			case MESSAGE_TYPE_GAME_EVENT_REMATCH:
				Application::getInstance().networkThread.setOpponentWantsRematch(true);
				break;
			case MESSAGE_TYPE_GAME_EVENT_REVIVE_HOME:
				shd::Atomic::exchange32(&Application::getInstance().matchState.numPlayersRevived, msgHeader->fragmentDetails);
				shd::Atomic::exchange32(&Application::getInstance().networkThread.specialEvents.reviveHome, 1);
				break;
			case MESSAGE_TYPE_GAME_EVENT_REVIVE_AWAY:
				shd::Atomic::exchange32(&Application::getInstance().matchState.numPlayersRevived, msgHeader->fragmentDetails);
				shd::Atomic::exchange32(&Application::getInstance().networkThread.specialEvents.reviveAway, 1);
				break;
			default:
				SHD_ASSERT(false);
				break;
			}
		}
	}

	return ret;
}

void NetworkTransport::resetSendReceiveCounters()
{
	shd::Atomic::exchange64(&m_bytesSent, 0);
	shd::Atomic::exchange64(&m_bytesReceived, 0);
}

void NetworkTransport::reset()
{
	shd::Atomic::exchange32(&m_lastPacketNumReceived, 0);
}

//-----------------------------------------------------------------------------
// Purpose: another user has sent us a packet - do we accept?
//-----------------------------------------------------------------------------
void NetworkTransport::onP2PSessionRequest(P2PSessionRequest_t *pP2PSessionRequest)
{
	SHD_PRINTF("A user wants to create a session with us...\n");

	if (Application::getInstance().networkThread.getLobby().getOpponentID() == pP2PSessionRequest->m_steamIDRemote)
	{
		SteamNetworking()->AcceptP2PSessionWithUser(pP2PSessionRequest->m_steamIDRemote);
		SHD_PRINTF("The user was the other member of the lobby. Accepting communication.\n");
	}
	else
	{
		SHD_PRINTF("The requesting user is not in a lobby with us... Who is it? Denied.\n");
	}
}

//-----------------------------------------------------------------------------
// Purpose: We sent a packet to another user but it failed
//-----------------------------------------------------------------------------
void NetworkTransport::onP2PSessionConnectFail(P2PSessionConnectFail_t *pP2PSessionConnectFail)
{
	(void)pP2PSessionConnectFail;

	// we've sent a packet to the user, but it never got through
	// we can just use the normal timeout
	SHD_PRINTF("Failed to send a packet to the other user.\n");
}