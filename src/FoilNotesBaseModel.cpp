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

#include "FoilNotesBaseModel.h"


// ==========================================================================
// FoilNotesBaseModel::Note
// ==========================================================================

const QString FoilNotesBaseModel::Note::PAGENR("pagenr");
const QString FoilNotesBaseModel::Note::COLOR("color");
const QString FoilNotesBaseModel::Note::BODY("body");

FoilNotesBaseModel::Note::Note() :
    iPageNr(0)
{
}

FoilNotesBaseModel::Note::Note(const Note& aNote) :
    iPageNr(aNote.iPageNr),
    iColor(aNote.iColor),
    iBody(aNote.iBody)
{
}

FoilNotesBaseModel::Note::Note(int aPageNr, QColor aColor, QString aBody) :
    iPageNr(aPageNr),
    iColor(aColor),
    iBody(aBody)
{
}

FoilNotesBaseModel::Note& FoilNotesBaseModel::Note::operator=(const Note& aNote)
{
    iPageNr = aNote.iPageNr;
    iColor = aNote.iColor;
    iBody = aNote.iBody;
    return *this;
}

bool FoilNotesBaseModel::Note::equals(const Note& aNote) const
{
    return iPageNr == aNote.iPageNr &&
        iColor == aNote.iColor &&
        iBody == aNote.iBody;
}

QVariantMap FoilNotesBaseModel::Note::toVariant() const
{
    QVariantMap map;
    map.insert(PAGENR, iPageNr);
    map.insert(COLOR, iColor);
    map.insert(BODY, iBody);
    return map;
}

// ==========================================================================
// FoilNotesBaseModel
// ==========================================================================

#define SUPER QAbstractListModel

FoilNotesBaseModel::FoilNotesBaseModel(QObject* aParent) :
    SUPER(aParent)
{
}

Qt::ItemFlags FoilNotesBaseModel::flags(const QModelIndex& aIndex) const
{
    return SUPER::flags(aIndex) | Qt::ItemIsEditable;
}

void FoilNotesBaseModel::selectAll()
{
    const int n = rowCount(QModelIndex());
    if (n > 0 && selectedCount() < n) {
        for (int i = 0; i < n; i++) {
            if (!selectedAt(i)) {
                setSelectedAt(i, true);
                if (selectedCount() == n) {
                    break;
                }
            }
        }
        emitSelectedChanged();
    }
}

void FoilNotesBaseModel::clearSelection()
{
    if (selectedCount() > 0) {
        const int n = rowCount(QModelIndex());
        for (int i = 0; i < n; i++) {
            if (selectedAt(i)) {
                setSelectedAt(i, false);
                if (!selectedCount()) {
                    break;
                }
            }
        }
        emitSelectedChanged();
    }
}

QList<int> FoilNotesBaseModel::selectedRows() const
{
    QList<int> rows;
    const int selected = selectedCount();
    if (selected > 0) {
        rows.reserve(selected);
        const int n = rowCount(QModelIndex());
        for (int i = 0; i < n && rows.count() < selected; i++) {
            if (selectedAt(i)) {
                rows.append(i);
            }
        }
    }
    return rows;
}

QVariantMap FoilNotesBaseModel::get(int aIndex) const
{
    const Note* note = noteAt(aIndex);
    return note ? note->toVariant() : QVariantMap();
}

void FoilNotesBaseModel::emitSelectedChanged()
{
    Q_EMIT selectedChanged();
}
