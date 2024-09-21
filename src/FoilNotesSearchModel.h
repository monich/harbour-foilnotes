/*
 * Copyright (C) 2018-2024 Slava Monich <slava@monich.com>
 * Copyright (C) 2018-2019 Jolla Ltd.
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

#ifndef FOILNOTES_SEARCH_MODEL_H
#define FOILNOTES_SEARCH_MODEL_H

#include <QtCore/QSortFilterProxyModel>
#include <QtGui/QColor>
#include <QtQml>

class FoilNotesSearchModel :
    public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterRoleName READ filterRoleName WRITE setFilterRoleName NOTIFY filterRoleNameChanged)
    Q_PROPERTY(QObject* sourceModel READ sourceModel WRITE setSourceModelObject NOTIFY sourceModelChanged)
    Q_PROPERTY(int textIndex READ textIndex WRITE setTextIndex NOTIFY textIndexChanged) // Expected by NotesGridView

public:
    FoilNotesSearchModel(QObject* aParent = NULL);
    ~FoilNotesSearchModel();

    void setSourceModelObject(QObject*);
    void setSourceModel(QAbstractItemModel*) Q_DECL_OVERRIDE;

    QString filterRoleName() const;
    void setFilterRoleName(QString);

    int textIndex() const;
    void setTextIndex(int aIndex);

    Q_INVOKABLE QVariantMap get(int) const;
    Q_INVOKABLE int sourceRow(int) const;

    // These are expected by NotesGridView
    Q_INVOKABLE void addNote(QColor, QString);
    Q_INVOKABLE void setBodyAt(int, QString);
    Q_INVOKABLE void setColorAt(int, QColor);
    Q_INVOKABLE void deleteNoteAt(int);

Q_SIGNALS:
    void filterRoleNameChanged();
    void sourceModelChanged();
    void textIndexChanged();

private:
    class Private;
    Private* iPrivate;
};

QML_DECLARE_TYPE(FoilNotesSearchModel)

#endif // FOILNOTES_SEARCH_MODEL_H
