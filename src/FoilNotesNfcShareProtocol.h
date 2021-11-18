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

#ifndef FOILNOTES_NFCSHARE_PROTOCOL_H
#define FOILNOTES_NFCSHARE_PROTOCOL_H

#include "foil_types.h"

#include <QColor>
#include <QString>

//
// Wire format
// ===========
//
// +--------+--------+--------+--------+
// | OPCODE |      N (Big endian)      |
// +--------+--------+--------+--------+
// | Payload N bytes (optional)
// +-------->
//
// OPCODE:
// 0: Event
// 1: Request
// 2: Response
//
// Event
// =====
//
// +--------+--------+--------+--------+
// |    0   |         N (>= 1)         |
// +--------+--------+--------+--------+
// |  CODE  | Data N-1 bytes (optional)
// +--------+-------->
//
// Event codes:
// 0: HELLO
//
// Request
// =======
//
// +--------+--------+--------+--------+
// |    1   |         N (>= 4)         |
// +--------+--------+--------+--------+
// |           ID             |  CODE  |
// +--------+--------+--------+--------+
// |  Data N-4 bytes (optional)
// +-------->
//
// Event codes:
// 0: SEND_PLAINTEXT
//
// Response
// ========
//
// +--------+--------+--------+--------+
// |    2   |         N (>= 3)         |
// +--------+--------+--------+--------+
// |           ID             | Data N-3 bytes (optional)
// +--------+--------+--------+-------->
//
//

class FoilNotesNfcShareProtocol {
    Q_DISABLE_COPY(FoilNotesNfcShareProtocol)

public:
    struct AppVersion { uint v1, v2, v3; };

    enum OpCode {
        OpCodeEvent,
        OpCodeRequest,
        OpCodeResponse
    };

    enum EventCode {
        EventHello
    };

    enum RequestCode {
        RequestSendPlaintext
    };

    enum ProtocolVersion {
        Version = 1
    };

    FoilNotesNfcShareProtocol(int aFd);
    virtual ~FoilNotesNfcShareProtocol();

    bool started() const;
    uint sendRequest(RequestCode, const QByteArray&);
    uint sendPlaintextRequest(const QColor& aColor, const QString& aText);
    void sendResponse(uint, const QByteArray& aData = QByteArray());

protected:
    virtual void handleEvent(EventCode, const uchar*, uint);
    virtual void handleHelloEvent(ProtocolVersion, const AppVersion*);
    virtual void handleRequestSendProgress(uint, uint, uint);
    virtual void handleIncomingRequest(RequestCode, uint, const uchar*, uint);
    virtual void handleResponse(uint, const uchar*, uint);
    virtual void handleError();

private:
    class Private;
    friend class Private;
    Private* iPrivate;
};

#endif // FOILNOTES_NFCSHARE_PROTOCOL_H
