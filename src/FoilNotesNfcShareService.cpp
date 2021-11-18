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

#include "nfcdc_peer_service.h"

#include "FoilNotesNfcShare.h"
#include "FoilNotesNfcShareProtocol.h"
#include "FoilNotesNfcShareService.h"

#include "HarbourDebug.h"

#define FOILNOTES_NFCSHARE_DBUSPATH "/foilnotes/share"

class FoilNotesNfcShareService::Private {
    class Protocol;
    friend class Protocol;

    enum {
        SERVICE_EVENT_SAP_CHANGED,
        SERVICE_EVENT_PEER_LEFT,
        SERVICE_EVENT_COUNT
    };

public:
    Private(FoilNotesNfcShareService* aShare);
    ~Private();

    void setActive(bool);

private:
    void peerPresenceChanged();
    void propertyChange(NFC_PEER_SERVICE_PROPERTY);
    void connectionHandler(NfcServiceConnection*);
    void peerLeft(const char*);
    void clientConnected(int);

    static void propertyChangeCB(NfcPeerService*, NFC_PEER_SERVICE_PROPERTY, void*);
    static void connectionHandlerCB(NfcPeerService*, NfcServiceConnection*, void*);
    static void peerLeftCB(NfcPeerService*, const char*, void*);

public:
    FoilNotesNfcShareService* iParent;
    Protocol* iConnection;
    NfcPeerService* iService;
    gulong iServiceEventIds[SERVICE_EVENT_COUNT];
};

// ==========================================================================
// FoilNotesNfcShareService::Private::Protocol
// ==========================================================================

class FoilNotesNfcShareService::Private::Protocol: public FoilNotesNfcShareProtocol {
public:
    Protocol(NfcServiceConnection* aConnection, FoilNotesNfcShareService* aParent);
    ~Protocol();

    void handleHelloEvent(ProtocolVersion, const AppVersion*) Q_DECL_OVERRIDE;
    void handleIncomingRequest(RequestCode, uint, const uchar*, uint) Q_DECL_OVERRIDE;

public:
    FoilNotesNfcShareService* iParent;
    NfcServiceConnection* iConnection;
};

FoilNotesNfcShareService::Private::Protocol::Protocol(NfcServiceConnection* aConnection,
    FoilNotesNfcShareService* aParent) :
    FoilNotesNfcShareProtocol(aConnection->fd),
    iParent(aParent),
    iConnection(nfc_service_connection_accept(aConnection))
{
}

FoilNotesNfcShareService::Private::Protocol::~Protocol()
{
    nfc_service_connection_unref(iConnection);
}

void FoilNotesNfcShareService::Private::Protocol::handleHelloEvent(ProtocolVersion aProtocol,
    const AppVersion* aApp)
{
    HDEBUG("Protocol" << aProtocol << "app" << aApp->v1 << aApp->v2 << aApp->v3);
    Q_EMIT iParent->connectedChanged();
    FoilNotesNfcShareProtocol::handleHelloEvent(aProtocol, aApp);
}

void FoilNotesNfcShareService::Private::Protocol::handleIncomingRequest(RequestCode aCode,
    uint aReqId, const uchar* aData, uint aLength)
{
    HDEBUG("Incoming request" << aCode << "id" << aReqId << "length" << aLength);
    const char* sep = (char*) memchr(aData, 0, aLength);
    if (sep) {
        const QString colorString(QString::fromLatin1((char*)aData));
        const QString text(QString::fromUtf8(sep + 1));
        HDEBUG("Color" << colorString);
        HDEBUG("Text" << text);
        Q_EMIT iParent->newNote(QColor(colorString), text);
    }
    sendResponse(aReqId);
}

// ==========================================================================
// FoilNotesNfcShareService::Private
// ==========================================================================

FoilNotesNfcShareService::Private::Private(FoilNotesNfcShareService* aParent) :
    iParent(aParent),
    iConnection(Q_NULLPTR),
    iService(Q_NULLPTR)
{
    memset(iServiceEventIds, 0, sizeof(iServiceEventIds));
}

FoilNotesNfcShareService::Private::~Private()
{
    delete iConnection;
    nfc_peer_service_remove_all_handlers(iService, iServiceEventIds);
    nfc_peer_service_unref(iService);
}

void FoilNotesNfcShareService::Private::setActive(bool aActive)
{
    if (aActive) {
         if (!iService) {
             iService = nfc_peer_service_new(FOILNOTES_NFCSHARE_DBUSPATH,
                 FOILNOTES_NFCSHARE_SN, connectionHandlerCB, this);
             iServiceEventIds[SERVICE_EVENT_SAP_CHANGED] =
                 nfc_peer_service_add_property_handler(iService,
                     NFC_PEER_SERVICE_PROPERTY_SAP, propertyChangeCB, this);
             iServiceEventIds[SERVICE_EVENT_PEER_LEFT] =
                 nfc_peer_service_add_peer_left_handler(iService,
                    peerLeftCB, this);
             Q_EMIT iParent->activeChanged();
         }
    } else if (iService) {
        nfc_peer_service_remove_all_handlers(iService, iServiceEventIds);
        nfc_peer_service_unref(iService);
        iService = Q_NULLPTR;
        if (iConnection) {
            const bool wasStarted = iConnection->started();
            delete iConnection;
            iConnection = Q_NULLPTR;
            if (wasStarted) {
                Q_EMIT iParent->connectedChanged();
            }
        }
        Q_EMIT iParent->activeChanged();
    }
}

#define FOILNOTES_NFCSHARE_SERVICE_CALLBACK_IMPL(name,type) \
void FoilNotesNfcShareService::Private::name##CB(NfcPeerService*, type aArg, void* aThis) \
{ ((Private*)aThis)->name(aArg); }

FOILNOTES_NFCSHARE_SERVICE_CALLBACK_IMPL(propertyChange, NFC_PEER_SERVICE_PROPERTY)
FOILNOTES_NFCSHARE_SERVICE_CALLBACK_IMPL(connectionHandler, NfcServiceConnection*)
FOILNOTES_NFCSHARE_SERVICE_CALLBACK_IMPL(peerLeft,const char*)

void FoilNotesNfcShareService::Private::propertyChange(NFC_PEER_SERVICE_PROPERTY)
{
    HDEBUG("NFC service SAP" << iService->sap);
    Q_EMIT iParent->registeredChanged();
}

void FoilNotesNfcShareService::Private::connectionHandler(NfcServiceConnection* aConnection)
{
    HDEBUG("Accepting connection from" << aConnection->rsap);
    const bool wasStarted = (iConnection && iConnection->started());
    delete iConnection;
    iConnection = new Protocol(aConnection, iParent);
    if (wasStarted) {
        Q_EMIT iParent->connectedChanged();
    }
}

void FoilNotesNfcShareService::Private::peerLeft(const char* aPath)
{
    HDEBUG("Peer" << aPath << "left");
    if (iConnection) {
        const bool wasStarted = (iConnection && iConnection->started());
        delete iConnection;
        iConnection = Q_NULLPTR;
        if (wasStarted) {
            Q_EMIT iParent->connectedChanged();
        }
    }
}

// ==========================================================================
// FoilNotesNfcShareService
// ==========================================================================

FoilNotesNfcShareService::FoilNotesNfcShareService(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

FoilNotesNfcShareService::~FoilNotesNfcShareService()
{
    delete iPrivate;
}

QObject* FoilNotesNfcShareService::createSingleton(QQmlEngine*, QJSEngine*)
{
    return new FoilNotesNfcShareService;
}

bool FoilNotesNfcShareService::active() const
{
    return iPrivate->iService != Q_NULLPTR;
}

void FoilNotesNfcShareService::setActive(bool aActive)
{
    HDEBUG(aActive);
    iPrivate->setActive(aActive);
}

bool FoilNotesNfcShareService::registered() const
{
    return iPrivate->iService && iPrivate->iService->sap != 0;
}

bool FoilNotesNfcShareService::connected() const
{
    return iPrivate->iConnection && iPrivate->iConnection->started();
}
