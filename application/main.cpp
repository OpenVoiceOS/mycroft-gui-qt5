/*
 * Copyright 2018 by Marco Martin <mart@kde.org>
 * Copyright 2018 David Edmundson <davidedmundson@kde.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <QQuickView>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QQmlContext>
#include <QtQml>
#include <QDebug>
#include <QCursor>
#include <QtWebView/QtWebView>

#include <QApplication>
#include <KDBusService>

#include "appsettings.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    QStringList arguments;
    for (int a = 0; a < argc; ++a) {
        arguments << QString::fromLocal8Bit(argv[a]);
    }

    QCommandLineParser parser;

    auto widthOption = QCommandLineOption(QStringLiteral("width"), QStringLiteral("Width of the screen"), QStringLiteral("width"));
    auto heightOption = QCommandLineOption(QStringLiteral("height"), QStringLiteral("Height of the screen"), QStringLiteral("height"));
    auto hideTextInputOption = QCommandLineOption(QStringLiteral("hideTextInput"), QStringLiteral("Hide the input box"));
    auto dpiOption = QCommandLineOption(QStringLiteral("dpi"), QStringLiteral("dpi"), QStringLiteral("dpi"));
    auto skillOption = QCommandLineOption(QStringLiteral("skill"), QStringLiteral("Single skill to load"), QStringLiteral("skill"));
    auto maximizeOption = QCommandLineOption(QStringLiteral("maximize"), QStringLiteral("When set, start maximized."));
    auto rotateScreen = QCommandLineOption(QStringLiteral("rotateScreen"), QStringLiteral("When set, rotate the screen by set degrees."), QStringLiteral("degrees"));
    auto helpOption = QCommandLineOption(QStringLiteral("help"), QStringLiteral("Show this help message"));
    parser.addOptions({widthOption, heightOption, hideTextInputOption, skillOption,
                       dpiOption, maximizeOption,
                       rotateScreen, helpOption});
    parser.process(arguments);


    qputenv("QT_WAYLAND_FORCE_DPI", parser.value(dpiOption).toLatin1());

    QApplication app(argc, argv);

    app.setApplicationName(QStringLiteral("mycroft.gui"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("mycroft")));

    // NOTE: Have to manually implement a --help option because the parser.addHelpOption() would
    //       be triggered at parser.process() time, but it requires the QApplication. But the
    //       'dpi' option for the GUI creates a chicken-and-the-egg issue.
    if (parser.isSet(helpOption)) {
        parser.showHelp();
        return 0;
    }

    QtWebView::initialize();

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    int width = parser.value(widthOption).toInt();
    int height = parser.value(heightOption).toInt();
    int rotation = parser.value(rotateScreen).toInt();
    bool maximize = parser.isSet(maximizeOption);

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("deviceWidth"), width);
    engine.rootContext()->setContextProperty(QStringLiteral("deviceHeight"), height);
    engine.rootContext()->setContextProperty(QStringLiteral("deviceMaximized"), maximize);
    engine.rootContext()->setContextProperty(QStringLiteral("hideTextInput"), parser.isSet(hideTextInputOption));
    engine.rootContext()->setContextProperty(QStringLiteral("globalScreenRotation"), parser.isSet(rotateScreen) ? rotation : 0);
    engine.rootContext()->setContextProperty(QStringLiteral("versionNumber"), QStringLiteral(mycroftguiapp_VERSION_STRING));

    engine.rootContext()->setContextProperty(QStringLiteral("keyFilter"), 0);
    engine.rootContext()->setContextProperty(QStringLiteral("isAndroid"), false);

    QString singleSkill = parser.value(skillOption);
    if (singleSkill.endsWith(QStringLiteral(".home"))) {
        singleSkill = singleSkill.left(singleSkill.indexOf(QStringLiteral(".home")));
        engine.rootContext()->setContextProperty(QStringLiteral("singleSkill"), singleSkill);
        engine.rootContext()->setContextProperty(QStringLiteral("singleSkillHome"), parser.value(skillOption));
    } else {
        engine.rootContext()->setContextProperty(QStringLiteral("singleSkill"), singleSkill);
        engine.rootContext()->setContextProperty(QStringLiteral("singleSkillHome"), QString());
    }

    if (parser.isSet(skillOption)) {
        app.setApplicationName(QStringLiteral("mycroft.gui.") + singleSkill);
        KDBusService service(KDBusService::Unique);
    }

    AppSettings *appSettings = new AppSettings(&view);
    engine.rootContext()->setContextProperty(QStringLiteral("applicationSettings"), appSettings);

    qmlRegisterType<SpeechIntent>("org.kde.private.mycroftgui", 1, 0, "SpeechIntent");

    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
