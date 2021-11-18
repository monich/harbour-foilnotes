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

#include "nfcdc_peer.h"
#include "nfcdc_default_adapter.h"

#include "FoilNotesNfcShare.h"
#include "FoilNotesNfcShareClient.h"
#include "FoilNotesNfcShareProtocol.h"

#include "HarbourDebug.h"

#include <unistd.h>
#include <sys/socket.h>

class FoilNotesNfcShareClient::Private {
    class Note {
    public:
        Note(QColor aColor, QString aText) :
            iColor(aColor), iText(aText) {}

        QColor iColor;
        QString iText;
    };

    enum {
        ADAPTER_EVENT_PEERS,
        ADAPTER_EVENT_COUNT
    };

    enum {
        CLIENT_EVENT_PRESENT,
        CLIENT_EVENT_COUNT
    };

public:
    class Protocol;

    Private(FoilNotesNfcShareClient* aShare);
    ~Private();

    void sendPlaintextNote(const QColor& aColor, const QString& aText);

private:
    void adapterPeersChanged();
    void peerPresenceChanged();
    void cancelConnection();
    void connectionFailed();
    void connected(int);
    void connect();

    static void adapterPeersCB(NfcDefaultAdapter*, NFC_DEFAULT_ADAPTER_PROPERTY, void*);
    static void clientPresentCB(NfcPeerClient*, NFC_PEER_PROPERTY, void*);
    static void clientConnectionCB(NfcPeerClient*, int, const GError*, void*);

public:
    FoilNotesNfcShareClient* iParent;
    Note* iNote;
    Protocol* iConnection;
    NfcDefaultAdapter* iAdapter;
    NfcPeerClient* iClient;
    GCancellable* iConnecting;
    gulong iAdapterEventIds[ADAPTER_EVENT_COUNT];
    gulong iClientEventIds[CLIENT_EVENT_COUNT];
    bool iError;
};

// ==========================================================================
// FoilNotesNfcShareClient::Private::Protocol
// ==========================================================================

class FoilNotesNfcShareClient::Private::Protocol: public FoilNotesNfcShareProtocol {
public:
    Protocol(int aFd, FoilNotesNfcShareClient::Private* aPrivate);

    void sendNote(const Note*);

    void handleHelloEvent(ProtocolVersion, const AppVersion*) Q_DECL_OVERRIDE;
    void handleRequestSendProgress(uint, uint, uint) Q_DECL_OVERRIDE;
    void handleResponse(uint, const uchar*, uint) Q_DECL_OVERRIDE;
    void handleError() Q_DECL_OVERRIDE;

public:
    FoilNotesNfcShareClient::Private* iPrivate;
    uint iSendReqId;
    uint iAckReqId;
};

FoilNotesNfcShareClient::Private::Protocol::Protocol(int aFd,
    FoilNotesNfcShareClient::Private* aPrivate) :
    FoilNotesNfcShareProtocol(aFd),
    iPrivate(aPrivate),
    iSendReqId(0),
    iAckReqId(0)
{
}

void FoilNotesNfcShareClient::Private::Protocol::sendNote(const Note* aNote)
{
    if (aNote) {
        iSendReqId = sendPlaintextRequest(aNote->iColor, aNote->iText);
    }
}

void FoilNotesNfcShareClient::Private::Protocol::handleHelloEvent(ProtocolVersion aProtocol,
    const AppVersion* aApp)
{
    HDEBUG("Protocol" << aProtocol << "app" << aApp->v1 << aApp->v2 << aApp->v3);
    sendNote(iPrivate->iNote);
    Q_EMIT iPrivate->iParent->stateChanged();
}

void FoilNotesNfcShareClient::Private::Protocol::handleRequestSendProgress(uint aId,
    uint aBytesSent, uint aBytesTotal)
{
    HDEBUG(aBytesSent << "bytes sent out of" << aBytesTotal);
}

void FoilNotesNfcShareClient::Private::Protocol::handleResponse(uint aReqId, const uchar*, uint)
{
    HDEBUG(aReqId << "ack'ed");
    iAckReqId = aReqId;
    if (iAckReqId == iSendReqId) {
        // Note has been sent successfully
        delete iPrivate->iNote;
        iPrivate->iNote = Q_NULLPTR;
        Q_EMIT iPrivate->iParent->stateChanged();
        Q_EMIT iPrivate->iParent->done();
    }
}

void FoilNotesNfcShareClient::Private::Protocol::handleError()
{
    if (!iPrivate->iError) {
        iPrivate->iError = true;
        Q_EMIT iPrivate->iParent->stateChanged();
    }
}

// ==========================================================================
// FoilNotesNfcShareClient::Private
// ==========================================================================

FoilNotesNfcShareClient::Private::Private(FoilNotesNfcShareClient* aParent) :
    iParent(aParent),
    iNote(Q_NULLPTR),
    iConnection(Q_NULLPTR),
    iAdapter(nfc_default_adapter_new()),
    iClient(Q_NULLPTR),
    iConnecting(Q_NULLPTR),
    iError(false)
{
    memset(iAdapterEventIds, 0, sizeof(iAdapterEventIds));
    memset(iClientEventIds, 0, sizeof(iClientEventIds));
    iAdapterEventIds[ADAPTER_EVENT_PEERS] =
        nfc_default_adapter_add_property_handler(iAdapter,
            NFC_DEFAULT_ADAPTER_PROPERTY_PEERS, adapterPeersCB, this);
}

FoilNotesNfcShareClient::Private::~Private()
{
    cancelConnection();
    nfc_default_adapter_remove_all_handlers(iAdapter, iAdapterEventIds);
    nfc_default_adapter_unref(iAdapter);
    nfc_peer_client_remove_all_handlers(iClient, iClientEventIds);
    nfc_peer_client_unref(iClient);
    delete iConnection;
    delete iNote;
}

void FoilNotesNfcShareClient::Private::sendPlaintextNote(const QColor& aColor, const QString& aText)
{
    delete iNote;
    iNote = new Note(aColor, aText);
    if (iConnection && iConnection->started()) {
        iConnection->sendNote(iNote);
        Q_EMIT iParent->stateChanged();
    }
}

void FoilNotesNfcShareClient::Private::adapterPeersChanged()
{
    const char* oldPeer = iClient ? iClient->path : Q_NULLPTR;
    const char* newPeer = iAdapter->peers[0];

    if (g_strcmp0(oldPeer, newPeer)) {
        if (oldPeer) {
            HDEBUG("Peer" << oldPeer << "left");
            nfc_peer_client_remove_all_handlers(iClient, iClientEventIds);
            nfc_peer_client_unref(iClient);
            iClient = Q_NULLPTR;
            cancelConnection();
            delete iConnection;
            iConnection = Q_NULLPTR;
            iError = false;
        }

        if (newPeer) {
            HDEBUG("Peer" << newPeer << "arrived");
            iClient = nfc_peer_client_new(newPeer);
            if (iClient->present) {
                connect();
            } else {
                // Wait for the client to initialize
                HDEBUG("Waiting for peer for initialize");
                iClientEventIds[CLIENT_EVENT_PRESENT] =
                    nfc_peer_client_add_property_handler(iClient,
                        NFC_PEER_PROPERTY_PRESENT, clientPresentCB, this);
            }
        }

        Q_EMIT iParent->stateChanged();
    }
}

void FoilNotesNfcShareClient::Private::adapterPeersCB(NfcDefaultAdapter*,
    NFC_DEFAULT_ADAPTER_PROPERTY, void* aThis)
{
    ((Private*)aThis)->adapterPeersChanged();
}

void FoilNotesNfcShareClient::Private::clientPresentCB(NfcPeerClient* aClient,
    NFC_PEER_PROPERTY, void* aThis)
{
    if (aClient->present) {
        Private* self = (Private*)aThis;

        nfc_peer_client_remove_all_handlers(aClient, self->iClientEventIds);
        self->connect();
    }
}

void FoilNotesNfcShareClient::Private::clientConnectionCB(NfcPeerClient*,
    int aFd, const GError* aError, void* aThis)
{
    Private* self = (Private*)aThis;

    g_object_unref(self->iConnecting);
    self->iConnecting = Q_NULLPTR;
    if (aError) {
        self->connectionFailed();
    } else {
        self->connected(aFd);
    }
}

void FoilNotesNfcShareClient::Private::cancelConnection()
{
    if (iConnecting) {
        g_cancellable_cancel(iConnecting);
        g_object_unref(iConnecting);
        iConnecting = Q_NULLPTR;
    }
}

void FoilNotesNfcShareClient::Private::connect()
{
    if (iClient && iClient->present && !iConnecting) {
        iConnecting = g_cancellable_new();
        HDEBUG("Connecting to" << FOILNOTES_NFCSHARE_SN);
        nfc_peer_client_connect_sn(iClient, FOILNOTES_NFCSHARE_SN,
            iConnecting, clientConnectionCB, this, NULL);
    }
}

void FoilNotesNfcShareClient::Private::connected(int aFd)
{
    HDEBUG("Connected to" << FOILNOTES_NFCSHARE_SN);
    delete iConnection;
    iConnection = new Protocol(aFd, this);
    iError = false;
    Q_EMIT iParent->stateChanged();
}

void FoilNotesNfcShareClient::Private::connectionFailed()
{
    HDEBUG("Failed to connect to" << FOILNOTES_NFCSHARE_SN);
    delete iConnection;
    iConnection = Q_NULLPTR;
    iError = true;
    Q_EMIT iParent->stateChanged();
}

// ==========================================================================
// FoilNotesNfcShareClient
// ==========================================================================

FoilNotesNfcShareClient::FoilNotesNfcShareClient(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

FoilNotesNfcShareClient::~FoilNotesNfcShareClient()
{
    delete iPrivate;
}

FoilNotesNfcShareClient::State FoilNotesNfcShareClient::state() const
{
    Private::Protocol* connection = iPrivate->iConnection;

    return (connection && connection->iSendReqId && connection->iSendReqId != connection->iAckReqId) ? Sending :
        (connection && connection->iSendReqId && connection->iSendReqId == connection->iAckReqId) ? Sent :
        (connection && connection->started()) ? Connected :
        (connection || iPrivate->iConnecting) ? Connecting :
        iPrivate->iError ? Error : Idle;
}

void FoilNotesNfcShareClient::sharePlaintext(QColor aColor, QString aText)
{
    HDEBUG(aColor << aText);
    iPrivate->sendPlaintextNote(aColor, aText);
}
