/*
 * Copyright (C) 2018-2026 Slava Monich <slava@monich.com>
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

#include "FoilNotesModel.h"

#include "HarbourDebug.h"
#include "HarbourTask.h"
#include "HarbourParentSignalQueueObject.h"
#include "HarbourProcessState.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSharedPointer>
#include <QtCore/QThreadPool>

#include "foil_output.h"
#include "foil_private_key.h"
#include "foil_random.h"
#include "foil_util.h"

#include "foilmsg.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#define ENCRYPT_KEY_TYPE        FOILMSG_KEY_AES_256
#define SIGNATURE_TYPE          FOILMSG_SIGNATURE_SHA256_RSA

#define HEADER_COLOR            "color"

#define ENCRYPT_FILE_MODE       0600

// Directories relative to home
#define FOIL_NOTES_DIR          "Documents/FoilNotes"
#define FOIL_KEY_DIR            ".local/share/foil"
#define FOIL_KEY_FILE           "foil.key"

#define INFO_FILE               ".info"
#define INFO_CONTENTS           "FoilNotes"
#define INFO_ORDER_HEADER       "Order"
#define INFO_ORDER_DELIMITER    ','
#define INFO_ORDER_DELIMITER_S  ","

// Role names
#define MODEL_ROLES(role) \
    role(Body, body) \
    role(Color, color) \
    role(PageNr, pagenr) \
    role(Selected, selected)

// s(SignalName,signalName,Suffix)
#define MODEL_SIGNALS(s) \
    s(Selected,selected,Changed) \
    s(Count,count,Changed) \
    s(Busy,busy,Changed) \
    s(KeyAvailable,keyAvailable,Changed) \
    s(FoilState,foilState,Changed) \
    s(Key,key,Generated) \
    s(TextIndex,textIndex,Changed) \
    s(Text,text,Changed)

// ==========================================================================
// FoilNotesModel::ModelData
// ==========================================================================

class FoilNotesModel::ModelData
{
public:
    enum Role {
        FirstRole = Qt::UserRole,
        #define ROLE(X,x) X##Role,
        MODEL_ROLES(ROLE)
        #undef ROLE
        LastRole
    };

    #define ROLE(X,x) static const QString RoleName##X;
    MODEL_ROLES(ROLE)
    #undef ROLE

    typedef QList<ModelData*> List;
    typedef List::ConstIterator ConstIterator;

    ModelData(const QString&, const QColor&, uint, const QString&, uint);

    QVariant get(Role) const;
    QString body() const { return iNote.iBody; }
    QColor color() const { return iNote.iColor; }
    QString colorName() const { return iNote.iColor.name(); }
    uint pagenr() const { return iNote.iPageNr; }

    static QString headerString(const FoilMsg*, const char*);
    static QColor headerColor(const FoilMsg*, const char*);
    static uint nextId(QAtomicInt*);

public:
    Note iNote;
    uint iNoteId;
    QString iPath;
    QString iFileName;
    bool iSelected;
};

#define ROLE(X,x) const QString FoilNotesModel::ModelData::RoleName##X(#x);
MODEL_ROLES(ROLE)
#undef ROLE

FoilNotesModel::ModelData::ModelData(
    const QString& aBody,
    const QColor& aColor,
    uint aPageNr,
    const QString& aPath,
    uint aNoteId) :
    iNote(aPageNr, aColor, aBody),
    iNoteId(aNoteId),
    iPath(aPath),
    iSelected(false)
{}

QVariant
FoilNotesModel::ModelData::get(
    Role aRole) const
{
    switch (aRole) {
    case BodyRole: return body();
    case ColorRole: return color();
    case PageNrRole: return pagenr();
    case SelectedRole: return iSelected;
    // No default to make sure that we get "warning: enumeration value
    // not handled in switch" if we forget to handle a real role.
    case FirstRole:
    case LastRole:
        break;
    }
    return QVariant();
}

QString
FoilNotesModel::ModelData::headerString(
    const FoilMsg* aMsg,
    const char* aKey)
{
    const char* value = foilmsg_get_value(aMsg, aKey);

    return (value && value[0]) ? QString::fromLatin1(value) : QString();
}

QColor
FoilNotesModel::ModelData::headerColor(
    const FoilMsg* aMsg,
    const char* aKey)
{
    return QColor(headerString(aMsg, aKey));
}

uint
FoilNotesModel::ModelData::nextId(
    QAtomicInt* aNextId)
{
    uint id;

    while (!(id = aNextId->fetchAndAddRelaxed(1)));
    return id;
}

// ==========================================================================
// FoilNotesModel::ModelInfo
// ==========================================================================

class FoilNotesModel::ModelInfo
{
public:
    ModelInfo() {}
    ModelInfo(const FoilMsg*);
    ModelInfo(const ModelInfo&);
    ModelInfo(const ModelData::List&);

    static ModelInfo load(const QString&, FoilPrivateKey*, FoilKey*);

    void save(const QString&, FoilPrivateKey*, FoilKey*) const;
    ModelInfo& operator=(const ModelInfo&);

public:
    QStringList iOrder;
};

FoilNotesModel::ModelInfo::ModelInfo(
    const ModelInfo& aInfo) :
    iOrder(aInfo.iOrder)
{}

FoilNotesModel::ModelInfo::ModelInfo(
    const ModelData::List& aData)
{
    const int n = aData.count();

    iOrder.reserve(n);
    for (int i = 0; i < n; i++) {
        const ModelData* data = aData.at(i);
        iOrder.append(QFileInfo(data->iPath).fileName());
    }
}

FoilNotesModel::ModelInfo::ModelInfo(
    const FoilMsg* msg)
{
    const char* order = foilmsg_get_value(msg, INFO_ORDER_HEADER);

    if (order) {
        char** strv = g_strsplit(order, INFO_ORDER_DELIMITER_S, -1);

        for (char** ptr = strv; *ptr; ptr++) {
            iOrder.append(g_strstrip(*ptr));
        }
        g_strfreev(strv);
        HDEBUG(order);
    }
}

FoilNotesModel::ModelInfo&
FoilNotesModel::ModelInfo::operator=(
    const ModelInfo& aInfo)
{
    iOrder = aInfo.iOrder;
    return *this;
}

FoilNotesModel::ModelInfo
FoilNotesModel::ModelInfo::load(
    const QString& aDir,
    FoilPrivateKey* aPrivate,
    FoilKey* aPublic)
{
    QString fullPath(aDir + "/" INFO_FILE);
    const QByteArray path(fullPath.toUtf8());
    const char* fname = path.constData();

    HDEBUG("Loading" << fname);

    ModelInfo info;
    FoilMsg* msg = foilmsg_decrypt_file(aPrivate, fname, Q_NULLPTR);

    if (msg) {
        if (foilmsg_verify(msg, aPublic)) {
            info = ModelInfo(msg);
        } else {
            HWARN("Could not verify" << fname);
        }
        foilmsg_free(msg);
    }
    return info;
}

void
FoilNotesModel::ModelInfo::save(
    const QString& aDir,
    FoilPrivateKey* aPrivate,
    FoilKey* aPublic) const
{
    QString fullPath(aDir + "/" INFO_FILE);
    const QByteArray path(fullPath.toUtf8());
    const char* fname = path.constData();
    FoilOutput* out = foil_output_file_new_open(fname);

    if (out) {
        QString buf;
        const int n = iOrder.count();

        for (int i = 0; i < n; i++) {
            if (!buf.isEmpty()) buf += QChar(INFO_ORDER_DELIMITER);
            buf += iOrder.at(i);
        }

        const QByteArray order(buf.toUtf8());

        HDEBUG("Saving" << fname);
        HDEBUG(INFO_ORDER_HEADER ":" << order.constData());

        FoilMsgHeaders headers;
        FoilMsgHeader header[1];

        headers.header = header;
        headers.count = 0;
        header[headers.count].name = INFO_ORDER_HEADER;
        header[headers.count].value = order.constData();
        headers.count++;

        FoilMsgEncryptOptions opt;

        foilmsg_encrypt_defaults(&opt);
        opt.key_type = ENCRYPT_KEY_TYPE;
        opt.signature = SIGNATURE_TYPE;

        FoilBytes data;

        foil_bytes_from_string(&data, INFO_CONTENTS);
        foilmsg_encrypt(out, &data, Q_NULLPTR, &headers, aPrivate, aPublic,
            &opt, Q_NULLPTR);
        foil_output_unref(out);
        if (chmod(fname, ENCRYPT_FILE_MODE) < 0) {
            HWARN("Failed to chmod" << fname << strerror(errno));
        }
    } else {
        HWARN("Failed to open" << fname);
    }
}

// ==========================================================================
// FoilNotesModel::BaseTask
// ==========================================================================

class FoilNotesModel::BaseTask :
    public HarbourTask
{
public:
    BaseTask(QThreadPool*, FoilPrivateKey*, FoilKey*);
    virtual ~BaseTask();

    FoilMsg* decryptAndVerify(const QString&) const;
    FoilMsg* decryptAndVerify(const char*) const;

    static bool removeFile(const QString&);
    static QString toString(const FoilMsg*);
    static FoilOutput* createFoilFile(const QString&, GString*);

public:
    FoilPrivateKey* iPrivateKey;
    FoilKey* iPublicKey;
};

FoilNotesModel::BaseTask::BaseTask(
    QThreadPool* aPool,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey) :
    HarbourTask(aPool),
    iPrivateKey(foil_private_key_ref(aPrivateKey)),
    iPublicKey(foil_key_ref(aPublicKey))
{}

FoilNotesModel::BaseTask::~BaseTask()
{
    foil_private_key_unref(iPrivateKey);
    foil_key_unref(iPublicKey);
}

bool
FoilNotesModel::BaseTask::removeFile(
    const QString& aPath)
{
    if (!aPath.isEmpty()) {
        if (QFile::remove(aPath)) {
            HDEBUG("Removed" << qPrintable(aPath));
            return true;
        }
        HWARN("Failed to delete" << qPrintable(aPath));
    }
    return false;
}

FoilMsg*
FoilNotesModel::BaseTask::decryptAndVerify(
    const QString& aFileName) const
{
    if (!aFileName.isEmpty()) {
        const QByteArray fileNameBytes(aFileName.toUtf8());

        return decryptAndVerify(fileNameBytes.constData());
    } else{
        return Q_NULLPTR;
    }
}

FoilMsg*
FoilNotesModel::BaseTask::decryptAndVerify(
    const char* aFileName) const
{
    if (aFileName) {
        HDEBUG("Decrypting" << aFileName);

        FoilMsg* msg = foilmsg_decrypt_file(iPrivateKey, aFileName, Q_NULLPTR);

        if (msg) {
#if HARBOUR_DEBUG
            for (uint i = 0; i < msg->headers.count; i++) {
                const FoilMsgHeader* header = msg->headers.header + i;
                HDEBUG(" " << header->name << ":" << header->value);
            }
#endif // HARBOUR_DEBUG
            if (foilmsg_verify(msg, iPublicKey)) {
                return msg;
            } else {
                HWARN("Could not verify" << aFileName);
            }
            foilmsg_free(msg);
        }
    }
    return Q_NULLPTR;
}

QString
FoilNotesModel::BaseTask::toString(
    const FoilMsg* aMsg)
{
    if (aMsg) {
        const char* type = aMsg->content_type;

        if (!type || g_str_has_prefix(type, "text/")) {
            gsize size;
            const char* data = (char*)g_bytes_get_data(aMsg->data, &size);

            if (data && size) {
                return QString::fromUtf8(data, size);
            }
        } else {
            HWARN("Unexpected content type" << type);
        }
    }
    return QString();
}

FoilOutput*
FoilNotesModel::BaseTask::createFoilFile(
    const QString& aDestDir,
    GString* aOutPath)
{
    // Generate random name for the encrypted file
    FoilOutput* out = Q_NULLPTR;
    const QByteArray dir(aDestDir.toUtf8());

    g_string_truncate(aOutPath, 0);
    g_string_append_len(aOutPath, dir.constData(), dir.size());
    g_string_append_c(aOutPath, '/');

    const gsize prefix_len = aOutPath->len;

    for (int i = 0; i < 100 && !out; i++) {
        guint8 data[8];

        foil_random_generate(FOIL_RANDOM_DEFAULT, data, sizeof(data));
        g_string_truncate(aOutPath, prefix_len);
        g_string_append_printf(aOutPath, "%02X%02X%02X%02X%02X%02X%02X%02X",
            data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
        out = foil_output_file_new_open(aOutPath->str);
    }
    HASSERT(out);
    return out;
}

// ==========================================================================
// FoilNotesModel::GenerateKeyTask
// ==========================================================================

class FoilNotesModel::GenerateKeyTask :
    public BaseTask
{
    Q_OBJECT

public:
    GenerateKeyTask(QThreadPool*, const QString&, int, const QString&);

    virtual void performTask() Q_DECL_OVERRIDE;

public:
    const QString iKeyFile;
    const int iBits;
    const QString iPassword;
};

FoilNotesModel::GenerateKeyTask::GenerateKeyTask(
    QThreadPool* aPool,
    const QString& aKeyFile,
    int aBits,
    const QString& aPassword) :
    BaseTask(aPool, Q_NULLPTR, Q_NULLPTR),
    iKeyFile(aKeyFile),
    iBits(aBits),
    iPassword(aPassword)
{}

void
FoilNotesModel::GenerateKeyTask::performTask()
{
    HDEBUG("Generating key..." << iBits << "bits");

    FoilKey* key = foil_key_generate_new(FOIL_KEY_RSA_PRIVATE, iBits);

    if (key) {
        GError* error = Q_NULLPTR;
        const QByteArray path(iKeyFile.toUtf8());
        const QByteArray passphrase(iPassword.toUtf8());
        FoilOutput* out = foil_output_file_new_open(path.constData());
        FoilPrivateKey* pk = FOIL_PRIVATE_KEY(key);

        if (foil_private_key_encrypt(pk, out, FOIL_KEY_EXPORT_FORMAT_DEFAULT,
            passphrase.constData(),
            Q_NULLPTR, &error)) {
            iPrivateKey = pk;
            iPublicKey = foil_public_key_new_from_private(pk);
        } else {
            HWARN(error->message);
            g_error_free(error);
            foil_key_unref(key);
        }
        foil_output_unref(out);
    }
    HDEBUG("Done!");
}

// ==========================================================================
// FoilNotesModel::EncryptTask
// ==========================================================================

class FoilNotesModel::EncryptTask :
    public BaseTask
{
    Q_OBJECT

public:
    EncryptTask(QThreadPool*, const QString&, const QColor&, uint, uint,
        FoilPrivateKey*, FoilKey*, const QString&, uint);
    EncryptTask(QThreadPool*, const ModelData*, FoilPrivateKey*, FoilKey*,
        const QString&);

    ~EncryptTask();

    virtual void performTask() Q_DECL_OVERRIDE;

public:
    const QString iBody;
    const QColor iColor;
    const uint iPageNr;
    const uint iNoteId;
    const uint iReqId;
    const QString iDestDir;
    const QString iRemoveFile;
    ModelData* iData;
};

FoilNotesModel::EncryptTask::EncryptTask(
    QThreadPool* aPool,
    const QString& aBody,
    const QColor& aColor,
    uint aPageNr,
    uint aNoteId,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey,
    const QString& aDestDir,
    uint aReqId) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iBody(aBody),
    iColor(aColor),
    iPageNr(aPageNr),
    iNoteId(aNoteId),
    iReqId(aReqId),
    iDestDir(aDestDir),
    iData(Q_NULLPTR)
{
    HDEBUG("Encrypting" << iPageNr << iColor.name() << iBody << iReqId);
}

FoilNotesModel::EncryptTask::EncryptTask(
    QThreadPool* aPool,
    const ModelData* aData,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey,
    const QString& aDestDir) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iBody(aData->body()),
    iColor(aData->color()),
    iPageNr(aData->pagenr()),
    iNoteId(aData->iNoteId),
    iReqId(0),
    iDestDir(aDestDir),
    iRemoveFile(aData->iPath),
    iData(Q_NULLPTR)
{
    HDEBUG("Encrypting" << iPageNr << iColor.name() << iBody);
}

FoilNotesModel::EncryptTask::~EncryptTask()
{
    if (iData) {
        removeFile(iData->iPath);
        delete iData;
    }
}

void
FoilNotesModel::EncryptTask::performTask()
{
    GString* dest = g_string_sized_new(iDestDir.size() + 9);
    FoilOutput* out = createFoilFile(iDestDir, dest);

    if (out) {
        FoilMsgEncryptOptions opt;

        foilmsg_encrypt_defaults(&opt);
        opt.key_type = ENCRYPT_KEY_TYPE;
        opt.signature = SIGNATURE_TYPE;

        FoilMsgHeaders headers;
        FoilMsgHeader header[1];

        headers.header = header;
        headers.count = 0;

        QByteArray color(iColor.name().toLatin1());

        header[headers.count].name = HEADER_COLOR;
        header[headers.count].value = color.constData();
        headers.count++;

        HASSERT(headers.count <= G_N_ELEMENTS(header));
        HDEBUG("Writing" << iBody);

        QByteArray bodyBytes(iBody.toUtf8());
        FoilBytes body;

        body.val = (guint8*)bodyBytes.constData();
        body.len = bodyBytes.length();

        if (foilmsg_encrypt(out, &body, Q_NULLPTR, &headers,
            iPrivateKey, iPublicKey, &opt, Q_NULLPTR)) {
            QString path(QString::fromLocal8Bit(dest->str, dest->len));
            iData = new ModelData(iBody, iColor, 0, path, iNoteId);
            removeFile(iRemoveFile);
            if (chmod(dest->str, ENCRYPT_FILE_MODE) < 0) {
                HWARN("Failed to chmod" << dest->str << strerror(errno));
            }
        }

        foil_output_unref(out);
        if (!iData) {
            unlink(dest->str);
        }
        g_string_free(dest, TRUE);
    }
}

// ==========================================================================
// FoilNotesModel::SaveInfoTask
// ==========================================================================

class FoilNotesModel::SaveInfoTask :
    public BaseTask
{
    Q_OBJECT

public:
    SaveInfoTask(QThreadPool*, ModelInfo, const QString&,
        FoilPrivateKey*,FoilKey*);

    virtual void performTask() Q_DECL_OVERRIDE;

public:
    const ModelInfo iInfo;
    const QString iFoilDir;
};

FoilNotesModel::SaveInfoTask::SaveInfoTask(
    QThreadPool* aPool,
    ModelInfo aInfo,
    const QString& aFoilDir,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iInfo(aInfo),
    iFoilDir(aFoilDir)
{}

void
FoilNotesModel::SaveInfoTask::performTask()
{
    if (!isCanceled()) {
        iInfo.save(iFoilDir, iPrivateKey, iPublicKey);
    }
}

// ==========================================================================
// FoilNotesModel::DecryptNotesTask
// ==========================================================================

class FoilNotesModel::DecryptNotesTask :
    public BaseTask
{
    Q_OBJECT

public:
    // The purpose of this class is to make sure that ModelData doesn't
    // get lost in transit when we asynchronously post the results from
    // DecryptNotesTask to FoilNotesModel.
    //
    // If the signal successfully reaches the slot, the receiver zeros
    // iModelData which stops ModelData from being deallocated by the
    // Progress destructor. If the signal never reaches the slot, then
    // ModelData is deallocated together with when the last reference
    // to Progress
    class Progress {
    public:
        typedef QSharedPointer<Progress> Ptr;

        Progress(ModelData* aModelData, DecryptNotesTask* aTask) :
            iModelData(aModelData), iTask(aTask) {}
        ~Progress() { delete iModelData; }

    public:
        ModelData* iModelData;
        DecryptNotesTask* iTask;
    };

    DecryptNotesTask(QThreadPool* aPool, const QString& aDir,
        FoilPrivateKey* aPrivateKey, FoilKey* aPublicKey,
        QAtomicInt* aNextNoteId);

    virtual void performTask() Q_DECL_OVERRIDE;

    bool decryptNote(const QString&, uint);

Q_SIGNALS:
    void progress(DecryptNotesTask::Progress::Ptr aProgress);

public:
    const QString iDir;
    QAtomicInt* iNextNoteId;
    bool iSaveInfo;
};

Q_DECLARE_METATYPE(FoilNotesModel::DecryptNotesTask::Progress::Ptr)

FoilNotesModel::DecryptNotesTask::DecryptNotesTask(
    QThreadPool* aPool,
    const QString& aDir,
    FoilPrivateKey* aPrivateKey,
    FoilKey* aPublicKey,
    QAtomicInt* aNextNoteId) :
    BaseTask(aPool, aPrivateKey, aPublicKey),
    iDir(aDir),
    iNextNoteId(aNextNoteId),
    iSaveInfo(false)
{}

bool
FoilNotesModel::DecryptNotesTask::decryptNote(
    const QString& aPath,
    uint aPageNr)
{
    bool ok = false;
    FoilMsg* msg = decryptAndVerify(aPath);

    if (msg) {
        QColor color = ModelData::headerColor(msg, HEADER_COLOR);
        if (color.isValid()) {
            ModelData* data = new ModelData(toString(msg), color, aPageNr,
                aPath, ModelData::nextId(iNextNoteId));
            HDEBUG("Loaded note from" << qPrintable(aPath));
            // The Progress takes ownership of ModelData
            Q_EMIT progress(Progress::Ptr(new Progress(data, this)));
            ok = true;
        }
        foilmsg_free(msg);
    }
    return ok;
}

void
FoilNotesModel::DecryptNotesTask::performTask()
{
    if (!isCanceled()) {
        const QString path(iDir);

        HDEBUG("Checking" << iDir);

        QDir dir(path);
        QFileInfoList list = dir.entryInfoList(QDir::Files |
            QDir::Dirs | QDir::NoDotAndDotDot, QDir::NoSort);

        // Restore the order
        ModelInfo info(ModelInfo::load(iDir, iPrivateKey, iPublicKey));

        const QString infoFile(INFO_FILE);
        QHash<QString,QString> fileMap;
        int i;

        for (i = 0; i < list.count(); i++) {
            const QFileInfo& info = list.at(i);
            if (info.isFile()) {
                const QString name(info.fileName());
                if (name != infoFile) {
                    fileMap.insert(name, info.filePath());
                }
            }
        }

        // First decrypt files in known order
        uint pagenr = 1;

        for (i = 0; i < info.iOrder.count() && !isCanceled(); i++) {
            const QString name(info.iOrder.at(i));
            if (fileMap.contains(name) &&
                decryptNote(fileMap.take(name), pagenr)) {
                pagenr++;
            } else {
                // Broken order or something
                HDEBUG(qPrintable(name) << "oops!");
                iSaveInfo = true;
            }
        }

        // Followed by the remaining files in no particular order
        if (!fileMap.isEmpty()) {
            const QStringList remainingFiles = fileMap.values();

            HDEBUG("Remaining file(s)" << remainingFiles);
            for (i = 0; i < remainingFiles.count() && !isCanceled(); i++) {
                if (decryptNote(remainingFiles.at(i), pagenr)) {
                    pagenr++;
                } else {
                    HDEBUG(remainingFiles.at(i) << "was not expected");
                    iSaveInfo = true;
                }
            }
        }
    }
}

// ==========================================================================
// FoilNotesModel::Private
// ==========================================================================

enum FoilNotesModelSignal {
    #define SIGNAL_ENUM_(Name,name,Suffix) Signal##Name##Suffix,
    MODEL_SIGNALS(SIGNAL_ENUM_)
    #undef SIGNAL_ENUM_
    FoilNotesModelSignalCount
};

typedef HarbourParentSignalQueueObject<FoilNotesModel,
    FoilNotesModelSignal, FoilNotesModelSignalCount>
    FoilNotesModelPrivateBase;

class FoilNotesModel::Private :
    public FoilNotesModelPrivateBase
{
    Q_OBJECT

    static const SignalEmitter gSignalEmitters[];

public:
    Private(FoilNotesModel*);
    ~Private();

public Q_SLOTS:
    void onDecryptNotesProgress(DecryptNotesTask::Progress::Ptr);
    void onDecryptNotesTaskDone();
    void onEncryptNoteDone();
    void onSaveInfoDone();
    void onSaveNoteDone();
    void onGenerateKeyTaskDone();

public:
    int rowCount() const;
    ModelData* dataAt(int aIndex);
    uint nextId();
    int findNote(uint aNoteId);
    void updatePageNr(uint);
    void updatePageNr(uint, uint);
    void updateText();
    bool checkPassword(const QString&);
    bool changePassword(const QString&, const QString&);
    void setKeys(FoilPrivateKey*, FoilKey* aPublic = Q_NULLPTR);
    void setFoilState(FoilState);
    void addNote(const QColor&, const QString&);
    void insertModelData(ModelData*);
    void dataChanged(int, ModelData::Role);
    void dataChanged(const QList<int>&, ModelData::Role);
    void destroyItemAt(int aIndex);
    bool destroyItemAndRemoveFilesAt(int);
    void deleteNoteAt(int);
    void deleteNotes(QList<int>);
    void saveNoteAt(int);
    void setBodyAt(int, const QString&);
    void setColorAt(int, const QColor&);
    void clearModel();
    bool busy() const;
    void saveInfo();
    void generate(int, const QString&);
    void lock(bool aTimeout);
    bool unlock(const QString&);
    bool encrypt(const QString&, const QColor&, uint, uint);

public:
    ModelData::List iData;
    FoilState iFoilState;
    QString iFoilNotesDir;
    QString iFoilKeyDir;
    QString iFoilKeyFile;
    FoilPrivateKey* iPrivateKey;
    FoilKey* iPublicKey;
    QThreadPool* iThreadPool;
    HarbourTask::AutoReleasePointer<SaveInfoTask> iSaveInfoTask;
    HarbourTask::AutoReleasePointer<GenerateKeyTask> iGenerateKeyTask;
    HarbourTask::AutoReleasePointer<DecryptNotesTask> iDecryptNotesTask;
    QList<EncryptTask*> iEncryptTasks;
    QAtomicInt iNextId;
    int iSelected;
    int iTextIndex;
    QString iText;
};

const FoilNotesModelPrivateBase::SignalEmitter
FoilNotesModel::Private::gSignalEmitters [] = {
    #define SIGNAL_EMITTER_(Name,name,Suffix) &FoilNotesModel::name##Suffix,
    MODEL_SIGNALS(SIGNAL_EMITTER_)
    #undef  SIGNAL_EMITTER_
};

FoilNotesModel::Private::Private(
    FoilNotesModel* aParent) :
    FoilNotesModelPrivateBase(aParent, gSignalEmitters),
    iFoilState(FoilKeyMissing),
    iFoilNotesDir(QDir::homePath() + "/" FOIL_NOTES_DIR),
    iFoilKeyDir(QDir::homePath() + "/" FOIL_KEY_DIR),
    iFoilKeyFile(iFoilKeyDir + "/" + FOIL_KEY_FILE),
    iPrivateKey(Q_NULLPTR),
    iPublicKey(Q_NULLPTR),
    iThreadPool(new QThreadPool(this)),
    iNextId(1),
    iSelected(0),
    iTextIndex(-1)
{
    // Serialize the tasks:
    iThreadPool->setMaxThreadCount(1);
    qRegisterMetaType<DecryptNotesTask::Progress::Ptr>("DecryptNotesTask::Progress::Ptr");

    HDEBUG("Key file" << qPrintable(iFoilKeyFile));
    HDEBUG("Notes dir" << qPrintable(iFoilNotesDir));

    // Create the directories if necessary
    if (QDir().mkpath(iFoilKeyDir)) {
        const QByteArray dir(iFoilKeyDir.toUtf8());
        chmod(dir.constData(), 0700);
    }

    if (QDir().mkpath(iFoilNotesDir)) {
        const QByteArray dir(iFoilNotesDir.toUtf8());
        chmod(dir.constData(), 0700);
    }

    // Initialize the key state
    GError* error = Q_NULLPTR;
    const QByteArray path(iFoilKeyFile.toUtf8());
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), Q_NULLPTR, &error);
    if (key) {
        HDEBUG("Key not encrypted");
        iFoilState = FoilKeyNotEncrypted;
        foil_private_key_unref(key);
    } else {
        if (error->domain == FOIL_ERROR) {
            if (error->code == FOIL_ERROR_KEY_ENCRYPTED) {
                HDEBUG("Key encrypted");
                iFoilState = FoilLocked;
            } else {
                HDEBUG("Key invalid" << error->code);
                iFoilState = FoilKeyInvalid;
            }
        } else {
            HDEBUG(error->message);
            iFoilState = HarbourProcessState::isJailedApp() ?
                FoilJailed : FoilKeyMissing;
        }
        g_error_free(error);
    }
}

FoilNotesModel::Private::~Private()
{
    foil_private_key_unref(iPrivateKey);
    foil_key_unref(iPublicKey);
    iSaveInfoTask.reset();
    iGenerateKeyTask.reset();
    iDecryptNotesTask.reset();
    for (int i = 0; i < iEncryptTasks.count(); i++) {
        iEncryptTasks.at(i)->release();
    }
    iEncryptTasks.clear();
    iThreadPool->waitForDone();
    qDeleteAll(iData);
}

inline
int FoilNotesModel::Private::rowCount() const
{
    return iData.count();
}

inline
FoilNotesModel::ModelData*
FoilNotesModel::Private::dataAt(
    int aIndex)
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        return iData.at(aIndex);
    } else {
        return Q_NULLPTR;
    }
}

inline
uint
FoilNotesModel::Private::nextId()
{
    return ModelData::nextId(&iNextId);
}

int
FoilNotesModel::Private::findNote(
    uint aNoteId)
{
    const int n = iData.count();
    for (int i = 0; i < n; i++) {
        const ModelData* data = iData.at(i);
        if (data->iNoteId == aNoteId) {
            return i;
        }
    }
    return -1;
}

void
FoilNotesModel::Private::setKeys(
    FoilPrivateKey* aPrivate,
    FoilKey* aPublic)
{
    if (aPrivate) {
        if (iPrivateKey) {
            foil_private_key_unref(iPrivateKey);
        } else {
            queueSignal(SignalKeyAvailableChanged);
        }
        foil_key_unref(iPublicKey);
        iPrivateKey = foil_private_key_ref(aPrivate);
        iPublicKey = aPublic ? foil_key_ref(aPublic) :
            foil_public_key_new_from_private(aPrivate);
    } else if (iPrivateKey) {
        queueSignal(SignalKeyAvailableChanged);
        foil_private_key_unref(iPrivateKey);
        foil_key_unref(iPublicKey);
        iPrivateKey = Q_NULLPTR;
        iPublicKey = Q_NULLPTR;
    }
}

bool
FoilNotesModel::Private::checkPassword(
    const QString& aPassword)
{
    GError* error = Q_NULLPTR;
    HDEBUG(iFoilKeyFile);
    const QByteArray path(iFoilKeyFile.toUtf8());

    // First make sure that it's encrypted
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), Q_NULLPTR, &error);
    if (key) {
        HWARN("Key not encrypted");
        foil_private_key_unref(key);
    } else if (error->domain == FOIL_ERROR) {
        if (error->code == FOIL_ERROR_KEY_ENCRYPTED) {
            // Validate the old password
            QByteArray password(aPassword.toUtf8());
            g_clear_error(&error);
            key = foil_private_key_decrypt_from_file
                (FOIL_KEY_RSA_PRIVATE, path.constData(),
                    password.constData(), &error);
            if (key) {
                HDEBUG("Password OK");
                foil_private_key_unref(key);
                return true;
            } else {
                HDEBUG("Wrong password");
                g_error_free(error);
            }
        } else {
            HWARN("Key invalid:" << error->message);
            g_error_free(error);
        }
    } else {
        HWARN(error->message);
        g_error_free(error);
    }
    return false;
}

bool
FoilNotesModel::Private::changePassword(
    const QString& aOldPassword,
    const QString& aNewPassword)
{
    HDEBUG(iFoilKeyFile);
    if (checkPassword(aOldPassword)) {
        GError* error = Q_NULLPTR;
        QByteArray password(aNewPassword.toUtf8());

        // First write the temporary file
        QString tmpKeyFile = iFoilKeyFile + ".new";
        const QByteArray tmp(tmpKeyFile.toUtf8());
        FoilOutput* out = foil_output_file_new_open(tmp.constData());

        if (foil_private_key_encrypt(iPrivateKey, out,
            FOIL_KEY_EXPORT_FORMAT_DEFAULT, password.constData(),
            Q_NULLPTR, &error) && foil_output_flush(out)) {
            foil_output_unref(out);

            // Then rename it
            QString saveKeyFile = iFoilKeyFile + ".save";
            QFile::remove(saveKeyFile);

            if (QFile::rename(iFoilKeyFile, saveKeyFile) &&
                QFile::rename(tmpKeyFile, iFoilKeyFile)) {
                BaseTask::removeFile(saveKeyFile);
                HDEBUG("Password changed");
                Q_EMIT parentObject()->passwordChanged();
                return true;
            }
        } else {
            if (error) {
                HWARN(error->message);
                g_error_free(error);
            }
            foil_output_unref(out);
        }
    }
    return false;
}

void
FoilNotesModel::Private::setFoilState(
    FoilState aState)
{
    if (iFoilState != aState) {
        iFoilState = aState;
        HDEBUG(aState);
        queueSignal(SignalFoilStateChanged);
    }
}

void
FoilNotesModel::Private::dataChanged(
    int aIndex,
    ModelData::Role aRole)
{
    if (aIndex >= 0 && aIndex < iData.count()) {
        FoilNotesModel* model = parentObject();
        QModelIndex modelIndex(model->index(aIndex));
        QVector<int> roles;
        roles.append(aRole);

        Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
    }
}

void
FoilNotesModel::Private::dataChanged(
    const QList<int>& aRows,
    ModelData::Role aRole)
{
    const int n = aRows.count();

    if (n > 0) {
        FoilNotesModel* model = parentObject();
        QVector<int> roles;
        roles.append(aRole);

        for (int i = 0; i < n; i++) {
            const int row = aRows.at(i);

            if (row>= 0 && row < iData.count()) {
                QModelIndex modelIndex(model->index(row));
                Q_EMIT model->dataChanged(modelIndex, modelIndex, roles);
            }
        }
    }
}

void
FoilNotesModel::Private::insertModelData(
    ModelData* aData)
{
    const uint n = iData.count();

    // Validate the position
    if (aData->pagenr() > n) {
        aData->iNote.iPageNr = n + 1;
    } else if (!aData->pagenr()) {
        aData->iNote.iPageNr = 1;
    }

    // Insert it into the model
    const int pos = aData->pagenr() - 1;
    FoilNotesModel* model = parentObject();

    model->beginInsertRows(QModelIndex(), pos, pos);
    iData.insert(pos, aData);
    HDEBUG(aData->pagenr() << aData->colorName() << aData->body());
    queueSignal(SignalCountChanged);
    model->endInsertRows();
    updateText();
    updatePageNr(pos + 1);
}

void
FoilNotesModel::Private::onDecryptNotesProgress(
    DecryptNotesTask::Progress::Ptr aProgress)
{
    if (aProgress && aProgress->iTask == iDecryptNotesTask.data()) {
        // Transfer ownership of this ModelData to the model
        insertModelData(aProgress->iModelData);
        aProgress->iModelData = Q_NULLPTR;
    }
    emitQueuedSignals();
}

void
FoilNotesModel::Private::onDecryptNotesTaskDone()
{
    HDEBUG(iData.count() << "note(s) decrypted");
    const bool infoUpdated = iDecryptNotesTask->iSaveInfo;

    iDecryptNotesTask.reset();
    if (iFoilState == FoilDecrypting) {
        setFoilState(FoilNotesReady);
    }
    if (infoUpdated) saveInfo();
    if (!busy()) {
        // We know we were busy when we received this signal
        queueSignal(SignalBusyChanged);
    }
    emitQueuedSignals();
}

void
FoilNotesModel::Private::addNote(
    const QColor& aColor,
    const QString& aBody)
{
    const bool wasBusy = busy();
    const uint id = nextId();
    ModelData* data = new ModelData(aBody, aColor, 1, QString(), id);

    // Update the model right away
    insertModelData(data);

    // Then submit the encryption task
    EncryptTask* task = new EncryptTask(iThreadPool, data,
        iPrivateKey, iPublicKey, iFoilNotesDir);

    iEncryptTasks.append(task);
    task->submit(this, SLOT(onSaveNoteDone()));
    if (!wasBusy) {
        // We must be busy now
        queueSignal(SignalBusyChanged);
    }
}

void
FoilNotesModel::Private::onSaveNoteDone()
{
    EncryptTask* task = qobject_cast<EncryptTask*>(sender());

    if (task) {
        HVERIFY(iEncryptTasks.removeAll(task));
        ModelData* taskData = task->iData;

        if (taskData) {
            // We let EncryptTask to delete taskData
            const int index = findNote(taskData->iNoteId);

            if (index >= 0) {
                // Steal actual path from taskData
                HDEBUG("Encrypted" << qPrintable(taskData->iPath));
                iData.at(index)->iPath = taskData->iPath;
                taskData->iPath.clear();
                saveInfo();
            }
        }
        task->release();
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

bool
FoilNotesModel::Private::encrypt(
    const QString& aBody,
    const QColor& aColor,
    uint aPageNr,
    uint aReqId)
{
    if (iPrivateKey) {
        const bool wasBusy = busy();
        EncryptTask* task = new EncryptTask(iThreadPool, aBody, aColor,
            aPageNr, nextId(), iPrivateKey, iPublicKey, iFoilNotesDir,
            aReqId);

        iEncryptTasks.append(task);
        task->submit(this, SLOT(onEncryptNoteDone()));
        if (!wasBusy) {
            // We know we are busy now
            queueSignal(SignalBusyChanged);
        }
        return true;
    }
    return false;
}

void
FoilNotesModel::Private::onEncryptNoteDone()
{
    EncryptTask* task = qobject_cast<EncryptTask*>(sender());

    if (task) {
        ModelData* data = task->iData;

        HVERIFY(iEncryptTasks.removeAll(task));

        // Notify submitter that we are done with this request
        if (data) {
            // Steal data from EncryptTask
            HDEBUG("Encrypted" << qPrintable(data->iPath));
            insertModelData(task->iData);
            task->iData = Q_NULLPTR;
            saveInfo();
        }
        Q_EMIT parentObject()->encryptionDone(task->iReqId, data != Q_NULLPTR);
        task->release();
        if (!busy()) {
            // We know we were busy when we received this signal
            queueSignal(SignalBusyChanged);
        }
        emitQueuedSignals();
    }
}

void
FoilNotesModel::Private::saveInfo()
{
    // N.B. This method may change the busy state but doesn't queue
    // BusyChanged signal, it's done by the caller.
    iSaveInfoTask.reset(new SaveInfoTask(iThreadPool, ModelInfo(iData),
        iFoilNotesDir, iPrivateKey, iPublicKey));
    iSaveInfoTask->submit(this, SLOT(onSaveInfoDone()));
}

void
FoilNotesModel::Private::onSaveInfoDone()
{
    HDEBUG("Done");
    iSaveInfoTask.reset();
    if (!busy()) {
        // We know we were busy when we received this signal
        queueSignal(SignalBusyChanged);
    }
    emitQueuedSignals();
}

void
FoilNotesModel::Private::generate(
    int aBits,
    const QString& aPassword)
{
    const bool wasBusy = busy();

    iGenerateKeyTask.reset(new GenerateKeyTask(iThreadPool, iFoilKeyFile,
        aBits, aPassword));
    iGenerateKeyTask->submit(this, SLOT(onGenerateKeyTaskDone()));
    setFoilState(FoilGeneratingKey);
    if (!wasBusy) {
        // We know we are busy now
        queueSignal(SignalBusyChanged);
    }
    emitQueuedSignals();
}

void
FoilNotesModel::Private::onGenerateKeyTaskDone()
{
    HDEBUG("Got a new key");
    if (iGenerateKeyTask->iPrivateKey) {
        setKeys(iGenerateKeyTask->iPrivateKey, iGenerateKeyTask->iPublicKey);
        setFoilState(FoilNotesReady);
    } else {
        setKeys(Q_NULLPTR);
        setFoilState(FoilKeyError);
    }
    iGenerateKeyTask.reset();
    if (!busy()) {
        // We know we were busy when we received this signal
        queueSignal(SignalBusyChanged);
    }
    queueSignal(SignalKeyGenerated);
    emitQueuedSignals();
}

inline
void
FoilNotesModel::Private::updatePageNr(uint aStartPos)
{
    updatePageNr(aStartPos, iData.count());
}

void
FoilNotesModel::Private::updatePageNr(
    uint aStartPos,
    uint aEndPos)
{
    if (aStartPos < aEndPos) {
        QList<int> updateRows;

        for (uint i = aStartPos; i < aEndPos; i++) {
            ModelData* data = iData.at(i);
            const uint pagenr = i + 1;

            if (data->pagenr() != pagenr) {
                HDEBUG(data->pagenr() << "->" << pagenr);
                data->iNote.iPageNr = pagenr;
                updateRows.append(i);
            }
        }
        dataChanged(updateRows, ModelData::PageNrRole);
    }
}

void
FoilNotesModel::Private::updateText()
{
    const int n = iData.count();
    QString text;

    if (iTextIndex >= 0 && iTextIndex < n) {
        text = iData.at(iTextIndex)->body();
    } else if (n > 0) {
        text = iData.at(0)->body();
    }
    if (iText != text) {
        iText = text;
        queueSignal(SignalTextChanged);
    }
}

void
FoilNotesModel::Private::destroyItemAt(
    int aIndex)
{
    if (aIndex >= 0 && aIndex <= iData.count()) {
        FoilNotesModel* model = parentObject();

        HDEBUG(iData.at(aIndex)->pagenr());
        model->beginRemoveRows(QModelIndex(), aIndex, aIndex);
        delete iData.takeAt(aIndex);
        model->endRemoveRows();
        updateText();
        updatePageNr(aIndex);
        queueSignal(SignalCountChanged);
    }
}

bool
FoilNotesModel::Private::destroyItemAndRemoveFilesAt(
    int aIndex)
{
    ModelData* data = dataAt(aIndex);
    if (data) {
        QString path(data->iPath);
        destroyItemAt(aIndex);
        BaseTask::removeFile(path);
        return true;
    }
    return false;
}

void
FoilNotesModel::Private::deleteNoteAt(
    int aIndex)
{
    const bool wasBusy = busy();

    if (destroyItemAndRemoveFilesAt(aIndex)) {
        // saveInfo() doesn't queue BusyChanged signal, we have to do it here
        saveInfo();
        if (wasBusy != busy()) {
            queueSignal(SignalBusyChanged);
        }
    }
}

void
FoilNotesModel::Private::deleteNotes(
    QList<int> aRows)
{
    if (!aRows.isEmpty()) {
        int deleted = 0;
        const bool wasBusy = busy();

        qSort(aRows);
        for (int i = aRows.count() - 1; i >= 0; i--) {
            const int pos = aRows.at(i);
            ModelData* data = dataAt(pos);

            if (data) {
                if (data->iSelected) {
                    data->iSelected = false;
                    iSelected--;
                    queueSignal(SignalSelectedChanged);
                }
                HDEBUG(data->pagenr());
                HVERIFY(destroyItemAndRemoveFilesAt(pos));
                deleted++;
            }
        }
        if (deleted > 0) {
            saveInfo();
            if (!wasBusy) {
                // We must be busy now
                queueSignal(SignalBusyChanged);
            }
        }
    }
}

void
FoilNotesModel::Private::saveNoteAt(
    int aIndex)
{
    const bool wasBusy = busy();
    // Caller has checked the index
    ModelData* data = iData.at(aIndex);
    EncryptTask* saveTask = new EncryptTask(iThreadPool, data,
        iPrivateKey, iPublicKey, iFoilNotesDir);

    // Cancel pending encryption tasks
    bool replaced = false;

    for (int i = 0; i < iEncryptTasks.count(); i++) {
        EncryptTask* task = iEncryptTasks.at(i);

        if (task->iNoteId == data->iNoteId) {
            // A bit of optimization (reuse the list slot)
            iEncryptTasks.replace(i, saveTask);
            task->release();
            replaced = true;
            HDEBUG("Cancelled EncryptTask for note" << data->iNoteId);
            break;
        }
    }

    // Submit the new task
    if (!replaced) {
        iEncryptTasks.append(saveTask);
    }
    saveTask->submit(this, SLOT(onSaveNoteDone()));
    if (!wasBusy) {
        // We must be busy now
        queueSignal(SignalBusyChanged);
    }
}

void
FoilNotesModel::Private::setBodyAt(
    int aIndex,
    const QString& aBody)
{
    ModelData* data = dataAt(aIndex);

    if (data && data->body() != aBody) {
        data->iNote.iBody = aBody;
        saveNoteAt(aIndex);
        updateText();
        dataChanged(aIndex, ModelData::BodyRole);
    }
}

void
FoilNotesModel::Private::setColorAt(
    int aIndex,
    const QColor& aColor)
{
    ModelData* data = dataAt(aIndex);

    if (data && data->color() != aColor) {
        data->iNote.iColor = aColor;
        saveNoteAt(aIndex);
        dataChanged(aIndex, ModelData::ColorRole);
    }
}

void
FoilNotesModel::Private::clearModel()
{
    const int n = iData.count();

    if (n > 0) {
        FoilNotesModel* model = parentObject();

        model->beginRemoveRows(QModelIndex(), 0, n - 1);
        qDeleteAll(iData);
        iData.clear();
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
    }
}

void
FoilNotesModel::Private::lock(
    bool aTimeout)
{
    // Cancel whatever we are doing
    FoilNotesModel* model = parentObject();
    QList<EncryptTask*> encryptTasks;
    const bool wasBusy = busy();

    iSaveInfoTask.reset();
    iDecryptNotesTask.reset();
    encryptTasks.swap(iEncryptTasks);

    for (int i = 0; i < encryptTasks.count(); i++) {
        EncryptTask* task = encryptTasks.at(i);
        Q_EMIT model->encryptionDone(task->iReqId, false);
        task->release();
    }

    // Destroy decrypted notes
    if (!iData.isEmpty()) {
        model->beginRemoveRows(QModelIndex(), 0, iData.count() - 1);
        qDeleteAll(iData);
        iData.clear();
        model->endRemoveRows();
        queueSignal(SignalCountChanged);
    }
    if (busy() != wasBusy) {
        queueSignal(SignalBusyChanged);
    }
    if (iPrivateKey) {
        // Throw the keys away
        setKeys(Q_NULLPTR);
        setFoilState(aTimeout ? FoilLockedTimedOut : FoilLocked);
        HDEBUG("Locked");
    } else {
        HDEBUG("Nothing to lock, there's no key yet!");
    }
}

bool
FoilNotesModel::Private::unlock(
    const QString& aPassword)
{
    GError* error = Q_NULLPTR;
    const QByteArray path(iFoilKeyFile.toUtf8());
    bool ok = false;

    // First make sure that it's encrypted
    HDEBUG(iFoilKeyFile);
    FoilPrivateKey* key = foil_private_key_decrypt_from_file
        (FOIL_KEY_RSA_PRIVATE, path.constData(), Q_NULLPTR, &error);

    if (key) {
        HWARN("Key not encrypted");
        setFoilState(FoilKeyNotEncrypted);
        foil_private_key_unref(key);
    } else if (error->domain == FOIL_ERROR) {
        if (error->code == FOIL_ERROR_KEY_ENCRYPTED) {
            // Then try to decrypt it
            const QByteArray password(aPassword.toUtf8());
            g_clear_error(&error);
            key = foil_private_key_decrypt_from_file
                (FOIL_KEY_RSA_PRIVATE, path.constData(),
                    password.constData(), &error);
            if (key) {
                HDEBUG("Password accepted, thank you!");
                setKeys(key);
                // Now that we know the key, decrypt the notes
                iDecryptNotesTask.reset(new DecryptNotesTask(iThreadPool,
                    iFoilNotesDir, iPrivateKey, iPublicKey, &iNextId));
                clearModel();
                connect(iDecryptNotesTask.data(),
                    SIGNAL(progress(DecryptNotesTask::Progress::Ptr)),
                    SLOT(onDecryptNotesProgress(DecryptNotesTask::Progress::Ptr)),
                    Qt::QueuedConnection);
                iDecryptNotesTask->submit(this, SLOT(onDecryptNotesTaskDone()));
                setFoilState(FoilDecrypting);
                foil_private_key_unref(key);
                ok = true;
            } else {
                HDEBUG("Wrong password");
                g_error_free(error);
                setFoilState(FoilLocked);
            }
        } else {
            HWARN("Key invalid:" << error->message);
            g_error_free(error);
            setFoilState(FoilKeyInvalid);
        }
    } else {
        HWARN(error->message);
        g_error_free(error);
        setFoilState(FoilKeyMissing);
    }
    return ok;
}

bool
FoilNotesModel::Private::busy() const
{
    if (iSaveInfoTask ||
        iGenerateKeyTask ||
        iDecryptNotesTask ||
        !iEncryptTasks.isEmpty()) {
        return true;
    } else {
        return false;
    }
}

// ==========================================================================
// FoilNotesModel
// ==========================================================================

FoilNotesModel::FoilNotesModel(
    QObject* aParent) :
    FoilNotesBaseModel(aParent),
    iPrivate(new Private(this))
{}

// Callback for qmlRegisterSingletonType<FoilNotesModel>
QObject*
FoilNotesModel::createSingleton(
    QQmlEngine*,
    QJSEngine*)
{
    return new FoilNotesModel();
}

QHash<int,QByteArray>
FoilNotesModel::roleNames() const
{
    QHash<int,QByteArray> roles;

    #define ROLE(X,x) roles.insert(ModelData::X##Role, #x);
    MODEL_ROLES(ROLE)
    #undef ROLE
    return roles;
}

int
FoilNotesModel::rowCount(
    const QModelIndex&) const
{
    return iPrivate->rowCount();
}

QVariant
FoilNotesModel::data(
    const QModelIndex& aIndex,
    int aRole) const
{
    ModelData* data = iPrivate->dataAt(aIndex.row());

    return data ? data->get(ModelData::Role(aRole)) : QVariant();
}

bool
FoilNotesModel::setData(
    const QModelIndex& aIndex,
    const QVariant& aValue,
    int aRole)
{
    const int row = aIndex.row();
    ModelData* data = iPrivate->dataAt(row);

    if (data) {
        QVector<int> roles;

        switch ((ModelData::Role)aRole) {
        case ModelData::BodyRole:
            {
                const QString body(aValue.toString());

                if (data->body() != body) {
                    data->iNote.iBody = body;
                    iPrivate->saveNoteAt(row);
                    iPrivate->updateText();
                    iPrivate->emitQueuedSignals();
                    roles.append(aRole);
                    Q_EMIT dataChanged(aIndex, aIndex, roles);
                }
            }
            return true;
        case ModelData::ColorRole:
            {
                QColor color(aValue.toString());

                if (color.isValid()) {
                    if (data->color() != color) {
                        data->iNote.iColor = color;
                        iPrivate->saveNoteAt(row);
                        iPrivate->emitQueuedSignals();
                        roles.append(aRole);
                        Q_EMIT dataChanged(aIndex, aIndex, roles);
                    }
                    return true;
                }
            }
            break;
        case ModelData::SelectedRole:
            {
                const bool selected(aValue.toBool());

                if (data->iSelected != selected) {
                    HDEBUG(data->pagenr() << selected);
                    data->iSelected = selected;
                    if (selected) {
                        iPrivate->iSelected++;
                    } else {
                        HASSERT(iPrivate->iSelected > 0);
                        iPrivate->iSelected--;
                    }
                    iPrivate->queueSignal(SignalSelectedChanged);
                    iPrivate->emitQueuedSignals();
                    roles.append(aRole);
                    Q_EMIT dataChanged(aIndex, aIndex, roles);
                }
            }
            return true;
        // No default to make sure that we get "warning: enumeration value
        // not handled in switch" if we forget to handle a real role.
        case ModelData::PageNrRole:
        case ModelData::FirstRole:
        case ModelData::LastRole:
            break;
        }
    }
    return false;
}

bool
FoilNotesModel::moveRows(
    const QModelIndex& aSrcParent,
    int aSrcRow,
    int aCount,
    const QModelIndex& aDestParent,
    int aDestRow)
{
    const int size = iPrivate->rowCount();

    if (aSrcParent == aDestParent &&
        aSrcRow != aDestRow &&
        aSrcRow >= 0 && aSrcRow < size &&
        aDestRow >= 0 && aDestRow < size) {
        HDEBUG(aSrcRow << "->" << aDestRow);
        const bool wasBusy = iPrivate->busy();

        beginMoveRows(aSrcParent, aSrcRow, aSrcRow, aDestParent,
            (aDestRow < aSrcRow) ? aDestRow : (aDestRow + 1));
        iPrivate->iData.move(aSrcRow, aDestRow);
        endMoveRows();

        iPrivate->updatePageNr(qMin(aSrcRow, aDestRow), qMax(aSrcRow, aDestRow) + 1);
        iPrivate->updateText();
        iPrivate->saveInfo();
        if (!wasBusy) {
            iPrivate->queueSignal(SignalBusyChanged);
        }
        iPrivate->emitQueuedSignals();
        return true;
    } else {
        return false;
    }
}

bool
FoilNotesModel::busy() const
{
    return iPrivate->busy();
}

bool
FoilNotesModel::keyAvailable() const
{
    return iPrivate->iPrivateKey != Q_NULLPTR;
}

FoilNotesModel::FoilState
FoilNotesModel::foilState() const
{
    return iPrivate->iFoilState;
}

QString
FoilNotesModel::text() const
{
    return iPrivate->iText;
}

int
FoilNotesModel::textIndex() const
{
    return iPrivate->iTextIndex;
}

void
FoilNotesModel::setTextIndex(
    int aIndex)
{
    if (iPrivate->iTextIndex != aIndex) {
        iPrivate->iTextIndex = aIndex;
        iPrivate->queueSignal(SignalTextIndexChanged);
        iPrivate->updateText();
        iPrivate->emitQueuedSignals();
    }
}

bool
FoilNotesModel::checkPassword(
    QString aPassword)
{
    return iPrivate->checkPassword(aPassword);
}

bool
FoilNotesModel::changePassword(
    QString aOld,
    QString aNew)
{
    bool ok = iPrivate->changePassword(aOld, aNew);

    iPrivate->emitQueuedSignals();
    return ok;
}

void
FoilNotesModel::generateKey(
    int aBits,
    QString aPassword)
{
    iPrivate->generate(aBits, aPassword);
    iPrivate->emitQueuedSignals();
}

void
FoilNotesModel::lock(
    bool aTimeout)
{
    iPrivate->lock(aTimeout);
    iPrivate->emitQueuedSignals();
}

bool
FoilNotesModel::unlock(
    QString aPassword)
{
    const bool ok = iPrivate->unlock(aPassword);

    iPrivate->emitQueuedSignals();
    return ok;
}

void
FoilNotesModel::addNote(
    QColor aColor,
    QString aBody)
{
    HDEBUG(aColor.name() << aBody);
    iPrivate->addNote(aColor, aBody);
    iPrivate->emitQueuedSignals();
}

void
FoilNotesModel::deleteNoteAt(
    int aIndex)
{
    HDEBUG(aIndex);
    iPrivate->deleteNoteAt(aIndex);
    iPrivate->emitQueuedSignals();
}

void
FoilNotesModel::setBodyAt(
    int aIndex,
    QString aBody)
{
    iPrivate->setBodyAt(aIndex, aBody);
    iPrivate->emitQueuedSignals();
}

void
FoilNotesModel::setColorAt(
    int aIndex,
    QColor aColor)
{
    iPrivate->setColorAt(aIndex, aColor);
    iPrivate->emitQueuedSignals();
}

void
FoilNotesModel::deleteNotes(
    QList<int> aRows)
{
    iPrivate->deleteNotes(aRows);
    iPrivate->emitQueuedSignals();
}

bool
FoilNotesModel::encryptNote(
    QString aBody,
    QColor aColor,
    int aPageNr,
    int aReqId)
{
    HDEBUG(aReqId << aColor.name() << aBody);
    bool ok = iPrivate->encrypt(aBody, aColor, aPageNr, aReqId);

    iPrivate->emitQueuedSignals();
    return ok;
}

const FoilNotesBaseModel::Note*
FoilNotesModel::noteAt(
    int aRow) const
{
    const ModelData* data = iPrivate->dataAt(aRow);

    return data ? &data->iNote : Q_NULLPTR;
}

bool
FoilNotesModel::selectedAt(
    int aRow) const
{
    const ModelData* data = iPrivate->dataAt(aRow);

    return data && data->iSelected;
}

int
FoilNotesModel::selectedCount() const
{
    return iPrivate->iSelected;
}

void
FoilNotesModel::setSelectedAt(
    int aRow,
    bool aSelected)
{
    ModelData* data = iPrivate->dataAt(aRow);

    if (data) {
        HDEBUG(data->pagenr() << aSelected);
        if (data->iSelected && !aSelected) {
            HASSERT(iPrivate->iSelected > 0);
            data->iSelected = false;
            iPrivate->iSelected--;
            iPrivate->queueSignal(SignalSelectedChanged);
        } else if (!data->iSelected && aSelected) {
            HASSERT(iPrivate->iSelected < iPrivate->rowCount());
            data->iSelected = true;
            iPrivate->iSelected++;
            iPrivate->queueSignal(SignalSelectedChanged);
        }
        iPrivate->dataChanged(aRow, ModelData::SelectedRole);
    }
}

void
FoilNotesModel::emitQueuedSignals()
{
    iPrivate->emitQueuedSignals();
}

#include "FoilNotesModel.moc"
