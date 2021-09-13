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

#include "FoilNotesDefs.h"
#include "FoilNotesHints.h"
#include "FoilNotesModel.h"
#include "FoilNotesPlaintextModel.h"
#include "FoilNotesSearchModel.h"
#include "FoilNotesSettings.h"
#include "FoilNotes.h"

#include "HarbourQrCodeGenerator.h"
#include "HarbourQrCodeImageProvider.h"

#include "HarbourDebug.h"
#include "HarbourColorEditorModel.h"
#include "HarbourOrganizeListModel.h"
#include "HarbourProcessState.h"
#include "HarbourSystemState.h"
#include "HarbourTheme.h"

#include <sailfishapp.h>
#include <gutil_log.h>

#include <QGuiApplication>
#include <QtQuick>

static void register_types(const char* uri, int v1 = 1, int v2 = 0)
{
    qmlRegisterSingletonType<HarbourProcessState>(uri, v1, v2, "HarbourProcessState", HarbourProcessState::createSingleton);
    qmlRegisterSingletonType<HarbourSystemState>(uri, v1, v2, "HarbourSystemState", HarbourSystemState::createSingleton);
    qmlRegisterSingletonType<HarbourTheme>(uri, v1, v2, "HarbourTheme", HarbourTheme::createSingleton);
    qmlRegisterSingletonType<FoilNotesHints>(uri, v1, v2, "FoilNotesHints", FoilNotesHints::createSingleton);
    qmlRegisterSingletonType<FoilNotesSettings>(uri, v1, v2, "FoilNotesSettings", FoilNotesSettings::createSingleton);
    qmlRegisterSingletonType<FoilNotes>(uri, v1, v2, "FoilNotes", FoilNotes::createSingleton);
    qmlRegisterSingletonType<FoilNotesModel>(uri, v1, v2, "FoilNotesModel", FoilNotesModel::createSingleton);
    qmlRegisterSingletonType<FoilNotesPlaintextModel>(uri, v1, v2, "FoilNotesPlaintextModel", FoilNotesPlaintextModel::createSingleton);
    qmlRegisterType<FoilNotesSearchModel>(uri, v1, v2, "FoilNotesSearchModel");
    qmlRegisterType<HarbourColorEditorModel>(uri, v1, v2, "HarbourColorEditorModel");
    qmlRegisterType<HarbourOrganizeListModel>(uri, v1, v2, "HarbourOrganizeListModel");
    qmlRegisterType<HarbourQrCodeGenerator>(uri, v1, v2, "HarbourQrCodeGenerator");
}

int main(int argc, char *argv[])
{
    QGuiApplication* app = SailfishApp::application(argc, argv);
    app->setApplicationName(FOILNOTES_APP_NAME);

    register_types(FOILNOTES_QML_IMPORT, 1, 0);

#ifdef HARBOUR_TRANSFER_METHODS_MODEL_H
    HarbourTransferMethodInfo2::registerTypes();
#endif

    // Load translations
    QLocale locale;
    QTranslator* tr = new QTranslator(app);
#ifdef OPENREPOS
    // OpenRepos build has settings applet
    const QString transDir("/usr/share/translations");
#else
    const QString transDir = SailfishApp::pathTo("translations").toLocalFile();
#endif
    const QString transFile(FOILNOTES_APP_NAME);
    if (tr->load(locale, transFile, "-", transDir) ||
        tr->load(transFile, transDir)) {
        app->installTranslator(tr);
    } else {
        HDEBUG("Failed to load translator for" << locale);
        delete tr;
    }
#ifdef HARBOUR_TRANSFER_METHODS_MODEL_H
    tr = new QTranslator(app);
    if (HarbourTransferMethodsModel::loadTranslations(tr, locale) ||
        HarbourTransferMethodsModel::loadTranslations(tr, QLocale("en_GB"))) {
        app->installTranslator(tr);
    } else {
        delete tr;
    }
#endif

    gutil_log_timestamp = FALSE;
    gutil_log_func = gutil_log_stdout;
    gutil_log_default.name = FOILNOTES_APP_NAME;
#ifdef DEBUG
    gutil_log_default.level = GLOG_LEVEL_VERBOSE;
#endif

    // Create the view
    QQuickView* view = SailfishApp::createView();
    QQmlContext* context = view->rootContext();
    QQmlEngine* engine = context->engine();

    engine->addImageProvider("qrcode", new HarbourQrCodeImageProvider);

    // Initialize global properties
    context->setContextProperty("MaximumHintCount", 1);
    context->setContextProperty("FoilNotesAppName", FOILNOTES_APP_NAME);

    // Initialize the view and show it
    view->setTitle(qtTrId("foilnotes-app_name"));
    view->setSource(SailfishApp::pathTo("qml/main.qml"));
    view->showFullScreen();

    int ret = app->exec();

    delete view;
    delete app;
    return ret;
}
