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

#include "FoilNotesHints.h"
#include "FoilNotesDefs.h"

#include "HarbourDebug.h"

#include <MGConfItem>

#define DCONF_KEY(x)                    FOILNOTES_DCONF_ROOT "hints/" x
#define KEY_LEFT_SWIPE_TO_PLAINTEXT     DCONF_KEY("leftSwipeToPlaintext")
#define KEY_LEFT_SWIPE_TO_DECRYPTED     DCONF_KEY("leftSwipeToDecrypted")
#define KEY_RIGHT_SWIPE_TO_ENCRYPTED    DCONF_KEY("rightSwipeToEncrypted")

FoilNotesHints::FoilNotesHints(QObject* aParent) :
    QObject(aParent),
    iLeftSwipeToPlaintext(new MGConfItem(KEY_LEFT_SWIPE_TO_PLAINTEXT, this)),
    iLeftSwipeToDecrypted(new MGConfItem(KEY_LEFT_SWIPE_TO_DECRYPTED, this)),
    iRightSwipeToEncrypted(new MGConfItem(KEY_RIGHT_SWIPE_TO_ENCRYPTED, this))
{
    connect(iLeftSwipeToPlaintext, SIGNAL(valueChanged()),
        SIGNAL(leftSwipeToPlaintextChanged()));
    connect(iLeftSwipeToDecrypted, SIGNAL(valueChanged()),
        SIGNAL(leftSwipeToDecryptedChanged()));
    connect(iRightSwipeToEncrypted, SIGNAL(valueChanged()),
        SIGNAL(rightSwipeToEncryptedChanged()));
}

// Callback for qmlRegisterSingletonType<FoilNotesHints>
QObject*
FoilNotesHints::createSingleton(
    QQmlEngine* aEngine,
    QJSEngine* aScript)
{
    return new FoilNotesHints(aEngine);
}

// leftSwipeToPlaintext

int
FoilNotesHints::leftSwipeToPlaintext() const
{
    return iLeftSwipeToPlaintext->value(0).toInt();
}

void
FoilNotesHints::setLeftSwipeToPlaintext(
    int aValue)
{
    HDEBUG(aValue);
    iLeftSwipeToPlaintext->set(aValue);
}

// leftSwipeToDecrypted

int
FoilNotesHints::leftSwipeToDecrypted() const
{
    return iLeftSwipeToDecrypted->value(0).toInt();
}

void
FoilNotesHints::setLeftSwipeToDecrypted(
    int aValue)
{
    HDEBUG(aValue);
    iLeftSwipeToDecrypted->set(aValue);
}

// rightSwipeToEncrypted

int
FoilNotesHints::rightSwipeToEncrypted() const
{
    return iRightSwipeToEncrypted->value(0).toInt();
}

void
FoilNotesHints::setRightSwipeToEncrypted(
    int aValue)
{
    HDEBUG(aValue);
    iRightSwipeToEncrypted->set(aValue);
}
