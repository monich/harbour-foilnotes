/*
 * Copyright (C) 2018-2019 Jolla Ltd.
 * Copyright (C) 2018-2019 Slava Monich <slava@monich.com>
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

#ifndef FOILNOTES_HINTS_H
#define FOILNOTES_HINTS_H

#include <QtQml>

class MGConfItem;

class FoilNotesHints : public QObject {
    Q_OBJECT
    // The value of the property is the number of times the hint has been used.
    Q_PROPERTY(int leftSwipeToPlaintext READ leftSwipeToPlaintext
        WRITE setLeftSwipeToPlaintext NOTIFY leftSwipeToPlaintextChanged)
    Q_PROPERTY(int leftSwipeToDecrypted READ leftSwipeToDecrypted
        WRITE setLeftSwipeToDecrypted NOTIFY leftSwipeToDecryptedChanged)
    Q_PROPERTY(int rightSwipeToEncrypted READ rightSwipeToEncrypted
        WRITE setRightSwipeToEncrypted NOTIFY rightSwipeToEncryptedChanged)

public:
    explicit FoilNotesHints(QObject* aParent = Q_NULLPTR);
    static QObject* createSingleton(QQmlEngine* aEngine, QJSEngine* aScript);

    int leftSwipeToPlaintext() const;
    void setLeftSwipeToPlaintext(int aValue);

    int leftSwipeToDecrypted() const;
    void setLeftSwipeToDecrypted(int aValue);

    int rightSwipeToEncrypted() const;
    void setRightSwipeToEncrypted(int aValue);

Q_SIGNALS:
    void leftSwipeToPlaintextChanged();
    void leftSwipeToDecryptedChanged();
    void rightSwipeToEncryptedChanged();

private:
    MGConfItem* iLeftSwipeToPlaintext;
    MGConfItem* iLeftSwipeToDecrypted;
    MGConfItem* iRightSwipeToEncrypted;
};

QML_DECLARE_TYPE(FoilNotesHints)

#endif // FOILNOTES_HINTS_H
