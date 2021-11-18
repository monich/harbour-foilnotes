/*
 * Copyright (C) 2021 Jolla Ltd.
 * Copyright (C) 2021 Slava Monich <slava@monich.com>
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the names of the copyright holders nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "FoilNotesNfcShare.h"
#include "FoilNotesNfcShareProtocol.h"

#include <QSocketNotifier>

#include "HarbourDebug.h"

#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

// ==========================================================================
// FoilNotesNfcShareProtocol::Private
// ==========================================================================

class FoilNotesNfcShareProtocol::Private : public QObject {
    Q_OBJECT

    static const char HELLO_MAGIC[];

    enum {
        HeaderSize = 4,
        EventHeaderSize = 5,
        RequestHeaderSize = 8,
        ResponseHeaderSize = 7,
        HelloEventPayloadSize = 20
    };

public:
    class Packet {
    public:
        QByteArray iHeader;
        QByteArray iData;
        Packet* iNext;
    public:
        Packet() : iNext(Q_NULLPTR) {}
        ~Packet() { delete iNext; }

        OpCode opCode() const { return (OpCode)iHeader.constData()[0]; }
        bool isRequest() const { return opCode() == OpCodeRequest; }
        uint requestId() const { return be3((uchar*)iHeader.constData() + 4); }
    };

    Private(int, FoilNotesNfcShareProtocol*);
    ~Private();

    static uint be3(const uchar*);

    static Packet* newRequest(uint, RequestCode, const QByteArray&);
    static Packet* newResponse(uint, const QByteArray&);
    static Packet* newEvent(EventCode, const QByteArray&);
    static Packet* newHelloEvent();
    void submitPacket(Packet*);

    bool tryToRead(size_t);
    void handleEvent(uchar, const uchar*, uint);
    void error();

public Q_SLOTS:
    void canRead();
    void tryToWrite();

public:
    FoilNotesNfcShareProtocol* iProtocol;
    int iFd;
    bool iStarted;
    uint iLastId;
    QSocketNotifier* iReadNotifier;
    QSocketNotifier* iWriteNotifier;
    QByteArray iReadBuf;
    Packet* iWriteQueue;
    Packet* iWriteQueueTail;
    Packet* iWritePacket;
    int iWritePacketOffset;
};

const char FoilNotesNfcShareProtocol::Private::HELLO_MAGIC[] = "foilnotes:share";

FoilNotesNfcShareProtocol::Private::Private(int aFd, FoilNotesNfcShareProtocol* aProtocol) :
    iProtocol(aProtocol),
    iFd(dup(aFd)),
    iStarted(false),
    iLastId(0),
    iReadNotifier(new QSocketNotifier(iFd, QSocketNotifier::Read, this)),
    iWriteNotifier(new QSocketNotifier(iFd, QSocketNotifier::Write, this)),
    iWriteQueue(Q_NULLPTR),
    iWriteQueueTail(Q_NULLPTR),
    iWritePacket(Q_NULLPTR),
    iWritePacketOffset(0)
{
    connect(iReadNotifier, SIGNAL(activated(int)), SLOT(canRead()));
    connect(iWriteNotifier, SIGNAL(activated(int)), SLOT(tryToWrite()));
    submitPacket(newHelloEvent());
    tryToWrite();
}

FoilNotesNfcShareProtocol::Private::~Private()
{
    delete iReadNotifier;
    delete iWriteNotifier;
    delete iWritePacket;
    delete iWriteQueue;
    if (iFd >= 0) {
        shutdown(iFd, SHUT_RDWR);
        close(iFd);
    }
}

inline uint FoilNotesNfcShareProtocol::Private::be3(const uchar* aBuffer)
{
    // Big engian 3-byte unsigned int
    return ((uint)aBuffer[0] << 16) + ((uint)aBuffer[1] << 8) + aBuffer[2];
}

void FoilNotesNfcShareProtocol::Private::error()
{
    iReadNotifier->setEnabled(false);
    iWriteNotifier->setEnabled(false);
    iProtocol->handleError();
}

void FoilNotesNfcShareProtocol::Private::submitPacket(Packet* aPacket)
{
    // Takes ownership of the packet
    if (!iWritePacket) {
        // Nothing is being written
        iWritePacket = aPacket;
        iWritePacketOffset = 0;
        // Disabled iReadNotifier means an error
        if (iStarted && iReadNotifier->isEnabled()) {
            iWriteNotifier->setEnabled(true);
            tryToWrite();
        }
    } else if (iWriteQueueTail) {
        // Append to the queue
        iWriteQueueTail->iNext = aPacket;
    } else {
        // First packet to queue
        iWriteQueue = iWriteQueueTail = aPacket;
    }
}

void FoilNotesNfcShareProtocol::Private::tryToWrite()
{
    if (!iWritePacket) {
        if (iWriteQueue) {
            // Pick the next one from the queue
            iWritePacket = iWriteQueue;
            iWriteQueue = iWritePacket->iNext;
            iWritePacket->iNext = Q_NULLPTR;
            if (!iWriteQueue) iWriteQueueTail = Q_NULLPTR;
            iWritePacketOffset = 0;
            if (iWritePacket->isRequest()) {
                iProtocol->handleRequestSendProgress(iWritePacket->requestId(),
                    0, iWritePacket->iData.size());
            }
        } else {
            // Nothing else to write, shut notifier off
            iWriteNotifier->setEnabled(false);
        }
    }

    if (iWritePacket) {
        if (iWritePacketOffset < iWritePacket->iHeader.size()) {
            const char* data = iWritePacket->iHeader.constData() + iWritePacketOffset;
            ssize_t numBytes = iWritePacket->iHeader.size() - iWritePacketOffset;
            ssize_t written;
            while ((written = write(iFd, data, numBytes)) == -1 && (errno == EINTR));
            if (written > 0) {
                iWritePacketOffset += written;
            }
            if (written < numBytes) {
                if (written < 0) {
                    error();
                }
                return;
            }
        }

        const ssize_t dataOffset = iWritePacketOffset - iWritePacket->iHeader.size();
        if (dataOffset < iWritePacket->iData.size()) {
            const char* data = iWritePacket->iData.constData() + dataOffset;
            ssize_t numBytes = iWritePacket->iData.size() - dataOffset;
            ssize_t written;
            while ((written = write(iFd, data, numBytes)) == -1 && (errno == EINTR));
            if (written > 0) {
                iWritePacketOffset += written;
                if (iWritePacket->isRequest()) {
                    iProtocol->handleRequestSendProgress(iWritePacket->requestId(),
                        dataOffset + written, iWritePacket->iData.size());
                }
            }
            if (written < numBytes) {
                if (written < 0) {
                    error();
                }
                return;
            }
            HDEBUG("Packet sent" << iWritePacketOffset << "bytes");
        }

        // The whole thing has been sent
        delete iWritePacket;
        iWritePacket = Q_NULLPTR;
        iWritePacketOffset = 0;

        if (!iStarted) {
            // Wrote initial packet, now wait for HELLO from the peer
            iWriteNotifier->setEnabled(false);
        }
    }
}

bool FoilNotesNfcShareProtocol::Private::tryToRead(size_t aNumBytes)
{
    const size_t oldSize = iReadBuf.size();
    iReadBuf.resize(oldSize + aNumBytes);
    ssize_t bytesRead;
    char* buf = iReadBuf.data() + oldSize;
    while ((bytesRead = read(iFd, buf, aNumBytes)) == -1 && (errno == EINTR));
    iReadBuf.resize(oldSize + qMax(bytesRead, (ssize_t)0));
    if (bytesRead > 0) {
        return true;
    } else {
        if (!bytesRead) {
            HDEBUG("EOF on read");
            iReadNotifier->setEnabled(false);
        } else {
            error();
        }
        return false;
    }
}

void FoilNotesNfcShareProtocol::Private::canRead()
{
    if (iReadBuf.size() < HeaderSize) {
        if (!tryToRead(HeaderSize - iReadBuf.size()) ||
            iReadBuf.size() < HeaderSize) {
            return;
        }
    }

    // +--------+--------+--------+--------+
    // | OPCODE |      N (Big endian)      |
    // +--------+--------+--------+--------+
    // | Payload N bytes (optional)
    // +-------->

    const uchar* header = (uchar*)iReadBuf.constData();
    const uint n = be3(header + 1);
    const ssize_t bytesTotal = HeaderSize + n;

#if HARBOUR_DEBUG
    if (iReadBuf.size() == HeaderSize) {
        HDEBUG("Incoming packet" << bytesTotal << "bytes");
    }
#endif // HARBOUR_DEBUG

    if (iReadBuf.size() < bytesTotal) {
        if (!tryToRead(bytesTotal - iReadBuf.size()) ||
            iReadBuf.size() < bytesTotal) {
            return;
        }
    }

    // Handle the complete packet
    const uchar* packet = (uchar*)iReadBuf.constData(); // May get reallocated
    switch ((OpCode)packet[0]) {
    case OpCodeEvent:
        // +--------+--------+--------+--------+
        // |    0   |         N (>= 1)         |
        // +--------+--------+--------+--------+
        // |  CODE  | Data N-1 bytes (optional)
        // +--------+-------->
        if (n >= 1) {
            handleEvent(packet[4], packet + 5, n - 1);
        } else {
            HWARN("Ignoring broken event");
        }
        break;
    case OpCodeRequest:
        // +--------+--------+--------+--------+
        // |    1   |         N (>= 4)         |
        // +--------+--------+--------+--------+
        // |           ID             |  CODE  |
        // +--------+--------+--------+--------+
        // |  Data N-4 bytes (optional)
        // +-------->
        if (n >= 4) {
            iProtocol->handleIncomingRequest((RequestCode) packet[7],
                be3(packet + 4), packet + 8, n - 4);
        } else {
            HWARN("Ignoring broken request");
        }
        break;
    case OpCodeResponse:
        // +--------+--------+--------+--------+
        // |    2   |         N (>= 3)         |
        // +--------+--------+--------+--------+
        // |           ID             | Data N-3 bytes (optional)
        // +--------+--------+--------+-------->
        if (n >= 3) {
            iProtocol->handleResponse(be3(packet + 4), packet + 7, n - 3);
        } else {
            HWARN("Ignoring broken response");
        }
        break;
    default:
        HWARN("Ignoring unsupported OpType" << header[0]);
        break;
    }

    iReadBuf = QByteArray();
}

void FoilNotesNfcShareProtocol::Private::handleEvent(uchar aCode,
    const uchar* aData, uint aLength)
{
    if (aCode == (uchar)EventHello) {
        if (!iStarted && aLength == HelloEventPayloadSize &&
            !memcmp(aData, HELLO_MAGIC, sizeof(HELLO_MAGIC))) {
            AppVersion appVersion;
            appVersion.v1 = aData[17];
            appVersion.v2 = aData[18];
            appVersion.v3 = aData[19];
            iStarted = true;
            tryToWrite();
            iProtocol->handleHelloEvent((ProtocolVersion)aData[16], &appVersion);
            return;
        }
    }
    iProtocol->handleEvent((EventCode)aCode, aData, aLength);
}

FoilNotesNfcShareProtocol::Private::Packet*
FoilNotesNfcShareProtocol::Private::newEvent(EventCode aCode, const QByteArray& aData)
{
    Packet* packet = new Packet;
    const uint size = aData.size() + (EventHeaderSize - HeaderSize);

    // +--------+--------+--------+--------+
    // |    0   |         N (>= 1)         |
    // +--------+--------+--------+--------+
    // |  CODE  | Data N-1 bytes (optional)
    // +--------+-------->
    packet->iHeader.resize(EventHeaderSize);
    uchar* header = (uchar*)packet->iHeader.data();
    header[0] = OpCodeEvent;
    header[1] = (uchar)(size >> 16);
    header[2] = (uchar)(size >> 8);
    header[3] = (uchar)size;
    header[4] = (uchar)aCode;

    packet->iData = aData;
    return packet;
}

FoilNotesNfcShareProtocol::Private::Packet*
FoilNotesNfcShareProtocol::Private::newRequest(uint aId,
    RequestCode aCode, const QByteArray& aData)
{
    Packet* packet = new Packet;
    const uint size = aData.size() + (RequestHeaderSize - HeaderSize);

    // +--------+--------+--------+--------+
    // |    1   |         N (>= 4)         |
    // +--------+--------+--------+--------+
    // |           ID             |  CODE  |
    // +--------+--------+--------+--------+
    // |  Data N-4 bytes (optional)
    // +-------->
    packet->iHeader.resize(RequestHeaderSize);
    uchar* header = (uchar*)packet->iHeader.data();
    header[0] = OpCodeRequest;
    header[1] = (uchar)(size >> 16);
    header[2] = (uchar)(size >> 8);
    header[3] = (uchar)size;
    header[4] = (uchar)(aId >> 16);
    header[5] = (uchar)(aId >> 8);
    header[6] = (uchar)aId;
    header[7] = (uchar)aCode;

    packet->iData = aData;
    return packet;
}

FoilNotesNfcShareProtocol::Private::Packet*
FoilNotesNfcShareProtocol::Private::newResponse(uint aId, const QByteArray& aData)
{
    Packet* packet = new Packet;
    const uint size = aData.size() + (ResponseHeaderSize - HeaderSize);

    // +--------+--------+--------+--------+
    // |    2   |         N (>= 3)         |
    // +--------+--------+--------+--------+
    // |           ID             | Data N-3 bytes (optional)
    // +--------+--------+--------+-------->
    packet->iHeader.resize(ResponseHeaderSize);
    uchar* header = (uchar*)packet->iHeader.data();
    header[0] = OpCodeResponse;
    header[1] = (uchar)(size >> 16);
    header[2] = (uchar)(size >> 8);
    header[3] = (uchar)size;
    header[4] = (uchar)(aId >> 16);
    header[5] = (uchar)(aId >> 8);
    header[6] = (uchar)aId;

    packet->iData = aData;
    return packet;
}

FoilNotesNfcShareProtocol::Private::Packet*
FoilNotesNfcShareProtocol::Private::newHelloEvent()
{
    //
    // HELLO event payload (20 bytes):
    //
    // +--------+--------+--------+--------+
    // |   'f'  |   'o'  |   'i'  |   'l'  |
    // +--------+--------+--------+--------+
    // |   'n'  |   'o'  |   't'  |   'e'  |
    // +--------+--------+--------+--------+
    // |   's'  |   ':'  |   's'  |   'h'  |
    // +--------+--------+--------+--------+
    // |   'a'  |   'r'  |   'e'  |    0   |
    // +--------+--------+--------+--------+
    // | VPROTO |   V1   |   V2   |   V3   |
    // +--------+--------+--------+--------+
    //
    // VPROTO is the protocol version
    // V1.V2.V3 is the app version
    //

    QByteArray data;

    data.resize(HelloEventPayloadSize);
    strcpy(data.data(), HELLO_MAGIC);
    uchar* payload = (uchar*)data.data();
    payload[16] = (uchar) Version;
    payload[17] = VERSION_MAJOR;
    payload[18] = VERSION_MINOR;
    payload[19] = VERSION_MICRO;
    return newEvent(EventHello, data);
}

// ==========================================================================
// FoilNotesNfcShareProtocol
// ==========================================================================

FoilNotesNfcShareProtocol::FoilNotesNfcShareProtocol(int aFd) :
    iPrivate(new Private(aFd, this))
{
}

FoilNotesNfcShareProtocol::~FoilNotesNfcShareProtocol()
{
}

void FoilNotesNfcShareProtocol::handleEvent(EventCode aCode, const uchar*, uint)
{
    HDEBUG("Unhandled event" << aCode);
}

void FoilNotesNfcShareProtocol::handleHelloEvent(ProtocolVersion, const AppVersion*)
{
}

void FoilNotesNfcShareProtocol::handleRequestSendProgress(uint, uint, uint)
{
}

void FoilNotesNfcShareProtocol::handleIncomingRequest(RequestCode, uint, const uchar*, uint)
{
}

void FoilNotesNfcShareProtocol::handleResponse(uint, const uchar*, uint)
{
}

void FoilNotesNfcShareProtocol::handleError()
{
}

bool FoilNotesNfcShareProtocol::started() const
{
    return iPrivate->iStarted;
}

uint FoilNotesNfcShareProtocol::sendRequest(RequestCode aCode, const QByteArray& aPayload)
{
    const uint id = ++iPrivate->iLastId;
    iPrivate->submitPacket(iPrivate->newRequest(id, aCode, aPayload));
    return id;
}

uint FoilNotesNfcShareProtocol::sendPlaintextRequest(const QColor& aColor, const QString& aText)
{
    const QByteArray colorName(aColor.name().toLatin1());
    const QByteArray textBytes(aText.toUtf8());
    QByteArray payload;
    payload.reserve(colorName.size() + 1 + textBytes.size());
    payload.append(colorName);
    payload.append((char)0);
    payload.append(textBytes);
    return sendRequest(RequestSendPlaintext, payload);
}

void FoilNotesNfcShareProtocol::sendResponse(uint aReqId, const QByteArray& aPayload)
{
    iPrivate->submitPacket(iPrivate->newResponse(aReqId, aPayload));
}

#include "FoilNotesNfcShareProtocol.moc"
