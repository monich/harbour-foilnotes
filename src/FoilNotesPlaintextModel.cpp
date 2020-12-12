/*
 * Copyright (C) 2018-2020 Jolla Ltd.
 * Copyright (C) 2018-2020 Slava Monich <slava@monich.com>
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

#include "FoilNotesPlaintextModel.h"

#include "HarbourSystemInfo.h"
#include "HarbourDebug.h"

#include <QTimer>
#include <QAtomicInt>
#include <QColor>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QFileSystemWatcher>

#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>

// ==========================================================================
// FoilNotesPlaintextModel::ModelData
// ==========================================================================

class FoilNotesPlaintextModel::ModelData {
public:
    typedef QList<ModelData> List;

    ModelData();
    ModelData(int aPageNr, QColor aColor, QString aBody);
    ModelData(const ModelData& aData);

    ModelData& operator=(const ModelData& aData);
    QString body() const { return iNote.iBody; }
    QColor color() const { return iNote.iColor; }
    uint pagenr() const { return iNote.iPageNr; }
    bool isBusy() const { return iEncReqId != 0; }

    bool equals(const ModelData& aData) const;
    bool operator==(const ModelData& aData) const { return equals(aData); }
    bool operator!=(const ModelData& aData) const { return !equals(aData); }

public:
    Note iNote;
    bool iSelected;
    int iEncReqId;
};

FoilNotesPlaintextModel::ModelData::ModelData() :
    iSelected(false),
    iEncReqId(0)
{
}

FoilNotesPlaintextModel::ModelData::ModelData(int aPageNr, QColor aColor, QString aBody) :
    iNote(aPageNr, aColor, aBody),
    iSelected(false),
    iEncReqId(0)
{
}

FoilNotesPlaintextModel::ModelData::ModelData(const ModelData& aData) :
    iNote(aData.iNote),
    iSelected(aData.iSelected),
    iEncReqId(aData.iEncReqId)
{
}

FoilNotesPlaintextModel::ModelData&
FoilNotesPlaintextModel::ModelData::operator=(const ModelData& aData)
{
    iNote = aData.iNote;
    iSelected = aData.iSelected;
    iEncReqId = aData.iEncReqId;
    return *this;
}

bool FoilNotesPlaintextModel::ModelData::equals(const ModelData& aData) const
{
    // Ignore iSelected and iEncReqId
    return iNote == aData.iNote;
}

// ==========================================================================
// FoilNotesPlaintextModel::Private
// ==========================================================================

class FoilNotesPlaintextModel::Private : public QObject {
    Q_OBJECT
public:
    enum {
        FIELD_PAGENR,
        FIELD_COLOR,
        FIELD_BODY,
        NUM_FIELDS
    };

    enum Role {
        PageNrRole = Qt::UserRole,
        ColorRole,
        BodyRole,
        SelectedRole,
        BusyRole
    };

    static const QString DB_TYPE;
    static const QString DB_NAME;
    static const QString DB_TABLE;
    static const QString DB_FIELD[NUM_FIELDS];

    static const int COMMIT_TIMEOUT = 500;

    Private(FoilNotesPlaintextModel* aParent);
    ~Private();

    static QString databaseDir();
    static QString databasePath();
    static QVector<int> diff(const ModelData* aNote1, const ModelData* aNote2);

    int rowCount() const;
    const ModelData* dataAt(const QModelIndex& aIndex) const;
    const ModelData* dataAt(int aIndex) const;
    ModelData* dataAt(const QModelIndex& aIndex);
    ModelData* dataAt(int aIndex);
    void updatePageNr(uint aStartPos);
    void updatePageNr(uint aStartPos, uint aEndPos);
    void dataChanged(int aRow, Role aRole);
    void addNote(QColor aColor, QString aBody);
    void deleteNoteAt(int aRow);
    void openDatabase(bool aNeedEvents = true);
    void scheduleCommit();
    void syncModel();
    void updateText();
    int nextReqId();

    FoilNotesPlaintextModel* parentModel();

public Q_SLOTS:
    void onFileChanged(QString aPath);
    void onDirectoryChanged(QString aPath);
    void commitChanges();

public:
    QSqlDatabase iDatabase;
    QString iDatabasePath;
    QFileSystemWatcher* iFileWatcher;
    QTimer* iCommitTimer;
    int iColumn[NUM_FIELDS];
    int iIgnoreFileChange;
    int iIgnoreDirChange;
    int iSelected;
    ModelData::List iModelData;
    ModelData::List iDatabaseData;
    QAtomicInt iNextReqId;
    int iTextIndex;
    QString iText;
};

#define TABLE_NAME        "notes"
#define FIELD_NAME_PAGENR "pagenr"
#define FIELD_NAME_COLOR  "color"
#define FIELD_NAME_BODY   "body"

const QString FoilNotesPlaintextModel::Private::DB_TYPE("QSQLITE");
const QString FoilNotesPlaintextModel::Private::DB_NAME("silicanotes");
const QString FoilNotesPlaintextModel::Private::DB_TABLE(TABLE_NAME);
const QString FoilNotesPlaintextModel::Private::DB_FIELD[] = {
    FIELD_NAME_PAGENR, FIELD_NAME_COLOR, FIELD_NAME_BODY
};

#define DB_FIELD_PAGENR DB_FIELD[FoilNotesPlaintextModel::Private::FIELD_PAGENR]
#define DB_FIELD_COLOR DB_FIELD[FoilNotesPlaintextModel::Private::FIELD_COLOR]
#define DB_FIELD_BODY DB_FIELD[FoilNotesPlaintextModel::Private::FIELD_BODY]

FoilNotesPlaintextModel::Private::Private(FoilNotesPlaintextModel* aModel) :
    QObject(aModel),
    iDatabase(QSqlDatabase::database(DB_NAME)),
    iDatabasePath(databasePath()),
    iFileWatcher(new QFileSystemWatcher(this)),
    iCommitTimer(new QTimer(this)),
    iIgnoreFileChange(0),
    iIgnoreDirChange(0),
    iSelected(0),
    iNextReqId(1),
    iTextIndex(-1)
{
    // No fields yet
    for (int i = 0; i < NUM_FIELDS; i++) iColumn[i] = -1;

    // Commit timer
    iCommitTimer->setSingleShot(true);
    iCommitTimer->setInterval(COMMIT_TIMEOUT);
    connect(iCommitTimer, SIGNAL(timeout()),
        SLOT(commitChanges()));

    // File watcher
    connect(iFileWatcher, SIGNAL(fileChanged(QString)),
        SLOT(onFileChanged(QString)));
    connect(iFileWatcher, SIGNAL(directoryChanged(QString)),
        SLOT(onDirectoryChanged(QString)));
    iFileWatcher->addPath(databaseDir());
    if (QFile::exists(iDatabasePath)) {
        HDEBUG("watching" << qPrintable(iDatabasePath));
        iFileWatcher->addPath(iDatabasePath);
    }

    // Database
    if (!iDatabase.isValid()) {
        HDEBUG("adding database" << DB_NAME);
        iDatabase = QSqlDatabase::addDatabase(DB_TYPE, DB_NAME);
    }
    iDatabase.setDatabaseName(iDatabasePath);
    openDatabase(false);
    updateText();
}

FoilNotesPlaintextModel::Private::~Private()
{
    if (iCommitTimer->isActive()) {
        HDEBUG("commit timer was running, commiting the changes");
        commitChanges();
    }
}

inline FoilNotesPlaintextModel* FoilNotesPlaintextModel::Private::parentModel()
{
    return qobject_cast<FoilNotesPlaintextModel*>(parent());
}

QString FoilNotesPlaintextModel::Private::databaseDir()
{
    static QString sDatabasePath;
    if (sDatabasePath.isEmpty()) {
        // Original path:
        // ~/.local/share/jolla-notes/QML/OfflineStorage/Databases
        // Sailfish OS 4.0.1 and later:
        // ~/.local/share/com.jolla/notes/QML/OfflineStorage/Databases
        QString top(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
        QDir dir(top + "/" + ((HarbourSystemInfo::osVersionCompareWith("4.0.1") >= 0) ?
            "com.jolla/notes" : "jolla-notes") + "/QML/OfflineStorage/Databases");
        sDatabasePath = dir.path();
        HDEBUG("Jolla Notes data directory" << qPrintable(sDatabasePath));
        if (!dir.exists()) {
            dir.mkpath(".");
        }
    }
    return sDatabasePath;
}

QString FoilNotesPlaintextModel::Private::databasePath()
{
    // This is how LocalStorage plugin generates database file name
    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(DB_NAME.toUtf8());
    const QString dbId(QLatin1String(md5.result().toHex()));
    return databaseDir() + "/" + dbId + ".sqlite";
}

void FoilNotesPlaintextModel::Private::openDatabase(bool aNeedEvents)
{
    if (!iDatabase.isOpen()) {
        if (iDatabase.open()) {
            QStringList tables = iDatabase.tables();
            if (tables.contains(DB_TABLE)) {
                HDEBUG("opened" << iDatabasePath);
            } else {
                HDEBUG("initializing" << iDatabasePath);
                QSqlQuery query(iDatabase);
                if (!query.exec("CREATE TABLE " TABLE_NAME " ("
                    FIELD_NAME_PAGENR " INTEGER, "
                    FIELD_NAME_COLOR " TEXT, "
                    FIELD_NAME_BODY " TEXT)")) {
                    HWARN(query.lastError());
                }
            }
            QSqlRecord record(iDatabase.record(DB_TABLE));
            for (int i = 0; i < NUM_FIELDS; i++) {
                iColumn[i] = record.indexOf(DB_FIELD[i]);
                HDEBUG(iColumn[i] << DB_FIELD[i]);
            }
            QSqlQuery query(iDatabase);
            iDatabaseData = ModelData::List();
            if (query.exec("SELECT "
                FIELD_NAME_PAGENR ", "
                FIELD_NAME_COLOR ", "
                FIELD_NAME_BODY " FROM "
                TABLE_NAME " ORDER BY "
                FIELD_NAME_PAGENR)) {
                while (query.next()) {
                    iDatabaseData.append(ModelData(query.value(0).toInt(),
                        QColor(query.value(1).toString()),
                        query.value(2).toString()));
                }
            } else {
                HWARN(query.lastError());
            }
            if (aNeedEvents) {
                if (iDatabaseData != iModelData) {
                    syncModel();
                }
            } else {
                iModelData = iDatabaseData;
            }
        } else {
            HWARN(iDatabase.lastError());
        }
    }
}

QVector<int> FoilNotesPlaintextModel::Private::diff(const ModelData* aNote1, const ModelData* aNote2)
{
    QVector<int> roles;
    if (aNote1->iNote.iPageNr != aNote2->iNote.iPageNr) {
        roles.append(PageNrRole);
    }
    if (aNote1->iNote.iColor != aNote2->iNote.iColor) {
        roles.append(ColorRole);
    }
    if (aNote1->iNote.iBody != aNote2->iNote.iBody) {
        roles.append(BodyRole);
    }
    if (aNote1->iSelected != aNote2->iSelected) {
        roles.append(SelectedRole);
    }
    if (aNote1->iEncReqId != aNote2->iEncReqId) {
        roles.append(BusyRole);
    }
    return roles;
}

void FoilNotesPlaintextModel::Private::deleteNoteAt(int aRow)
{
    const int count = iModelData.count();
    HASSERT(aRow >= 0 && aRow < count);
    if (aRow >= 0 && aRow < count) {
        FoilNotesPlaintextModel* model = parentModel();
        model->beginRemoveRows(QModelIndex(), aRow, aRow);
        iModelData.removeAt(aRow);
        model->endRemoveRows();
        updatePageNr(aRow);
        updateText();
        scheduleCommit();
    }
}

void FoilNotesPlaintextModel::Private::syncModel()
{
    FoilNotesPlaintextModel* model = parentModel();
    const int databaseCount = iDatabaseData.count();
    int modelCount = iModelData.count();
    int i = 0;

    // Remove extra rows from the beginning
    if (databaseCount < modelCount) {
        const int removed = modelCount - databaseCount;
        HDEBUG("removing" << removed << "note(s) from the model");
        while (i < databaseCount && iDatabaseData.at(i).equals(iModelData.at(i))) i++;
        model->beginRemoveRows(QModelIndex(), i, i + removed - 1);
        for (int k = 0; k < removed; k++) {
            iModelData.removeAt(i);
        }
        model->endRemoveRows();
        HASSERT(iModelData.count() == databaseCount);
        modelCount = iModelData.count();
    }

    // Update those in the middle
    while (i < modelCount) {
        const ModelData* databaseNote = &iDatabaseData.at(i);
        QVector<int> roles = diff(databaseNote, &iModelData.at(i));
        if (!roles.isEmpty()) {
            HDEBUG("note" << iDatabaseData.at(i).pagenr() << "has changed");
            iModelData.replace(i, *databaseNote);
            QModelIndex modelIndex(model->index(i));
            Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
        }
        i++;
    }

    // Add new ones at the end
    if (databaseCount > modelCount) {
        HDEBUG("adding" << (databaseCount - modelCount) << "note(s) to the model");
        model->beginInsertRows(QModelIndex(), modelCount, databaseCount - 1);
        while (i < databaseCount) {
            iModelData.append(iDatabaseData.at(i++));
        }
        model->endInsertRows();
    }

    // Update cover text
    updateText();
}

inline int FoilNotesPlaintextModel::Private::rowCount() const
{
    return iModelData.count();
}

inline const FoilNotesPlaintextModel::ModelData*
FoilNotesPlaintextModel::Private::dataAt(int aIndex) const
{
    return (aIndex >= 0 && aIndex < iModelData.count()) ? &(iModelData[aIndex]) : NULL;
}

inline const FoilNotesPlaintextModel::ModelData*
FoilNotesPlaintextModel::Private::dataAt(const QModelIndex& aIndex) const
{
    return dataAt(aIndex.row());
}

inline FoilNotesPlaintextModel::ModelData*
FoilNotesPlaintextModel::Private::dataAt(int aIndex)
{
    return (aIndex >= 0 && aIndex < iModelData.count()) ? &(iModelData[aIndex]) : NULL;
}

inline FoilNotesPlaintextModel::ModelData*
FoilNotesPlaintextModel::Private::dataAt(const QModelIndex& aIndex)
{
    return dataAt(aIndex.row());
}

int FoilNotesPlaintextModel::Private::nextReqId()
{
    int id;
    while (!(id = iNextReqId.fetchAndAddRelaxed(1)));
    return id;
}

void FoilNotesPlaintextModel::Private::updatePageNr(uint aStartRow)
{
    updatePageNr(aStartRow, iModelData.count());
}

void FoilNotesPlaintextModel::Private::updatePageNr(uint aStartRow, uint aEndRow)
{
    if (aStartRow < aEndRow) {
        QVector<int> roles;
        roles.append(PageNrRole);
        FoilNotesPlaintextModel* model = parentModel();
        for (uint row = aStartRow; row < aEndRow; row++) {
            ModelData* data = &iModelData[row];
            if (data->pagenr() != row + 1) {
                HDEBUG(data->pagenr() << "->" << (row + 1));
                data->iNote.iPageNr = row + 1;
                QModelIndex modelIndex(model->index(row));
                Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
            }
        }
    }
}

void FoilNotesPlaintextModel::Private::dataChanged(int aRow, Role aRole)
{
    QVector<int> roles;
    roles.append(aRole);
    FoilNotesPlaintextModel* model = parentModel();
    QModelIndex modelIndex(model->index(aRow));
    Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
    scheduleCommit();
}

void FoilNotesPlaintextModel::Private::scheduleCommit()
{
    if (!iCommitTimer->isActive()) {
        iCommitTimer->start();
    }
}

void FoilNotesPlaintextModel::Private::onDirectoryChanged(QString aPath)
{
    if (iIgnoreDirChange) {
        iIgnoreDirChange--;
        HDEBUG("ignoring" << qPrintable(aPath));
    } else {
        HDEBUG(qPrintable(aPath));
        if (QFile::exists(iDatabasePath)) {
            if (!iFileWatcher->files().contains(iDatabasePath)) {
                HDEBUG("watching" << qPrintable(iDatabasePath));
                iFileWatcher->addPath(iDatabasePath);
            }
            openDatabase();
        }
    }
}

void FoilNotesPlaintextModel::Private::onFileChanged(QString aPath)
{
    if (iIgnoreFileChange) {
        iIgnoreFileChange--;
        HDEBUG("ignoring" << qPrintable(aPath));
    } else {
        HDEBUG(qPrintable(aPath));
        iDatabase.close();
        openDatabase();
    }
}

void FoilNotesPlaintextModel::Private::commitChanges()
{
    if (iDatabase.transaction()) {
        iIgnoreDirChange++;
        iIgnoreFileChange++;

        // Check if only text or color has changed for one or more notes
        const int count = iModelData.count();
        bool fullUpdate = true;
        if (count == iDatabaseData.count()) {
            fullUpdate = false;
            for (int i = 0; i < count; i++) {
                const Note& note(iModelData.at(i).iNote);
                const Note& dbNote(iDatabaseData.at(i).iNote);
                if (note.iPageNr != dbNote.iPageNr) {
                    fullUpdate = true;
                    break;
                }
            }
        }

        bool ok = true;
        const QString placeholderPagenr(":" FIELD_NAME_PAGENR);
        const QString placeholderColor(":" FIELD_NAME_COLOR);
        const QString placeholderBody(":" FIELD_NAME_BODY);
        if (fullUpdate) {
            // Delete everything and completely replace the contents
            HDEBUG("Updating all notes");
            QSqlQuery query(iDatabase);
            if (!query.exec("DELETE FROM " TABLE_NAME)) {
                HWARN(query.lastError());
                ok = false;
            }
            for (int i = 0; i < count; i++) {
                if (query.prepare("INSERT INTO " TABLE_NAME " ("
                    FIELD_NAME_PAGENR ", "
                    FIELD_NAME_COLOR ", "
                    FIELD_NAME_BODY ") VALUES (:"
                    FIELD_NAME_PAGENR ", :"
                    FIELD_NAME_COLOR ", :"
                    FIELD_NAME_BODY ")")) {
                    const Note& note(iModelData.at(i).iNote);
                    query.bindValue(placeholderPagenr, note.iPageNr);
                    query.bindValue(placeholderColor, note.iColor);
                    query.bindValue(placeholderBody, note.iBody);
                    if (!query.exec()) {
                        HWARN(query.lastError());
                        ok = false;
                    }
                } else {
                    HWARN(query.lastError());
                    ok = false;
                }
            }
        } else {
            // Update modified notes
            for (int i = 0; i < count; i++) {
                const Note& note(iModelData.at(i).iNote);
                const Note& dbNote(iDatabaseData.at(i).iNote);
                if (!note.equals(dbNote)) {
                    QSqlQuery query(iDatabase);
                    if (query.prepare("UPDATE " TABLE_NAME " SET "
                        FIELD_NAME_COLOR " = :" FIELD_NAME_COLOR ", "
                        FIELD_NAME_BODY " = :" FIELD_NAME_BODY " WHERE "
                        FIELD_NAME_PAGENR " = :" FIELD_NAME_PAGENR)) {
                        query.bindValue(placeholderPagenr, note.iPageNr);
                        query.bindValue(placeholderColor, note.iColor);
                        query.bindValue(placeholderBody, note.iBody);
                        if (query.exec()) {
                            HDEBUG("Updated note" << note.iPageNr);
                        } else {
                            HWARN(query.lastError());
                            ok = false;
                        }
                    } else {
                        HWARN(query.lastError());
                        ok = false;
                    }
                }
            }
        }

        if (ok) {
            if (iDatabase.commit()) {
                iDatabaseData = iModelData;
                HDEBUG("commited changes");
            } else {
                HWARN(iDatabase.lastError());
            }
        } else {
            HWARN("Oops, failed to commit changes");
            if (!iDatabase.rollback()) {
                HWARN(iDatabase.lastError());
            }
        }
    }
}

void FoilNotesPlaintextModel::Private::updateText()
{
    QString text;
    const int n = iModelData.count();
    if (iTextIndex >= 0 && iTextIndex < n) {
        text = iModelData.at(iTextIndex).body();
    } else if (n > 0) {
        text = iModelData.at(0).body();
    }
    if (iText != text) {
        iText = text;
        Q_EMIT parentModel()->textChanged();
    }
}

// ==========================================================================
// FoilNotesPlaintextModel
// ==========================================================================

#define SUPER FoilNotesBaseModel

FoilNotesPlaintextModel::FoilNotesPlaintextModel(QObject* aParent) :
    SUPER(aParent),
    iPrivate(new Private(this))
{
}

// Callback for qmlRegisterSingletonType<FoilNotesPlaintextModel>
QObject* FoilNotesPlaintextModel::createSingleton(QQmlEngine*, QJSEngine*)
{
    return new FoilNotesPlaintextModel();
}

QHash<int,QByteArray> FoilNotesPlaintextModel::roleNames() const
{
    QHash<int,QByteArray> roles;
    roles.insert(Private::PageNrRole, FIELD_NAME_PAGENR);
    roles.insert(Private::ColorRole, FIELD_NAME_COLOR);
    roles.insert(Private::BodyRole, FIELD_NAME_BODY);
    roles.insert(Private::SelectedRole, "selected");
    roles.insert(Private::BusyRole, "busy");
    return roles;
}

int FoilNotesPlaintextModel::rowCount(const QModelIndex& aParent) const
{
    return iPrivate->iModelData.count();
}

QVariant FoilNotesPlaintextModel::data(const QModelIndex& aIndex, int aRole) const
{
    const ModelData* data = iPrivate->dataAt(aIndex);
    if (data) {
        switch (aRole) {
        case Private::PageNrRole:   return data->pagenr();
        case Private::ColorRole:    return data->color();
        case Private::BodyRole:     return data->body();
        case Private::SelectedRole: return data->iSelected;
        case Private::BusyRole:     return data->isBusy();
        }
    }
    return QVariant();
}

bool FoilNotesPlaintextModel::setData(const QModelIndex& aIndex, const QVariant& aValue, int aRole)
{
    ModelData* data = iPrivate->dataAt(aIndex);
    if (data) {
        QVector<int> roles;
        roles.append(aRole);
        switch ((Private::Role)aRole) {
        case Private::ColorRole:
            {
                QColor color(aValue.toString());
                if (color.isValid()) {
                    if (data->color() != color) {
                        HDEBUG(data->pagenr() << color);
                        data->iNote.iColor = color;
                        Q_EMIT dataChanged(aIndex, aIndex, roles);
                        iPrivate->scheduleCommit();
                    }
                    return true;
                }
            }
            break;
        case Private::BodyRole:
            {
                QString body(aValue.toString());
                if (data->body() != body) {
                    HDEBUG(data->pagenr() << body);
                    data->iNote.iBody = body;
                    Q_EMIT dataChanged(aIndex, aIndex, roles);
                    iPrivate->updateText();
                    iPrivate->scheduleCommit();
                }
            }
            return true;
        case Private::SelectedRole:
            {
                bool selected(aValue.toBool());
                if (data->iSelected != selected) {
                    HDEBUG(data->pagenr() << selected);
                    data->iSelected = selected;
                    if (selected) {
                        iPrivate->iSelected++;
                    } else {
                        HASSERT(iPrivate->iSelected > 0);
                        iPrivate->iSelected--;
                    }
                    Q_EMIT dataChanged(aIndex, aIndex, roles);
                    Q_EMIT selectedChanged();
                }
            }
            return true;
        case Private::PageNrRole:
        case Private::BusyRole:
            break;
        }
    }
    return false;
}

bool FoilNotesPlaintextModel::moveRows(const QModelIndex &aSrcParent, int aSrcRow,
    int aCount, const QModelIndex &aDestParent, int aDestRow)
{
    const int size = iPrivate->rowCount();
    if (aSrcParent == aDestParent &&
        aSrcRow != aDestRow &&
        aSrcRow >= 0 && aSrcRow < size &&
        aDestRow >= 0 && aDestRow < size) {

        HDEBUG(aSrcRow << "->" << aDestRow);
        beginMoveRows(aSrcParent, aSrcRow, aSrcRow, aDestParent,
            (aDestRow < aSrcRow) ? aDestRow : (aDestRow + 1));
        iPrivate->iModelData.move(aSrcRow, aDestRow);
        endMoveRows();

        iPrivate->updatePageNr(qMin(aSrcRow, aDestRow), qMax(aSrcRow, aDestRow) + 1);
        iPrivate->updateText();
        iPrivate->scheduleCommit();
        return true;
    } else {
        return false;
    }
}

void FoilNotesPlaintextModel::addNote(QColor aColor, QString aBody)
{
    HDEBUG(aColor.name() << aBody);

    // Insert new note
    beginInsertRows(QModelIndex(), 0, 0);
    iPrivate->iModelData.insert(0, ModelData(1, aColor, aBody));
    endInsertRows();

    iPrivate->updatePageNr(1);
    iPrivate->updateText();
    iPrivate->scheduleCommit();
}

void FoilNotesPlaintextModel::deleteNoteAt(int aRow)
{
    HDEBUG(aRow);
    iPrivate->deleteNoteAt(aRow);
    iPrivate->updateText();
}

void FoilNotesPlaintextModel::setBodyAt(int aRow, QString aBody)
{
    ModelData* data = iPrivate->dataAt(aRow);
    if (data && data->body() != aBody) {
        data->iNote.iBody = aBody;
        HDEBUG(data->pagenr() << aBody);
        iPrivate->dataChanged(aRow, Private::BodyRole);
        iPrivate->updateText();
        iPrivate->scheduleCommit();
    }
}

void FoilNotesPlaintextModel::setColorAt(int aRow, QColor aColor)
{
    ModelData* data = iPrivate->dataAt(aRow);
    if (data && data->color() != aColor) {
        data->iNote.iColor = aColor;
        HDEBUG(data->pagenr() << aColor);
        iPrivate->dataChanged(aRow, Private::ColorRole);
        iPrivate->scheduleCommit();
    }
}

QString FoilNotesPlaintextModel::text() const
{
    return iPrivate->iText;
}

int FoilNotesPlaintextModel::textIndex() const
{
    return iPrivate->iTextIndex;
}

void FoilNotesPlaintextModel::setTextIndex(int aIndex)
{
    if (iPrivate->iTextIndex != aIndex) {
        iPrivate->iTextIndex = aIndex;
        iPrivate->updateText();
        Q_EMIT textIndexChanged();
    }
}

void FoilNotesPlaintextModel::deleteNotes(QList<int> aRows)
{
    if (!aRows.isEmpty()) {
        qSort(aRows);
        bool selectionChanged = false;
        int deleted = 0;
        int lastRowToRemove = -1;
        int firstRowToRemove = -1;
        for (int i = aRows.count() - 1; i >= 0; i--) {
            const int pos = aRows.at(i);
            ModelData* data = iPrivate->dataAt(pos);
            if (data) {
                if (data->iSelected) {
                    data->iSelected = false;
                    iPrivate->iSelected--;
                    selectionChanged = true;
                }
                HDEBUG(data->pagenr());
                iPrivate->iModelData.removeAt(i);
                deleted++;
                if (lastRowToRemove < 0) {
                    lastRowToRemove = firstRowToRemove = i;
                } else {
                    HASSERT((firstRowToRemove - 1) == i);
                    firstRowToRemove--;
                }
            } else if (firstRowToRemove >= 0) {
                HDEBUG(firstRowToRemove << lastRowToRemove);
                beginRemoveRows(QModelIndex(), firstRowToRemove, lastRowToRemove);
                firstRowToRemove = lastRowToRemove = -1;
                endRemoveRows();
            }
        }
        if (firstRowToRemove >= 0) {
            HDEBUG(firstRowToRemove << lastRowToRemove);
            beginRemoveRows(QModelIndex(), firstRowToRemove, lastRowToRemove);
            endRemoveRows();
        }
        if (deleted > 0) {
            if (selectionChanged) {
                Q_EMIT selectedChanged();
            }
            iPrivate->scheduleCommit();
        }
    }
}

int FoilNotesPlaintextModel::startEncryptingAt(int aRow)
{
    ModelData* data = iPrivate->dataAt(aRow);
    if (data) {
        const bool wasBusy = data->isBusy();
        data->iEncReqId = iPrivate->nextReqId();
        HDEBUG("encrypting" << data->pagenr() << "id" << data->iEncReqId);
        if (!wasBusy) {
            iPrivate->dataChanged(aRow, Private::BusyRole);
        }
        return data->iEncReqId;
    } else {
        return 0;
    }
}

void FoilNotesPlaintextModel::onEncryptionDone(int aReqId, bool aSuccess)
{
    const int n = iPrivate->rowCount();
    for (int i = 0; i < n; i++) {
        ModelData* data = iPrivate->dataAt(i);
        if (data->iEncReqId == aReqId) {
            HDEBUG("Request" << aReqId << (aSuccess ? "succeeded" : "failed"));
            data->iEncReqId = 0;
            const bool wasSelected = data->iSelected;
            if (wasSelected) {
                HASSERT(iPrivate->iSelected > 0);
                iPrivate->iSelected--;
            }
            if (aSuccess) {
                iPrivate->deleteNoteAt(i);
            }
            if (wasSelected) {
                Q_EMIT selectedChanged();
            }
            return;
        }
    }
    HDEBUG("Encrypt request" << aReqId << "not found");
}

void FoilNotesPlaintextModel::saveNote(int aPageNr, QColor aColor, QString aBody)
{
    HDEBUG(aPageNr << aColor.name() << aBody);

    // Validate the position
    const int n = iPrivate->rowCount();
    if (aPageNr > n + 1) {
        HDEBUG("pagenr" << aPageNr << "->" << (n + 1));
        aPageNr = n + 1;
    } else if (aPageNr < 1) {
        HDEBUG("pagenr" << aPageNr << "-> 1");
        aPageNr = 1;
    }

    // Insert new note
    const int pos = aPageNr - 1;
    beginInsertRows(QModelIndex(), pos, pos);
    iPrivate->iModelData.insert(pos, ModelData(aPageNr, aColor, aBody));
    endInsertRows();

    iPrivate->updatePageNr(aPageNr);
    iPrivate->updateText();
    iPrivate->scheduleCommit();
}

const FoilNotesBaseModel::Note* FoilNotesPlaintextModel::noteAt(int aRow) const
{
    const ModelData* data = iPrivate->dataAt(aRow);
    return data ? &data->iNote : NULL;
}

bool FoilNotesPlaintextModel::selectedAt(int aRow) const
{
    const ModelData* data = iPrivate->dataAt(aRow);
    return data && data->iSelected;
}

void FoilNotesPlaintextModel::setSelectedAt(int aRow, bool aSelected)
{
    ModelData* data = iPrivate->dataAt(aRow);
    HDEBUG(data->pagenr() << aSelected);
    if (data->iSelected && !aSelected) {
        HASSERT(iPrivate->iSelected > 0);
        iPrivate->iSelected--;
    } else if (!data->iSelected && aSelected) {
        HASSERT(iPrivate->iSelected < iPrivate->rowCount());
        iPrivate->iSelected++;
    }
    data->iSelected = aSelected;
    iPrivate->dataChanged(aRow, Private::SelectedRole);
}

int FoilNotesPlaintextModel::selectedCount() const
{
    return iPrivate->iSelected;
}

#include "FoilNotesPlaintextModel.moc"
