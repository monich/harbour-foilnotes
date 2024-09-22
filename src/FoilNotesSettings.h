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

#ifndef FOILNOTES_SETTINGS_H
#define FOILNOTES_SETTINGS_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtGui/QColor>

class QQmlEngine;
class QJSEngine;

class FoilNotesSettings :
    public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList defaultColors READ defaultColors CONSTANT)
    Q_PROPERTY(QStringList availableColors READ availableColors WRITE setAvailableColors NOTIFY availableColorsChanged)
    Q_PROPERTY(int nextColorIndex READ nextColorIndex WRITE setNextColorIndex NOTIFY nextColorIndexChanged)
    Q_PROPERTY(bool sharedKeyWarning READ sharedKeyWarning WRITE setSharedKeyWarning NOTIFY sharedKeyWarningChanged)
    Q_PROPERTY(bool sharedKeyWarning2 READ sharedKeyWarning2 WRITE setSharedKeyWarning2 NOTIFY sharedKeyWarning2Changed)
    Q_PROPERTY(bool plaintextView READ plaintextView WRITE setPlaintextView NOTIFY plaintextViewChanged)
    Q_PROPERTY(QString gridFontSize READ gridFontSize NOTIFY gridFontSizeChanged)
    Q_PROPERTY(QString editorFontSize READ editorFontSize NOTIFY editorFontSizeChanged)
    Q_PROPERTY(bool autoLock READ autoLock NOTIFY autoLockChanged)
    Q_PROPERTY(int autoLockTime READ autoLockTime NOTIFY autoLockTimeChanged)

public:
    explicit FoilNotesSettings(QObject* aParent = Q_NULLPTR);
    ~FoilNotesSettings();

    // Callback for qmlRegisterSingletonType<FoilNotesSettings>
    static QObject* createSingleton(QQmlEngine*, QJSEngine*);

    const QStringList defaultColors() const;
    QStringList availableColors() const;
    void setAvailableColors(QStringList);

    int nextColorIndex() const;
    void setNextColorIndex(int);

    bool sharedKeyWarning() const;
    bool sharedKeyWarning2() const;
    void setSharedKeyWarning(bool);
    void setSharedKeyWarning2(bool);

    bool plaintextView() const;
    void setPlaintextView(bool);

    QString gridFontSize() const;
    QString editorFontSize() const;
    bool autoLock() const;
    int autoLockTime() const;

    Q_INVOKABLE int pickColorIndex();
    Q_INVOKABLE QColor pickColor();

Q_SIGNALS:
    void availableColorsChanged();
    void nextColorIndexChanged();
    void sharedKeyWarningChanged();
    void sharedKeyWarning2Changed();
    void plaintextViewChanged();
    void gridFontSizeChanged();
    void editorFontSizeChanged();
    void autoLockChanged();
    void autoLockTimeChanged();

private:
    class Private;
    Private* iPrivate;
};

#endif // FOILNOTES_SETTINGS_H
