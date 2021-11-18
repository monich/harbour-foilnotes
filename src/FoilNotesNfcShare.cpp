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
#include "nfcdc_peer_service.h"

#include "FoilNotesNfcShare.h"

#include "HarbourDebug.h"

#include <unistd.h>
#include <sys/socket.h>

#define FOILNOTES_NFCSHARE_SN       "foilnotes:share"
#define FOILNOTES_NFCSHARE_DBUSPATH "/foilnotes/share"

// ==========================================================================
// FoilNotesNfcShare::Private
// ==========================================================================

class FoilNotesNfcShare::Private {
    enum client_events {
        CLIENT_EVENT_PRESENT,
        CLIENT_EVENT_COUNT
    };

    enum service_events {
        SERVICE_EVENT_SAP_CHANGED,
        SERVICE_EVENT_PEER_ARRIVED,
        SERVICE_EVENT_PEER_LEFT,
        SERVICE_EVENT_COUNT
    };

public:
    Private(FoilNotesNfcShare* aShare);
    ~Private();

    void setAccept(bool aAccept);

private:
    void peerPresenceChanged();
    void propertyChange(NFC_PEER_SERVICE_PROPERTY);
    void connectionHandler(NfcServiceConnection*);
    void peerArrived(const char*);
    void peerLeft(const char*);
    void clientConnected(int aFd);

    static void propertyChangeCB(NfcPeerService*, NFC_PEER_SERVICE_PROPERTY, void*);
    static void connectionHandlerCB(NfcPeerService*, NfcServiceConnection*, void*);
    static void peerArrivedCB(NfcPeerService*, const char*, void*);
    static void peerLeftCB(NfcPeerService*, const char*, void*);

    static void clientPresentCB(NfcPeerClient*, NFC_PEER_PROPERTY, void*);
    static void clientConnectionCB(NfcPeerClient*, int fd, const GError* error, void* user_data);

public:
    FoilNotesNfcShare* iShare;
    NfcPeerService* iService;
    NfcServiceConnection* iServiceConnection;
    gulong iServiceEventIds[SERVICE_EVENT_COUNT];
    NfcPeerClient* iClient;
    gulong iClientEventIds[CLIENT_EVENT_COUNT];
    int iFd;
};

FoilNotesNfcShare::Private::Private(FoilNotesNfcShare* aShare) :
    iShare(aShare),
    iService(Q_NULLPTR),
    iServiceConnection(Q_NULLPTR),
    iClient(Q_NULLPTR),
    iFd(-1)
{
    memset(iServiceEventIds, 0, sizeof(iServiceEventIds));
    memset(iClientEventIds, 0, sizeof(iClientEventIds));
}

FoilNotesNfcShare::Private::~Private()
{
    nfc_service_connection_unref(iServiceConnection);
    nfc_peer_service_remove_all_handlers(iService, iServiceEventIds);
    nfc_peer_service_unref(iService);
    nfc_peer_client_remove_all_handlers(iClient, iClientEventIds);
    nfc_peer_client_unref(iClient);
    if (iFd >= 0) {
        shutdown(iFd, SHUT_RDWR);
        close(iFd);
    }
}

void FoilNotesNfcShare::Private::setAccept(bool aAccept)
{
    if (aAccept) {
         if (!iService) {
             iService = nfc_peer_service_new(FOILNOTES_NFCSHARE_DBUSPATH,
                 FOILNOTES_NFCSHARE_SN, connectionHandlerCB, this);
             iServiceEventIds[SERVICE_EVENT_SAP_CHANGED] =
                 nfc_peer_service_add_property_handler(iService,
                     NFC_PEER_SERVICE_PROPERTY_SAP, propertyChangeCB, this);
             iServiceEventIds[SERVICE_EVENT_PEER_ARRIVED] =
                 nfc_peer_service_add_peer_arrived_handler(iService,
                    peerArrivedCB, this);
             iServiceEventIds[SERVICE_EVENT_PEER_LEFT] =
                 nfc_peer_service_add_peer_left_handler(iService,
                    peerLeftCB, this);
             Q_EMIT iShare->acceptChanged();
         }
    } else if (iService) {
        nfc_peer_service_remove_all_handlers(iService, iServiceEventIds);
        nfc_peer_service_unref(iService);
        iService = Q_NULLPTR;
        if (iServiceConnection) {
            nfc_service_connection_unref(iServiceConnection);
            iServiceConnection = Q_NULLPTR;
            Q_EMIT iShare->receivingChanged();
        }
        Q_EMIT iShare->acceptChanged();
    }
}

#define FOILNOTES_NFCSHARE_SERVICE_CALLBACK_IMPL(name,type) \
void FoilNotesNfcShare::Private::name##CB(NfcPeerService* aService, type aArg, void* aThis) \
{ ((Private*)aThis)->name(aArg); }

FOILNOTES_NFCSHARE_SERVICE_CALLBACK_IMPL(propertyChange, NFC_PEER_SERVICE_PROPERTY)
FOILNOTES_NFCSHARE_SERVICE_CALLBACK_IMPL(connectionHandler, NfcServiceConnection*)
FOILNOTES_NFCSHARE_SERVICE_CALLBACK_IMPL(peerArrived, const char*)
FOILNOTES_NFCSHARE_SERVICE_CALLBACK_IMPL(peerLeft,const char*)

void FoilNotesNfcShare::Private::propertyChange(NFC_PEER_SERVICE_PROPERTY aProperty)
{
    HDEBUG("NFC service SAP" << iService->sap);
    Q_EMIT iShare->registeredChanged();
}

void FoilNotesNfcShare::Private::connectionHandler(NfcServiceConnection* aConnection)
{
    HDEBUG("Accepting connection from" << aConnection->rsap);
    const bool hadConnection = (iServiceConnection != Q_NULLPTR);
    nfc_service_connection_unref(iServiceConnection);
    iServiceConnection = nfc_service_connection_accept(aConnection);
    if (!hadConnection) {
        Q_EMIT iShare->receivingChanged();
    }
}

void FoilNotesNfcShare::Private::peerArrived(const char* aPath)
{
    HDEBUG("Peer" << aPath << "arrived");
    nfc_peer_client_remove_all_handlers(iClient, iClientEventIds);
    nfc_peer_client_unref(iClient);
    iClient = nfc_peer_client_new(aPath);
    if (iClient->present) {
        HDEBUG("Connecting to" << FOILNOTES_NFCSHARE_SN);
        nfc_peer_client_connect_sn(iClient, FOILNOTES_NFCSHARE_SN, NULL,
            clientConnectionCB, this, NULL);
    } else {
        // Wait for the client to initialize
        HDEBUG("Waiting for peer for initialize");
        iClientEventIds[CLIENT_EVENT_PRESENT] =
            nfc_peer_client_add_property_handler(iClient,
                NFC_PEER_PROPERTY_PRESENT, clientPresentCB, this);
    }
}

void FoilNotesNfcShare::Private::peerLeft(const char* aPath)
{
    HDEBUG("Peer" << aPath << "left");
    if (iFd >= 0) {
        shutdown(iFd, SHUT_RDWR);
        close(iFd);
        iFd = -1;
        Q_EMIT iShare->sendingChanged();
    }
    if (iServiceConnection) {
        nfc_service_connection_unref(iServiceConnection);
        iServiceConnection = Q_NULLPTR;
        Q_EMIT iShare->receivingChanged();
    }
    if (iClient) {
        nfc_peer_client_unref(iClient);
        iClient = Q_NULLPTR;
    }
}

void FoilNotesNfcShare::Private::clientPresentCB(NfcPeerClient* aClient,
    NFC_PEER_PROPERTY aPreperty, void* aThis)
{
    if (aClient->present) {
        Private* self = (Private*)aThis;
        nfc_peer_client_remove_all_handlers(aClient, self->iClientEventIds);
        HDEBUG("Connecting to" << FOILNOTES_NFCSHARE_SN);
        nfc_peer_client_connect_sn(aClient, FOILNOTES_NFCSHARE_SN, NULL,
            clientConnectionCB, self, NULL);
    }
}

void FoilNotesNfcShare::Private::clientConnectionCB(NfcPeerClient* aClient,
    int aFd, const GError* aError, void* aThis)
{
    if (!aError) {
        HDEBUG("Connected to" << FOILNOTES_NFCSHARE_SN);
        ((Private*)aThis)->clientConnected(aFd);
    }
}

void FoilNotesNfcShare::Private::clientConnected(int aFd)
{
    if (iFd >= 0) {
        shutdown(iFd, SHUT_RDWR);
        close(iFd);
    }
    iFd = dup(aFd);
    Q_EMIT iShare->sendingChanged();
}

// ==========================================================================
// FoilNotesNfcShare
// ==========================================================================

FoilNotesNfcShare::FoilNotesNfcShare(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

FoilNotesNfcShare::~FoilNotesNfcShare()
{
    delete iPrivate;
}

QObject* FoilNotesNfcShare::createSingleton(QQmlEngine*, QJSEngine*)
{
    return new FoilNotesNfcShare;
}

bool FoilNotesNfcShare::accept() const
{
    return iPrivate->iService != Q_NULLPTR;
}

void FoilNotesNfcShare::setAccept(bool aAccept)
{
    HDEBUG(aAccept);
    iPrivate->setAccept(aAccept);
}

bool FoilNotesNfcShare::registered() const
{
    return iPrivate->iService && iPrivate->iService->sap != 0;
}

bool FoilNotesNfcShare::receiving() const
{
    return iPrivate->iServiceConnection != Q_NULLPTR;
}

bool FoilNotesNfcShare::sending() const
{
    return iPrivate->iFd >= 0;
}

void FoilNotesNfcShare::share(QColor aColor, QString aText)
{
    HDEBUG(aColor << aText);
}
