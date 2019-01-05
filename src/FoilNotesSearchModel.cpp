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

#include "FoilNotesSearchModel.h"
#include "FoilNotesBaseModel.h"

#include "HarbourDebug.h"

// ==========================================================================
// FoilNotesSearchModel::Private
// ==========================================================================

class FoilNotesSearchModel::Private : public QObject {
    Q_OBJECT

public:
    static const char PROP_TEXT_INDEX[];

    Private(FoilNotesSearchModel* aParent);

    static int findRole(QAbstractItemModel* aModel, QString aRole);

    FoilNotesSearchModel* parentModel();
    int mapToSource(int aRow) const;
    int mapFromSource(int aRow) const;

    void updateFilterRole();

public Q_SLOTS:
    void updateTextIndex();

public:
    QString iFilterRole;
    int iTextIndex;
};

const char FoilNotesSearchModel::Private::PROP_TEXT_INDEX[] = "textIndex";

FoilNotesSearchModel::Private::Private(FoilNotesSearchModel* aParent) :
    QObject(aParent),
    iTextIndex(-1)
{
}

inline FoilNotesSearchModel* FoilNotesSearchModel::Private::parentModel()
{
    return qobject_cast<FoilNotesSearchModel*>(parent());
}

int FoilNotesSearchModel::Private::findRole(QAbstractItemModel* aModel, QString aRole)
{
    if (aModel && !aRole.isEmpty()) {
        const QByteArray roleName(aRole.toUtf8());
        const QHash<int,QByteArray> roleMap(aModel->roleNames());
        const QList<int> roles(roleMap.keys());
        const int n = roles.count();
        for (int i = 0; i < n; i++) {
            const QByteArray name(roleMap.value(roles.at(i)));
            if (name == roleName) {
                HDEBUG(aRole << roles.at(i));
                return roles.at(i);
            }
        }
        HDEBUG("Unknown role" << aRole);
    }
    return -1;
}

void FoilNotesSearchModel::Private::updateFilterRole()
{
    QSortFilterProxyModel* model = parentModel();
    const int role = findRole(model->sourceModel(), iFilterRole);
    model->setFilterRole((role >= 0) ? role : Qt::DisplayRole);
}

void FoilNotesSearchModel::Private::updateTextIndex()
{
    FoilNotesSearchModel* model = parentModel();
    QAbstractItemModel* source = model->sourceModel();
    const int oldTextIndex = iTextIndex;
    if (source) {
        bool ok;
        int value = source->property(PROP_TEXT_INDEX).toInt(&ok);
        if (ok) {
            QModelIndex index = model->mapFromSource(source->index(value, 0));
            if (index.isValid()) {
                iTextIndex = index.row();
            }
        }
    }
    if (oldTextIndex != iTextIndex) {
        HDEBUG(oldTextIndex << "->" << iTextIndex);
        Q_EMIT model->textIndexChanged();
    }
}

// ==========================================================================
// FoilNotesSearchModel
// ==========================================================================

#define SUPER QSortFilterProxyModel

FoilNotesSearchModel::FoilNotesSearchModel(QObject* aParent) :
    SUPER(aParent),
    iPrivate(new Private(this))
{
    setDynamicSortFilter(true);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

FoilNotesSearchModel::~FoilNotesSearchModel()
{
    delete iPrivate;
}

void FoilNotesSearchModel::setSourceModelObject(QObject* aModel)
{
    setSourceModel(qobject_cast<QAbstractItemModel*>(aModel));
}

void FoilNotesSearchModel::setSourceModel(QAbstractItemModel* aModel)
{
    QAbstractItemModel* source = sourceModel();
    if (source != aModel) {
        if (source) {
            disconnect(source, NULL, iPrivate, SLOT(updateTextIndex()));
        }
        SUPER::setSourceModel(aModel);
        iPrivate->updateFilterRole();
        if (aModel) {
            connect(aModel, SIGNAL(textIndexChanged()),
                iPrivate, SLOT(updateTextIndex()));
            connect(aModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                iPrivate, SLOT(updateTextIndex()));
            connect(aModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                iPrivate, SLOT(updateTextIndex()));
            connect(aModel, SIGNAL(modelReset()),
                iPrivate, SLOT(updateTextIndex()));
        }
    }
}

QString FoilNotesSearchModel::filterRoleName() const
{
    return iPrivate->iFilterRole;
}

void FoilNotesSearchModel::setFilterRoleName(QString aRoleName)
{
    if (iPrivate->iFilterRole != aRoleName) {
        iPrivate->iFilterRole = aRoleName;
        iPrivate->updateFilterRole();
        Q_EMIT filterRoleNameChanged();
    }
}

int FoilNotesSearchModel::textIndex() const
{
    return iPrivate->iTextIndex;
}

void FoilNotesSearchModel::setTextIndex(int aIndex)
{
    QObject* source = sourceModel();
    if (source) {
        QModelIndex sourceIndex = mapToSource(index(aIndex, 0));
        if (sourceIndex.isValid()) {
            source->setProperty(Private::PROP_TEXT_INDEX, sourceIndex.row());
        }
    }
}

void FoilNotesSearchModel::deleteNoteAt(int aRow)
{
    HDEBUG(aRow);
    FoilNotesBaseModel* source = qobject_cast<FoilNotesBaseModel*>(sourceModel());
    if (source) {
        QModelIndex sourceIndex = mapToSource(index(aRow, 0));
        if (sourceIndex.isValid()) {
            source->deleteNoteAt(sourceIndex.row());
        }
    }
}

#include "FoilNotesSearchModel.moc"
