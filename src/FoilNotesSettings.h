/*
 * Copyright (C) 2018-2021 Jolla Ltd.
 * Copyright (C) 2018-2021 Slava Monich <slava@monich.com>
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
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other materials provided with the
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

#ifndef FOILNOTES_SETTINGS_H
#define FOILNOTES_SETTINGS_H

#include <QObject>
#include <QColor>

class QQmlEngine;
class QJSEngine;

class FoilNotesSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList defaultColors READ defaultColors CONSTANT)
    Q_PROPERTY(QStringList availableColors READ availableColors WRITE setAvailableColors NOTIFY availableColorsChanged)
    Q_PROPERTY(int nextColorIndex READ nextColorIndex WRITE setNextColorIndex NOTIFY nextColorIndexChanged)
    Q_PROPERTY(bool sharedKeyWarning READ sharedKeyWarning WRITE setSharedKeyWarning NOTIFY sharedKeyWarningChanged)
    Q_PROPERTY(bool sharedKeyWarning2 READ sharedKeyWarning2 WRITE setSharedKeyWarning2 NOTIFY sharedKeyWarning2Changed)
    Q_PROPERTY(int autoLockTime READ autoLockTime WRITE setAutoLockTime NOTIFY autoLockTimeChanged)
    Q_PROPERTY(bool plaintextView READ plaintextView WRITE setPlaintextView NOTIFY plaintextViewChanged)

public:
    explicit FoilNotesSettings(QObject* aParent = Q_NULLPTR);
    ~FoilNotesSettings();

    // Callback for qmlRegisterSingletonType<FoilNotesSettings>
    static QObject* createSingleton(QQmlEngine* aEngine, QJSEngine* aScript);

    const QStringList defaultColors() const;
    QStringList availableColors() const;
    void setAvailableColors(QStringList aColors);

    int nextColorIndex() const;
    void setNextColorIndex(int aValue);

    bool sharedKeyWarning() const;
    bool sharedKeyWarning2() const;
    void setSharedKeyWarning(bool aValue);
    void setSharedKeyWarning2(bool aValue);

    int autoLockTime() const;
    void setAutoLockTime(int aValue);

    bool plaintextView() const;
    void setPlaintextView(bool aValue);

    Q_INVOKABLE int pickColorIndex();
    Q_INVOKABLE QColor pickColor();

Q_SIGNALS:
    void availableColorsChanged();
    void nextColorIndexChanged();
    void sharedKeyWarningChanged();
    void sharedKeyWarning2Changed();
    void autoLockTimeChanged();
    void plaintextViewChanged();

private:
    class Private;
    Private* iPrivate;
};

#endif // FOILNOTES_SETTINGS_H
