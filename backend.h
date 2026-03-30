#pragma once
#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

class Backend : public QObject {
    Q_OBJECT //macro to make it qml stuff

    //create property for events
    //calls events() to get stuff
    //eventsChanged fires in QML to update bindings
    Q_PROPERTY(QString events READ events NOTIFY eventsChanged) //events
    Q_PROPERTY(QString weather READ weather NOTIFY weatherChanged) //weather
    Q_PROPERTY(QString liveWeather READ liveWeather NOTIFY liveWeatherChanged) //live weather
    Q_PROPERTY(QStringList pictures READ pictures NOTIFY picturesChanged) //pictures

public:
    explicit Backend(QObject *parent = nullptr);

    QString events() const { return m_events; } //return m_events for QML
    QString weather() const { return m_weather; } //return m_weather for QML
    QString liveWeather() const { return m_liveWeather; } //return m_weather for QML
    QStringList pictures() const { return m_pictures; } //return m_pictures for QML


public slots:
    void fetchEvents();
    void fetchWeather(); //handles all weather
    void loadPictures(); //load pictures in picture folder

signals:
    void eventsChanged();
    void weatherChanged();
    void liveWeatherChanged();
    void picturesChanged();

private:
    void loadConfig(); //load info in config.json

    //config variables
    QString m_calendarUrl;
    QString m_weatherApiUrl;

    QNetworkAccessManager m_nam; //network access manager for everything

    //variables given to QML
    QString m_events;
    QString m_weather;
    QString m_liveWeather;
    QStringList m_pictures;

    void setEvents(const QString &eventsString);
    void setWeather(const QString &weatherString);
    void setLiveWeather(const QString &liveWeatherString);

    QString parseEvents(const QString &icsText);
    QString parseWeather(const QString &json);
    QString parseLiveWeather(const QString &json);

    QString getWeatherEmoji(int code, int rainChance);
};
