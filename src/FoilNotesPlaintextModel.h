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

#ifndef FOILNOTES_PLAINTEXT_MODEL_H
#define FOILNOTES_PLAINTEXT_MODEL_H

#include "FoilNotesBaseModel.h"

#include <QtQml>

class FoilNotesPlaintextModel: public FoilNotesBaseModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int textIndex READ textIndex WRITE setTextIndex NOTIFY textIndexChanged)
    Q_PROPERTY(QString text READ text NOTIFY textChanged)

public:
    FoilNotesPlaintextModel(QObject* aParent = NULL);

    // FoilNotesBaseModel
    void addNote(QColor aColor, QString aBody) Q_DECL_OVERRIDE;
    void deleteNoteAt(int aIndex) Q_DECL_OVERRIDE;
    void setBodyAt(int aRow, QString aBody) Q_DECL_OVERRIDE;
    void setColorAt(int aRow, QColor aColor) Q_DECL_OVERRIDE;
    void deleteNotes(QList<int> aRows) Q_DECL_OVERRIDE;

    const Note* noteAt(int aRow) const Q_DECL_OVERRIDE;
    int selectedCount() const Q_DECL_OVERRIDE;
    bool selectedAt(int aRow) const Q_DECL_OVERRIDE;
    void setSelectedAt(int aRow, bool aSelected) Q_DECL_OVERRIDE;

    // QAbstractItemModel
    QHash<int,QByteArray> roleNames() const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& aParent = QModelIndex()) const Q_DECL_OVERRIDE;
    QVariant data(const QModelIndex& aIndex, int aRole) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex& aIndex, const QVariant& aValue, int aRole) Q_DECL_OVERRIDE;
    bool moveRows(const QModelIndex &aSrcParent, int aSrcRow, int aCount,
        const QModelIndex &aDestParent, int aDestRow) Q_DECL_OVERRIDE;

    // These are specific to plaintext model
    Q_INVOKABLE int startEncryptingAt(int aRow);
    Q_INVOKABLE void saveNote(int aPageNr, QColor aColor, QString aBody);

    QString text() const;
    int textIndex() const;
    void setTextIndex(int aIndex);

    // Callback for qmlRegisterSingletonType<FoilNotesPlaintextModel>
    static QObject* createSingleton(QQmlEngine* aEngine, QJSEngine* aScript);

public Q_SLOTS:
    void onEncryptionDone(int requestId, bool success);

Q_SIGNALS:
    void countChanged();
    void textIndexChanged();
    void textChanged();

private:
    class ModelData;
    class Private;
    Private* iPrivate;
};

QML_DECLARE_TYPE(FoilNotesPlaintextModel)

#endif // FOILNOTES_PLAINTEXT_MODEL_H
