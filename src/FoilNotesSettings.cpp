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

#include "FoilNotesSettings.h"
#include "FoilNotesDefs.h"

#include "HarbourDebug.h"

#include <MGConfItem>

#include <QQmlEngine>

#define DCONF_KEY(x)                FOILNOTES_DCONF_ROOT x
#define KEY_NEXT_COLOR_INDEX        DCONF_KEY("nextColorIndex")
#define KEY_SHARED_KEY_WARNING      DCONF_KEY("sharedKeyWarning")

#define DEFAULT_NEXT_COLOR_INDEX    0
#define DEFAULT_SHARED_KEY_WARNING  true

// ==========================================================================
// FoilNotesSettings::Private
// ==========================================================================

class FoilNotesSettings::Private {
    static const char* gAvailableColors[];

public:
    Private(QObject* aParent);

public:
    QStringList iAvailableColors;
    MGConfItem* iNextColorIndex;
    MGConfItem* iSharedKeyWarning;
};

const char* FoilNotesSettings::Private::gAvailableColors[] = {
    "#00aff0", "#007fc4", "#00569b",
    "#f13c27", "#fedc00", "#f78628",
    "#4fb548", "#01823f", "#007f7f",
    "#7d499b", "#c82246", "#ff0198",
    "#937782", "#545454", "#a8a8a8"
};

FoilNotesSettings::Private::Private(QObject* aParent) :
    iNextColorIndex(new MGConfItem(KEY_NEXT_COLOR_INDEX, aParent)),
    iSharedKeyWarning(new MGConfItem(KEY_SHARED_KEY_WARNING, aParent))
{
    QObject::connect(iNextColorIndex, SIGNAL(valueChanged()),
        aParent, SIGNAL(nextColorIndexChanged()));
    QObject::connect(iSharedKeyWarning, SIGNAL(valueChanged()),
        aParent, SIGNAL(sharedKeyWarningChanged()));

    const uint n = sizeof(gAvailableColors)/sizeof(gAvailableColors[0]);
    iAvailableColors.reserve(n);
    for (uint i = 0; i < n; i++) {
        iAvailableColors.append(QLatin1String(gAvailableColors[i]));
    }
}

// ==========================================================================
// FoilNotesSettings
// ==========================================================================

FoilNotesSettings::FoilNotesSettings(QObject* aParent) :
    QObject(aParent),
    iPrivate(new Private(this))
{
}

FoilNotesSettings::~FoilNotesSettings()
{
    delete iPrivate;
}

// Callback for qmlRegisterSingletonType<FoilNotesSettings>
QObject*
FoilNotesSettings::createSingleton(
    QQmlEngine* aEngine,
    QJSEngine* aScript)
{
    return new FoilNotesSettings(aEngine);
}

// availableColors

QStringList
FoilNotesSettings::availableColors() const
{
    return iPrivate->iAvailableColors;
}

// nextColorIndex

int
FoilNotesSettings::nextColorIndex() const
{
    return iPrivate->iNextColorIndex->value(DEFAULT_NEXT_COLOR_INDEX).toInt() %
        iPrivate->iAvailableColors.count();
}

void
FoilNotesSettings::setNextColorIndex(
    int aValue)
{
    HDEBUG(aValue);
    iPrivate->iNextColorIndex->set(aValue %
        iPrivate->iAvailableColors.count());
}

int
FoilNotesSettings::pickColorIndex()
{
    const int index = nextColorIndex();
    setNextColorIndex(index + 1);
    return index;
}

QColor
FoilNotesSettings::pickColor()
{
    return QColor(iPrivate->iAvailableColors.at(pickColorIndex()));
}

// sharedKeyWarning

bool
FoilNotesSettings::sharedKeyWarning() const
{
    return iPrivate->iSharedKeyWarning->value(DEFAULT_SHARED_KEY_WARNING).toBool();
}

void
FoilNotesSettings::setSharedKeyWarning(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iSharedKeyWarning->set(aValue);
}
