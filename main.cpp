#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "backend.h"

int main(int argc, char *argv[]) {
    QGuiApplication QTralphEdisplay(argc, argv);
    QQmlApplicationEngine engine;

    Backend backend; //create backend from backend.h
    engine.rootContext()->setContextProperty("backend", &backend); //inject

    engine.load(QUrl("qrc:/qt/qml/QTralphEdisplay/main.qml"));
    return QTralphEdisplay.exec();
}
