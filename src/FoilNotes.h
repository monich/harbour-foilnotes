/*
 * Copyright (C) 2018-2022 Jolla Ltd.
 * Copyright (C) 2018-2022 Slava Monich <slava@monich.com>
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

#ifndef FOILNOTES_H
#define FOILNOTES_H

#include <QObject>
#include <QString>
#include <QColor>

class QQmlEngine;
class QJSEngine;

// Static utilities
class FoilNotes : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool otherFoilAppsInstalled READ otherFoilAppsInstalled NOTIFY otherFoilAppsInstalledChanged)
    Q_PROPERTY(qreal opacityFaint READ opacityFaint CONSTANT)
    Q_PROPERTY(qreal opacityLow READ opacityLow CONSTANT)
    Q_PROPERTY(qreal opacityHigh READ opacityHigh CONSTANT)
    Q_PROPERTY(qreal opacityOverlay READ opacityOverlay CONSTANT)

public:
    explicit FoilNotes(QObject* aParent = Q_NULLPTR);

    // Callback for qmlRegisterSingletonType<FoilNotes>
    static QObject* createSingleton(QQmlEngine* aEngine, QJSEngine* aScript);

    Q_INVOKABLE static QString generateFileName(QString aText);
    Q_INVOKABLE static qreal colorDifference(const QColor& aColor1, const QColor& aColor2);

    bool otherFoilAppsInstalled() const;

    static qreal opacityFaint() { return 0.2; }
    static qreal opacityLow() { return 0.4; }
    static qreal opacityHigh() { return 0.6; }
    static qreal opacityOverlay() { return 0.8; }

Q_SIGNALS:
    void otherFoilAppsInstalledChanged();

private:
    class Private;
    Private* iPrivate;
};

#endif // FOILNOTES_H
