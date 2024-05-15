#include <QtEndian>
#include <QThread>
#include "logger.h"
#include "zboss.h"

static uint8_t const crc8Table[256] = {
    0xea, 0xd4, 0x96, 0xa8, 0x12, 0x2c, 0x6e, 0x50, 0x7f, 0x41, 0x03, 0x3d,
    0x87, 0xb9, 0xfb, 0xc5, 0xa5, 0x9b, 0xd9, 0xe7, 0x5d, 0x63, 0x21, 0x1f,
    0x30, 0x0e, 0x4c, 0x72, 0xc8, 0xf6, 0xb4, 0x8a, 0x74, 0x4a, 0x08, 0x36,
    0x8c, 0xb2, 0xf0, 0xce, 0xe1, 0xdf, 0x9d, 0xa3, 0x19, 0x27, 0x65, 0x5b,
    0x3b, 0x05, 0x47, 0x79, 0xc3, 0xfd, 0xbf, 0x81, 0xae, 0x90, 0xd2, 0xec,
    0x56, 0x68, 0x2a, 0x14, 0xb3, 0x8d, 0xcf, 0xf1, 0x4b, 0x75, 0x37, 0x09,
    0x26, 0x18, 0x5a, 0x64, 0xde, 0xe0, 0xa2, 0x9c, 0xfc, 0xc2, 0x80, 0xbe,
    0x04, 0x3a, 0x78, 0x46, 0x69, 0x57, 0x15, 0x2b, 0x91, 0xaf, 0xed, 0xd3,
    0x2d, 0x13, 0x51, 0x6f, 0xd5, 0xeb, 0xa9, 0x97, 0xb8, 0x86, 0xc4, 0xfa,
    0x40, 0x7e, 0x3c, 0x02, 0x62, 0x5c, 0x1e, 0x20, 0x9a, 0xa4, 0xe6, 0xd8,
    0xf7, 0xc9, 0x8b, 0xb5, 0x0f, 0x31, 0x73, 0x4d, 0x58, 0x66, 0x24, 0x1a,
    0xa0, 0x9e, 0xdc, 0xe2, 0xcd, 0xf3, 0xb1, 0x8f, 0x35, 0x0b, 0x49, 0x77,
    0x17, 0x29, 0x6b, 0x55, 0xef, 0xd1, 0x93, 0xad, 0x82, 0xbc, 0xfe, 0xc0,
    0x7a, 0x44, 0x06, 0x38, 0xc6, 0xf8, 0xba, 0x84, 0x3e, 0x00, 0x42, 0x7c,
    0x53, 0x6d, 0x2f, 0x11, 0xab, 0x95, 0xd7, 0xe9, 0x89, 0xb7, 0xf5, 0xcb,
    0x71, 0x4f, 0x0d, 0x33, 0x1c, 0x22, 0x60, 0x5e, 0xe4, 0xda, 0x98, 0xa6,
    0x01, 0x3f, 0x7d, 0x43, 0xf9, 0xc7, 0x85, 0xbb, 0x94, 0xaa, 0xe8, 0xd6,
    0x6c, 0x52, 0x10, 0x2e, 0x4e, 0x70, 0x32, 0x0c, 0xb6, 0x88, 0xca, 0xf4,
    0xdb, 0xe5, 0xa7, 0x99, 0x23, 0x1d, 0x5f, 0x61, 0x9f, 0xa1, 0xe3, 0xdd,
    0x67, 0x59, 0x1b, 0x25, 0x0a, 0x34, 0x76, 0x48, 0xf2, 0xcc, 0x8e, 0xb0,
    0xd0, 0xee, 0xac, 0x92, 0x28, 0x16, 0x54, 0x6a, 0x45, 0x7b, 0x39, 0x07,
    0xbd, 0x83, 0xc1, 0xff};

static uint16_t const crc16Table[256] = {
	0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
	0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
	0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
	0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
	0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
	0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
	0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
	0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
	0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
	0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
	0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
	0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
	0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
	0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
	0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
	0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
	0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
	0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
	0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
	0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
	0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
	0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
	0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
	0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
	0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
	0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
	0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
	0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
	0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
	0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
	0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
	0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78};

ZBoss::ZBoss(QSettings *config, QObject *parent) : Adapter(config, parent)
{
    m_packet_seq = 0;
    m_tsn = 0;
    m_ack_seq = -1;

    m_networkKey = QByteArray::fromHex(config->value("security/key", "000102030405060708090a0b0c0d0e0f").toString().remove("0x").toUtf8());
    
    m_policy.append({POLICY_TC_LINK_KEYS_REQUIRED,           static_cast <char> (0x00)});
    m_policy.append({POLICY_IC_REQUIRED,                     static_cast <char> (0x00)});
    m_policy.append({POLICY_TC_REJOIN_ENABLED,               0x01});
    m_policy.append({POLICY_IGNORE_TC_REJOIN,                static_cast <char> (0x00)});
    m_policy.append({POLICY_APS_INSECURE_JOIN,               static_cast <char> (0x00)});
    m_policy.append({POLICY_DISABLE_NWK_MGMT_CHANNEL_UPDATE, static_cast <char> (0x00)});
}

bool ZBoss::unicastRequest(quint8, quint16 networkAddress, quint8 srcEndPointId, quint8 dstEndPointId, quint16 clusterId, const QByteArray &payload)
{
    apsdeDataRequestStruct request;
    QByteArray buffer;
    
    buffer.append(reinterpret_cast <const char*> (&networkAddress), sizeof(networkAddress));
    buffer.append(6, 0x00);
    memcpy(&request.ieeeAddress, buffer.constData(), sizeof(request.ieeeAddress));

    request.paramLength = 0x15;
    request.dataLength = payload.length();
    request.profileId = qToBigEndian <quint16> (m_endpoints.contains(srcEndPointId) ? m_endpoints.value(srcEndPointId)->profileId() : 0x0000);
    request.clusterId = clusterId;
    request.dstEndpointId = dstEndPointId;
    request.srcEndpointId = srcEndPointId;
    request.radius = 0x00;
    request.dstMode = ADDRESS_MODE_16_BIT;
    request.txMode = 0x00;
    request.alias = 0x00;
    request.srcAlias = 0x0000;
    request.aliasSeq = 0x00;

    return sendRequest(APSDE_DATA_REQ, QByteArray(reinterpret_cast <char*> (&request), sizeof(request)).append(payload)) && !m_replyStatus;
}

bool ZBoss::multicastRequest(quint8, quint16 groupId, quint8 srcEndPointId, quint8 dstEndPointId, quint16 clusterId, const QByteArray &payload)
{
    apsdeDataRequestStruct request;
    QByteArray buffer;
    
    buffer.append(reinterpret_cast <const char*> (&groupId), sizeof(groupId));
    buffer.append(6, 0x00);
    memcpy(&request.ieeeAddress, buffer.constData(), sizeof(request.ieeeAddress));

    request.paramLength = 0x15;
    request.dataLength = payload.length();
    request.profileId = qToBigEndian <quint16> (m_endpoints.contains(srcEndPointId) ? m_endpoints.value(srcEndPointId)->profileId() : 0x0000);
    request.clusterId = clusterId;
    request.dstEndpointId = dstEndPointId;
    request.srcEndpointId = srcEndPointId;
    request.radius = 0x00;
    request.dstMode = ADDRESS_MODE_GROUP;
    request.txMode = 0x00;
    request.alias = 0x00;
    request.srcAlias = 0x0000;
    request.aliasSeq = 0x00;

    return sendRequest(APSDE_DATA_REQ, QByteArray(reinterpret_cast <char*> (&request), sizeof(request)).append(payload)) && !m_replyStatus;
}

bool ZBoss::unicastInterPanRequest(quint8, const QByteArray &, quint16 , const QByteArray &)
{
    return false;
}

bool ZBoss::broadcastInterPanRequest(quint8, quint16, const QByteArray &)
{
    return false;
}

bool ZBoss::setInterPanChannel(quint8)
{
    return false;
}

void ZBoss::resetInterPanChannel(void)
{

}

bool ZBoss::zdoRequest(quint8, quint16 networkAddress, quint16 clusterId, const QByteArray &data)
{
    quint16 command;

    switch (clusterId)
    {
        case ZDO_NODE_DESCRIPTOR_REQUEST:   command = ZBOSS_ZDO_NODE_DESC_REQ; break;
        case ZDO_SIMPLE_DESCRIPTOR_REQUEST: command = ZBOSS_ZDO_SIMPLE_DESC_REQ; break;
        case ZDO_ACTIVE_ENDPOINTS_REQUEST:  command = ZBOSS_ZDO_ACTIVE_EP_REQ; break;
        default: return false;
    }

    return sendRequest(command, QByteArray(reinterpret_cast <char*> (&networkAddress), sizeof(networkAddress)).append(data)) && !m_replyStatus;
}

bool ZBoss::bindRequest(quint8, quint16 networkAddress, quint8 endpointId, quint16 clusterId, const QByteArray &address, quint8 dstEndpointId, bool unbind)
{
    QByteArray buffer = address.isEmpty() ? m_ieeeAddress : address;
    zdoBindRequestStruct request;

    memcpy(&request.srcAddress, m_requestAddress.constData(), sizeof(request.srcAddress));
    memcpy(&request.dstAddress, buffer.constData(), sizeof(request.dstAddress));

    request.networkAddress = networkAddress;
    request.srcEndpointId = endpointId;
    request.clusterId = clusterId;
    request.dstMode = buffer.length() == 2 ? ADDRESS_MODE_GROUP : ADDRESS_MODE_64_BIT;
    
    if (request.dstMode == ADDRESS_MODE_64_BIT)
        request.dstEndpointId = dstEndpointId;
    else
        request.dstEndpointId = 0x00;

    return sendRequest(unbind ? ZBOSS_ZDO_UNBIND_REQ : ZBOSS_ZDO_BIND_REQ, QByteArray(reinterpret_cast <char*> (&request), sizeof(request))) && !m_replyStatus;
}

bool ZBoss::leaveRequest(quint8, quint16 networkAddress)
{
    zdoLeaveRequestStruct request;

    request.dstAddress = 0x00;
    request.networkAddress = networkAddress;
    request.flags = 0x00;
    return sendRequest(ZBOSS_ZDO_MGMT_LEAVE_REQ, QByteArray(reinterpret_cast <char*> (&request), sizeof(request)));
}

bool ZBoss::lqiRequest(quint8, quint16 networkAddress, quint8 index)
{
    return sendRequest(ZBOSS_ZDO_MGMT_LQI_REQ, QByteArray(reinterpret_cast <char*> (&networkAddress), sizeof(networkAddress)).append(static_cast <quint8> (index))) && !m_replyStatus;
}

quint8 ZBoss::getCRC8(quint8 *data, quint32 length)
{
    quint8 crc = 0x00;
    
    if (data == NULL)
        return 0;
    quint8 const *end = data + length;
    while (data < end)
        crc = crc8Table[crc ^ *data++];
    return crc;
}

quint16 ZBoss::getCRC16(quint8 *data, quint32 length)
{
    quint16 crc = 0x0000;

    while (length--)
        crc = static_cast <quint16> (crc >> 8) ^ crc16Table[(crc ^ *data++) & 0x00FF];
    return crc;
}

bool ZBoss::sendRequest(quint16 command, const QByteArray &data)
{
    QByteArray request, crc_llheader, common;
    lowLeverHeaderStruct llheader;
    commonHeaderStruct commonHead;
    quint16 crc;

    commonHead.version = ZBOSS_PROTOCOL_VERSION;
    commonHead.type = REQUEST;
    commonHead.id = command;

    m_command = command;

    common = QByteArray(reinterpret_cast <char*> (&commonHead), sizeof(commonHead));
    common.append(getTSN());
    common.append(data);

    llheader.signature = qToBigEndian <quint16> (ZBOSS_SIGNATURE);
    llheader.length = common.length() + 7;
    llheader.type = ZBOSS_NCP_API_HL;
    llheader.flags = FIRST_FRAG | LAST_FRAG | m_packet_seq << 2;
    llheader.CRC = 0;

    m_packet_seq = (m_packet_seq % 3) + 1;

    crc_llheader.append(reinterpret_cast <char*> (&llheader.length), sizeof(llheader.length));
    crc_llheader.append(llheader.type);
    crc_llheader.append(llheader.flags);
    llheader.CRC = getCRC8(reinterpret_cast <quint8*> (crc_llheader.data()), crc_llheader.length());

    request = QByteArray(reinterpret_cast <char*> (&llheader), sizeof(llheader));

    if (m_adapterDebug)
        logInfo << "-->" << request.toHex(':');

    crc = getCRC16(reinterpret_cast <quint8*> (common.data()), common.length());
    request.append(reinterpret_cast <char*> (&crc), sizeof(crc));
    request.append(common);

    sendData(request);
    return waitForSignal(this, SIGNAL(dataReceived()), ZBOSS_REQUEST_TIMEOUT);
}

void ZBoss::sendAck()
{
    QByteArray request, crc_llheader;
    lowLeverHeaderStruct llheader;

    llheader.signature = qToBigEndian <quint16> (ZBOSS_SIGNATURE);
    llheader.length = 7;
    llheader.type = ZBOSS_NCP_API_HL;
    llheader.flags = IS_ACK | m_ack_seq << 4;
    llheader.CRC = 0;

    m_ack_seq = (m_ack_seq + 1) % 4;

    crc_llheader.append(reinterpret_cast <char*> (&llheader.length), sizeof(llheader.length));
    crc_llheader.append(llheader.type);
    crc_llheader.append(llheader.flags);
    llheader.CRC = getCRC8(reinterpret_cast <quint8*> (crc_llheader.data()), crc_llheader.length());

    request = QByteArray(reinterpret_cast <char*> (&llheader), sizeof(llheader));

    if (m_adapterDebug)
        logInfo << "-->" << request.toHex(':');

    sendData(request);
}

void ZBoss::parsePacket(quint8 type, quint16 command, const QByteArray &data)
{
    if (command == 0x00)
        return;

    if (m_adapterDebug)
        logInfo << "<--" << QString::asprintf("0x%04x", qFromBigEndian(command)) << data.toHex(':');

    switch (qFromBigEndian(command))
    {
        case APSDE_DATA_IND:
        {
            QByteArray byteArrayStruct = data.mid(0, sizeof(apsdeDataIndicatonStruct));
            const apsdeDataIndicatonStruct *message = reinterpret_cast <const apsdeDataIndicatonStruct*> (byteArrayStruct.constData());
            QByteArray payload = data.mid(sizeof(apsdeDataIndicatonStruct), message->dataLength);
            emit zclMessageReveived(message->srcNetworkAddress, message->srcEndpointId, message->clusterId, message->lqi, payload);
            break;
        }

        case ZBOSS_ZDO_NODE_DESC_REQ:
        {
            QByteArray byteArrayStruct = data.mid(3, sizeof(zdoNodeDescriptorResponseStruct));
            const zdoNodeDescriptorResponseStruct *message = reinterpret_cast <const zdoNodeDescriptorResponseStruct*> (byteArrayStruct.constData());
            QByteArray payload;
            quint8 logicalType = message->flags & 0x0003;
            quint8 apsFlags = message->flags & 0x0700;
            quint16 address = message->networkAddress;

            payload.append(1, 0x00);
            payload.append(reinterpret_cast <const char*> (&message->networkAddress), sizeof(message->networkAddress));
            payload.append(logicalType);
            payload.append(apsFlags);
            payload.append(message->macCpb);
            payload.append(reinterpret_cast <const char*> (&message->manufacturerCode), sizeof(message->manufacturerCode));
            payload.append(message->maxBufferSize);
            payload.append(reinterpret_cast <const char*> (&message->maxTransferSize), sizeof(message->maxTransferSize));
            payload.append(reinterpret_cast <const char*> (&message->serverMask), sizeof(message->serverMask));
            payload.append(reinterpret_cast <const char*> (&message->maxOutTransferSize), sizeof(message->maxOutTransferSize));
            payload.append(message->descriptorCapabilities);

            emit zdoMessageReveived(address, ZDO_NODE_DESCRIPTOR_REQUEST, payload);
            break;
        }

        case ZBOSS_ZDO_SIMPLE_DESC_REQ:
        {
            QByteArray byteArrayStruct = data.mid(3, sizeof(zdoSimpleDescriptorResponseStruct));
            const zdoSimpleDescriptorResponseStruct *message = reinterpret_cast <const zdoSimpleDescriptorResponseStruct*> (byteArrayStruct.constData());
            quint8 offset = sizeof(zdoSimpleDescriptorResponseStruct) + (2 * message->inpClusterCount) + (2 * message->outClusterCount);
            QByteArray buffer = data.mid(3 + offset, 2);
            QByteArray payload;
            quint16 networkAddress;
            memcpy(&networkAddress, buffer.constData(), sizeof(networkAddress));

            payload.append(1, 0x00);
            payload.append(reinterpret_cast <const char*> (&networkAddress), sizeof(networkAddress));
            payload.append(1, 0x00);
            payload.append(message->endpointId);
            payload.append(reinterpret_cast <const char*> (&message->profileId), sizeof(message->profileId));
            payload.append(reinterpret_cast <const char*> (&message->deviceId), sizeof(message->deviceId));
            payload.append(message->version);

            emit zdoMessageReveived(networkAddress, ZDO_SIMPLE_DESCRIPTOR_REQUEST, payload);
            break;
        }

        case ZBOSS_ZDO_ACTIVE_EP_REQ:
        {
            QByteArray buffer = data.mid(3, sizeof(quint8));
            quint8 count, offset;
            quint16 networkAddress;
            QByteArray payload;

            memcpy(&count, buffer.constData(), sizeof(count));
            offset = sizeof(count) + (1 * count);
            buffer = data.mid(3 + offset, 2);            
            memcpy(&networkAddress, buffer.constData(), sizeof(networkAddress));

            payload.append(static_cast <char> (0x00));
            payload.append(reinterpret_cast <const char*> (&networkAddress), sizeof(networkAddress));
            payload.append(count);
            for (int i = 0; i < count; i++)
                payload.append(data.mid(3 + sizeof(count) + i, 1));
            
            emit zdoMessageReveived(networkAddress, ZDO_ACTIVE_ENDPOINTS_REQUEST, payload);
            break;
        }

        case ZBOSS_ZDO_DEV_ANNCE_IND:
        {
            quint64 ieeeAddress;
            const deviceAnnounceIndicatonStruct *message = reinterpret_cast <const deviceAnnounceIndicatonStruct*> (data.constData());
            ieeeAddress = qToBigEndian <quint64> (message->ieeeAddress);
            emit deviceJoined(QByteArray(reinterpret_cast <char*> (&ieeeAddress), sizeof(ieeeAddress)), message->networkAddress);
            break;
        }

        case NWK_LEAVE_IND:
        {
            const nwkLeaveStruct *message = reinterpret_cast <const nwkLeaveStruct*> (data.constData());
            quint64 ieeeAddress = qToBigEndian <quint64> (message->ieeeAddress);
            emit deviceLeft(QByteArray(reinterpret_cast <char*> (&ieeeAddress), sizeof(ieeeAddress)));
            break;
        }

        default:
        {
            if ((type == RESPONSE) && (m_command == qFromBigEndian(command)))
            {
                if ((m_tsn == static_cast <quint8> (data.at(0))))
                {
                    emit dataReceived();

                    if (qFromBigEndian(command) == ZBOSS_NCP_RESET_IND) {
                        if (!startCoordinator())
                        {
                            logWarning << "Coordinator startup failed";
                            return;
                        }

                        m_resetTimer->stop();
                        return;
                    }

                    m_replyStatus = static_cast <quint8> (data.at(2));
                    m_replyData = data.mid(3, data.length() - 3);
                }
                return;
            }

            logWarning << "Unrecognized ZBoss command" << QString::asprintf("0x%04x", qFromBigEndian(command)) << "with data" << (data.isEmpty() ? "(empty)" : data.toHex(':'));
            break;
        }
    }
}

bool ZBoss::startCoordinator(void)
{
    moduleVersionResponseStruct version;
    localIEEEResponseStruct localIeee;
    channelMaskRequestStruct channel;
    nwkSetRequestStruct nwk;
    nwkForamtionStruct network;
    quint64 extendedPanId;
    bool withoutFormation = false;

    if (!sendRequest(ZBOSS_GET_MODULE_VERSION) || m_replyStatus)
    {
        logWarning << "Adapter version request failed";
        return false;
    }

    memcpy(&version, m_replyData.constData(), sizeof(version));
    m_manufacturerName = "Nordic Semiconductor";
    m_modelName = QString::asprintf("ZBOSS");
    m_firmware = QString::asprintf("%d.%d.%d", version.fwVersionMinor, version.fwVersionRevision, version.fwVersionCommit);

    logInfo << QString("Adapter type: %1 (%2)").arg(m_modelName, m_firmware).toUtf8().constData();

    if (!sendRequest(ZBOSS_SET_ZIGBEE_ROLE, QByteArray(1, static_cast <char> (LogicalType::Coordinator))) || m_replyStatus)
    {
        logWarning << "Set adapter logical type request failed";
        return false;
    }

    if (!sendRequest(ZBOSS_GET_LOCAL_IEEE_ADDR) || m_replyStatus)
    {
        logWarning << "Local IEEE address request failed";
        return false;
    }

    memcpy(&localIeee, m_replyData.constData(), sizeof(localIeee));
    m_ieeeAddress = QByteArray(reinterpret_cast <char*> (&localIeee.ieeeAddress), sizeof(localIeee.ieeeAddress));

    if (!sendRequest(ZBOSS_SET_PAN_ID, QByteArray(reinterpret_cast <char*> (&m_panId), sizeof(m_panId))) || m_replyStatus)
    {
        logWarning << "Set panid request failed";
        return false;
    }

    channel.page = 0;
    channel.mask = (1 << m_channel);

    if (!sendRequest(ZBOSS_SET_ZIGBEE_CHANNEL_MASK, QByteArray(reinterpret_cast <char*> (&channel), sizeof(channel))) || m_replyStatus)
    {
        logWarning << "Set channel mask request failed";
        return false;
    }

    memcpy(nwk.key, m_networkKey.constData(), sizeof(nwk.key));
    nwk.number = 0;

    if (!sendRequest(ZBOSS_SET_NWK_KEY, QByteArray(reinterpret_cast <char*> (&nwk), sizeof(nwk))) || m_replyStatus)
    {
        logWarning << "Set nwk request failed";
        return false;
    }

    for (int i = 0; i < m_policy.length(); i++)
    {
        setTCPolicyStruct request = m_policy.at(i);

        if (sendRequest(ZBOSS_SET_TC_POLICY, QByteArray(reinterpret_cast <char*> (&request), sizeof(request))) && !m_replyStatus)
            continue;

        logWarning << "Set policy" << QString::asprintf("0x%04x", request.id) << "request failed";
    }

    if (!sendRequest(ZBOSS_GET_EXTENDED_PAN_ID) || m_replyStatus)
    {
        logWarning << "Local IEEE address request failed";
        return false;
    }

    memcpy(&extendedPanId, m_replyData.constData(), sizeof(extendedPanId));

    network.channelListLen = 0x01;
    network.channelList = channel;
    network.scanDuration = 0x05;
    network.flag = 0x00;
    network.address = 0x0000;
    network.ieeeAddress = qToBigEndian <quint64> (extendedPanId);

    if (!sendRequest(NWK_FORMATION, QByteArray(reinterpret_cast<char *>(&network), sizeof(network))) || m_replyStatus)
    {
        withoutFormation = true;
    }

    if (withoutFormation)
    {
        if (!sendRequest(NWK_START_WITHOUT_FORMATION) || m_replyStatus)
        {
            logWarning << "Form network failed";
            return false;
        }
    }

    emit coordinatorReady();
    return true;
}

void ZBoss::softReset(void)
{
    m_packet_seq = 0;
    m_tsn = 0;
    m_ack_seq = -1;
    sendRequest(ZBOSS_NCP_RESET_IND, QByteArray(1, 0x01));    
}

void ZBoss::parseData(QByteArray &buffer)
{
    while (!buffer.isEmpty())
    {
        QByteArray frame, data;
        int length = static_cast <int> (static_cast <char> (buffer.at(3)) + static_cast <char> (buffer.at(2)));

        if (!buffer.startsWith(QByteArray::fromHex("dead")))
            return;

        if (m_portDebug)
            logInfo << "Packet received:" << buffer.mid(0, length + 2).toHex(':');

        frame = buffer.mid(10, length - 8);

        for (int i = 0; i < frame.length(); i++)
            data.append(1, frame.at(i));

        m_queue.enqueue(data);
        buffer.remove(0, length + 2);
    }
}

bool ZBoss::permitJoin(bool enabled)
{
    
    if (!sendRequest(ZBOSS_ZDO_PERMIT_JOINING_REQ, QByteArray(2, 0x00).append(1, enabled ? 0xFF : 0x00).append(0x01)) || m_replyStatus)
    {
        logWarning << "Form network failed";
        return false;
    }
    return true;
}

void ZBoss::handleQueue(void)
{
    while (!m_queue.isEmpty())
    {
        QByteArray packet = m_queue.dequeue();
        quint16 command;
        memcpy(&command, packet.mid(1, 2).constData(), sizeof(command));
        
        quint8 type = static_cast <quint8> (packet.at(0));
        parsePacket(type, qFromBigEndian(command), packet.mid(3));
        sendAck();
    }
}
