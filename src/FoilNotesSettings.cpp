/*
 * Copyright (C) 2018-2024 Slava Monich <slava@monich.com>
 * Copyright (C) 2018-2021 Jolla Ltd.
 *
 * You may use this file under the terms of the BSD license as follows:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer
 *     in the documentation and/or other materials provided with the
 *     distribution.
 *
 *  3. Neither the names of the copyright holders nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * any official policies, either expressed or implied.
 */

#include "FoilNotesSettings.h"
#include "FoilNotesDefs.h"

#include "HarbourDebug.h"

#include <MGConfItem>

#define DCONF_KEY(x)                FOILNOTES_DCONF_ROOT x
#define KEY_AVAILABLE_COLORS        DCONF_KEY("availableColors")
#define KEY_NEXT_COLOR_INDEX        DCONF_KEY("nextColorIndex")
#define KEY_SHARED_KEY_WARNING      DCONF_KEY("sharedKeyWarning")
#define KEY_SHARED_KEY_WARNING2     DCONF_KEY("sharedKeyWarning2")
#define KEY_PLAINTEXT_VIEW          DCONF_KEY("plaintextView")
#define KEY_GRID_FONT_SIZE          DCONF_KEY("gridFontSize")
#define KEY_EDITOR_FINT_SIZE        DCONF_KEY("editorFontSize")
#define KEY_AUTO_LOCK               DCONF_KEY("autoLock")
#define KEY_AUTO_LOCK_TIME          DCONF_KEY("autoLockTime")

#define DEFAULT_NEXT_COLOR_INDEX    0
#define DEFAULT_SHARED_KEY_WARNING  true
#define DEFAULT_PLAINTEXT_VIEW      false
#define DEFAULT_GRID_FONT_SIZE      QStringLiteral("small")
#define DEFAULT_EDITOR_FONT_SIZE    QStringLiteral("medium")
#define DEFAULT_AUTO_LOCK           true
#define DEFAULT_AUTO_LOCK_TIME      15000

// ==========================================================================
// FoilNotesSettings::Private
// ==========================================================================

class FoilNotesSettings::Private :
    public QObject
{
    Q_OBJECT
    static const char* gAvailableColors[];

public:
    Private(QObject* aParent);

    static QStringList defaultColors();
    QStringList availableColors() const;

public Q_SLOTS:
    void onAvailableColorsChanged();

public:
    MGConfItem* iAvailableColorsConf;
    MGConfItem* iNextColorIndex;
    MGConfItem* iSharedKeyWarning;
    MGConfItem* iSharedKeyWarning2;
    MGConfItem* iPlainTextView;
    MGConfItem* iGridFontSize;
    MGConfItem* iEditorFontSize;
    MGConfItem* iAutoLock;
    MGConfItem* iAutoLockTime;
    const QStringList iDefaultColors;
    QStringList iAvailableColors;
    QVariant iDefaultSharedKeyWarning;
    QVariant iDefaultAutoLockTime;
};

const char* FoilNotesSettings::Private::gAvailableColors[] = {
    "#00aff0", "#007fc4", "#00569b",
    "#f13c27", "#fedc00", "#f78628",
    "#4fb548", "#01823f", "#007f7f",
    "#7d499b", "#c82246", "#ff0198",
    "#937782", "#545454", "#a8a8a8"
};

FoilNotesSettings::Private::Private(QObject* aParent) :
    QObject(aParent),
    iAvailableColorsConf(new MGConfItem(KEY_AVAILABLE_COLORS, aParent)),
    iNextColorIndex(new MGConfItem(KEY_NEXT_COLOR_INDEX, aParent)),
    iSharedKeyWarning(new MGConfItem(KEY_SHARED_KEY_WARNING, aParent)),
    iSharedKeyWarning2(new MGConfItem(KEY_SHARED_KEY_WARNING2, aParent)),
    iPlainTextView(new MGConfItem(KEY_PLAINTEXT_VIEW, aParent)),
    iGridFontSize(new MGConfItem(KEY_GRID_FONT_SIZE, aParent)),
    iEditorFontSize(new MGConfItem(KEY_EDITOR_FINT_SIZE, aParent)),
    iAutoLock(new MGConfItem(KEY_AUTO_LOCK, aParent)),
    iAutoLockTime(new MGConfItem(KEY_AUTO_LOCK_TIME, aParent)),
    iDefaultColors(defaultColors()),
    iDefaultSharedKeyWarning(DEFAULT_SHARED_KEY_WARNING),
    iDefaultAutoLockTime(DEFAULT_AUTO_LOCK_TIME)
{
    QObject::connect(iAvailableColorsConf, SIGNAL(valueChanged()),
        SLOT(onAvailableColorsChanged()));
    QObject::connect(iNextColorIndex, SIGNAL(valueChanged()),
        aParent, SIGNAL(nextColorIndexChanged()));
    QObject::connect(iSharedKeyWarning, SIGNAL(valueChanged()),
        aParent, SIGNAL(sharedKeyWarningChanged()));
    QObject::connect(iSharedKeyWarning2, SIGNAL(valueChanged()),
        aParent, SIGNAL(sharedKeyWarning2Changed()));
    QObject::connect(iPlainTextView, SIGNAL(valueChanged()),
        aParent, SIGNAL(plaintextViewChanged()));
    QObject::connect(iGridFontSize, SIGNAL(valueChanged()),
        aParent, SIGNAL(gridFontSizeChanged()));
    QObject::connect(iEditorFontSize, SIGNAL(valueChanged()),
        aParent, SIGNAL(editorFontSizeChanged()));
    QObject::connect(iAutoLock, SIGNAL(valueChanged()),
        aParent, SIGNAL(autoLockChanged()));
    QObject::connect(iAutoLockTime, SIGNAL(valueChanged()),
        aParent, SIGNAL(autoLockTimeChanged()));
    iAvailableColors = availableColors();
}

QStringList
FoilNotesSettings::Private::defaultColors()
{
    QStringList colors;
    const uint n = sizeof(gAvailableColors)/sizeof(gAvailableColors[0]);
    colors.reserve(n);
    for (uint i = 0; i < n; i++) {
        colors.append(QLatin1String(gAvailableColors[i]));
    }
    return colors;
}

QStringList
FoilNotesSettings::Private::availableColors() const
{
    return iAvailableColorsConf->value(iDefaultColors).toStringList();
}

void
FoilNotesSettings::Private::onAvailableColorsChanged()
{
    const QStringList newColors(availableColors());
    if (iAvailableColors != newColors) {
        iAvailableColors = newColors;
        Q_EMIT qobject_cast<FoilNotesSettings*>(parent())->availableColorsChanged();
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
    QQmlEngine*,
    QJSEngine*)
{
    return new FoilNotesSettings();
}

// defaultColors
const QStringList
FoilNotesSettings::defaultColors() const
{
    return iPrivate->iDefaultColors;
}

// availableColors

QStringList
FoilNotesSettings::availableColors() const
{
    return iPrivate->iAvailableColors;
}

void
FoilNotesSettings::setAvailableColors(
    QStringList aColors)
{
    iPrivate->iAvailableColorsConf->set(aColors);
    if (iPrivate->iAvailableColors != aColors) {
        Q_EMIT availableColorsChanged();
    }
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
    iPrivate->iNextColorIndex->set(aValue % iPrivate->iAvailableColors.count());
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
// sharedKeyWarning2

bool
FoilNotesSettings::sharedKeyWarning() const
{
    return iPrivate->iSharedKeyWarning->value(iPrivate->
        iDefaultSharedKeyWarning).toBool();
}

bool
FoilNotesSettings::sharedKeyWarning2() const
{
    return iPrivate->iSharedKeyWarning2->value(iPrivate->
        iDefaultSharedKeyWarning).toBool();
}

void
FoilNotesSettings::setSharedKeyWarning(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iSharedKeyWarning->set(aValue);
}

void
FoilNotesSettings::setSharedKeyWarning2(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iSharedKeyWarning2->set(aValue);
}

// plaintextView

bool
FoilNotesSettings::plaintextView() const
{
    return iPrivate->iPlainTextView->value(DEFAULT_PLAINTEXT_VIEW).toBool();
}

void
FoilNotesSettings::setPlaintextView(
    bool aValue)
{
    HDEBUG(aValue);
    iPrivate->iPlainTextView->set(aValue);
}

// gridFontSize

QString
FoilNotesSettings::gridFontSize() const
{
    return iPrivate->iGridFontSize->value(DEFAULT_GRID_FONT_SIZE).toString();
}

// editorFontSize

QString
FoilNotesSettings::editorFontSize() const
{
    return iPrivate->iEditorFontSize->value(DEFAULT_EDITOR_FONT_SIZE).toString();
}

// autoLock

bool
FoilNotesSettings::autoLock() const
{
    return iPrivate->iAutoLock->value(DEFAULT_AUTO_LOCK).toBool();
}

// autoLockTime

int
FoilNotesSettings::autoLockTime() const
{
    QVariant val(iPrivate->iAutoLockTime->value(iPrivate->iDefaultAutoLockTime));
    bool ok;
    const int ival(val.toInt(&ok));
    return (ok && ival >= 0) ? ival : DEFAULT_AUTO_LOCK_TIME;
}

#include "FoilNotesSettings.moc"
