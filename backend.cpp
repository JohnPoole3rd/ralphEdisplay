#include <QNetworkReply>
#include <utility>
#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include "backend.h"

//constructor for Backend
Backend::Backend(QObject *parent)
    : QObject(parent) //create QObject with the parent
    , m_events("No events found")
    , m_weather("No weather found")
    , m_liveWeather("No live weather found")
{
    loadConfig();
    loadPictures();
}

//load config.json
void Backend::loadConfig() {
    QFile file(QCoreApplication::applicationDirPath() + "/config.json");

    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "No config.json found, exiting";
        QCoreApplication::exit(1);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll()); //create json doc
    QJsonObject config = doc.object(); //create json object

    //no defaults because functions that call these already handle throwing errors
    m_calendarUrl   =   config["calendarUrl"].toString();
    m_weatherApiUrl =   config["weatherApiUrl"].toString();
}

//load pictures in pictures folder
void Backend::loadPictures() {
    QDir dir(QCoreApplication::applicationDirPath() + "/pictures");
    dir.setNameFilters({"*.jpg", "*.jpeg", "*.png"});
    for (const QFileInfo &file : dir.entryInfoList(QDir::Files))
        m_pictures.append("file://" + file.absoluteFilePath());
    emit picturesChanged();
}

//CALENDER EVENTS--------------------------------------------------

void Backend::setEvents(const QString &eventsString) {
    m_events = eventsString; //set m_events to the input events string
    emit eventsChanged();
}

//what QML calls to fetch events and set m_events from URL
void Backend::fetchEvents() {
    QNetworkRequest req{QUrl(m_calendarUrl)};
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            setEvents("Error: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        const QString icsText = QString::fromUtf8(reply->readAll());
        reply->deleteLater();

        setEvents(parseEvents(icsText));
    });
}

//used for expanding rrules in ics files for repeating events
void expandRRule(const QString &rrule, const QDateTime &dtstart,
                 const QString &summary, QList<QPair<QDateTime, QString>> &allEvents) {

    QDateTime limit = QDateTime::currentDateTime().addDays(60);

    QMap<QString, QString> parts;
    for (const QString &part : rrule.split(';')) {
        QStringList kv = part.split('=');
        if (kv.size() == 2)
            parts[kv[0]] = kv[1];
    }

    QString freq  = parts.value("FREQ");
    int interval  = parts.value("INTERVAL", "1").toInt();
    int count     = parts.value("COUNT", "-1").toInt();
    QString until = parts.value("UNTIL");
    QString byday = parts.value("BYDAY");

    QDateTime untilDt;
    if (!until.isEmpty()) {
        if (until.contains('T')) {
            if (until.endsWith('Z'))
                untilDt = QDateTime::fromString(until, "yyyyMMddThhmmss'Z'").toLocalTime();
            else
                untilDt = QDateTime::fromString(until, "yyyyMMddThhmmss");
        } else {
            // date only, no time component
            QDate untilDate = QDate::fromString(until, "yyyyMMdd");
            if (untilDate.isValid())
                untilDt = QDateTime(untilDate, QTime(23, 59, 59), QTimeZone::LocalTime);
        }
    }

    QList<int> byDays;
    if (!byday.isEmpty()) {
        QMap<QString, int> dayMap = {
            {"SU",0},{"MO",1},{"TU",2},{"WE",3},{"TH",4},{"FR",5},{"SA",6}
        };
        for (const QString &day : byday.split(',')) {
            QString d = day;
            d.remove(QRegularExpression("^-?\\d+"));
            if (dayMap.contains(d))
                byDays.append(dayMap[d]);
        }
    }

    QDateTime current = dtstart;
    if (freq == "DAILY")        current = current.addDays(interval);
    else if (freq == "WEEKLY")  current = current.addDays(7 * interval);
    else if (freq == "MONTHLY") current = current.addMonths(interval);
    else if (freq == "YEARLY")  current = current.addYears(interval);
    else return;

    int fired = 0;

    while (true) {
        if (count != -1 && fired >= count) break;
        if (untilDt.isValid() && current > untilDt) break;
        if (current > limit) break;

        if (!byDays.isEmpty()) {
            QDateTime periodEnd;
            if (freq == "WEEKLY")       periodEnd = current.addDays(7 * interval);
            else if (freq == "MONTHLY") periodEnd = current.addMonths(interval);
            else if (freq == "YEARLY")  periodEnd = current.addYears(interval);
            else periodEnd = current.addDays(interval);

            QDateTime day = current;
            while (day < periodEnd) {
                int dow = day.date().dayOfWeek() % 7;
                if (byDays.contains(dow)) {
                    if (count != -1 && fired >= count) break;
                    if (untilDt.isValid() && day > untilDt) break;
                    if (day > limit) break;
                    allEvents.append(qMakePair(day, summary));
                    fired++;
                }
                day = day.addDays(1);
            }
            current = periodEnd;
        } else {
            allEvents.append(qMakePair(current, summary));
            fired++;

            if (freq == "DAILY")        current = current.addDays(interval);
            else if (freq == "WEEKLY")  current = current.addDays(7 * interval);
            else if (freq == "MONTHLY") current = current.addMonths(interval);
            else if (freq == "YEARLY")  current = current.addYears(interval);
            else break;
        }
    }
}

//function for parsing ics file
QString Backend::parseEvents(const QString &icsText) {
    QStringList events;
    QList<QPair<QDateTime, QString>> allEvents;
    QStringList lines = icsText.split("\n", Qt::SkipEmptyParts);

    QString currentSummary;
    QString currentDtstart;
    QString currentRRule;
    bool inEvent = false;
    bool isAllDay = false;

    for (const QString &line : lines) {
        QString l = line.trimmed();

        if (l.startsWith("BEGIN:VEVENT")) {
            inEvent = true;
            currentSummary.clear();
            currentDtstart.clear();
            currentRRule.clear();
            isAllDay = false;
        }
        else if (l.startsWith("SUMMARY:")) {
            currentSummary = l.mid(8).trimmed();
        }
        else if (l.startsWith("DTSTART") && currentDtstart.isEmpty()) {
            currentDtstart = l.section(':', -1).trimmed();
            isAllDay = (currentDtstart.length() == 8);
        }
        else if (l.startsWith("RRULE:")) {
            currentRRule = l.mid(6).trimmed();
        }
        else if (l.startsWith("END:VEVENT") && inEvent) {
            if (!currentSummary.isEmpty() && !currentDtstart.isEmpty()) {
                QDateTime dt;
                if (isAllDay) {
                    QDate date = QDate::fromString(currentDtstart, "yyyyMMdd");
                    dt = QDateTime(date, QTime(0, 0), QTimeZone::LocalTime);
                } else {
                    QString utcDt = currentDtstart;
                    if (!utcDt.endsWith('Z')) utcDt += 'Z';
                    QDateTime parsed = QDateTime::fromString(utcDt, "yyyyMMddThhmmss'Z'");
                    if (parsed.isValid())
                        dt = parsed.toLocalTime();
                }

                if (dt.isValid()) {
                    if (currentRRule.isEmpty())
                        allEvents.append(qMakePair(dt, currentSummary));
                    else
                        expandRRule(currentRRule, dt, currentSummary, allEvents);
                }
            }
            inEvent = false;
        }
    }

    std::sort(allEvents.begin(), allEvents.end(),
            [](const QPair<QDateTime, QString> &a, const QPair<QDateTime, QString> &b) {
                return a.first < b.first;
            });

    QDateTime now = QDateTime::currentDateTime();
    int count = 0;
    for (const auto &event : allEvents) {
        if (event.first > now && count <  8) {
            QString formatted = (event.first.time() == QTime(0, 0))
            ? event.first.toString("M/d/yy")
            : event.first.toString("M/d/yy, h:mm AP");
            events.append(event.second + ", " + formatted);
            count++;
        }
    }

    return events.isEmpty() ? "No events found" : events.join("\n\n");
}
//Weather-----------------------------------------------------------

void Backend::setWeather(const QString &weatherString) {
    m_weather = weatherString;
    emit weatherChanged();
}

void Backend::setLiveWeather(const QString &liveWeatherString) {
    m_liveWeather = liveWeatherString;
    emit liveWeatherChanged();
}

//called my QML on timer
void Backend::fetchWeather() {
    QNetworkRequest req{QUrl(m_weatherApiUrl)};
    auto *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            setWeather("Error: " + reply->errorString());
            setLiveWeather("Error: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        QString json = reply->readAll();
        reply->deleteLater();

        setWeather(parseWeather(json));
        setLiveWeather(parseLiveWeather(json));
    });
}

QString Backend::getWeatherEmoji(int code, int rainChance) {
    if (rainChance > 70) return "🌧️";  // Heavy rain
    if (rainChance > 30) return "🌦️";  // Light rain

    switch (code) {
        case 0: case 1: case 2: case 3: return "☀️";   // Clear sky
        case 45: case 48: return "🌫️";                 // Fog
        case 51: case 53: case 55:                     // Drizzle
        case 61: case 63: case 65: return "🌦️";        // Rain
        case 71: case 73: case 75: case 77: return "❄️"; // Snow
        case 95: case 96: case 99: return "⛈️";        // Thunderstorm
        default: return "🌤️";                          // Partly cloudy
    }
}

QString Backend::parseWeather(const QString &json) {
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        setWeather("Invalid weather JSON");
        return("Invalid weather JSON");
    }

    QJsonObject root = doc.object();
    QJsonObject daily = root["daily"].toObject();

    QStringList forecast;

    QJsonArray highs = daily["temperature_2m_max"].toArray();
    QJsonArray lows = daily["temperature_2m_min"].toArray();
    QJsonArray rainChance = daily["precipitation_probability_max"].toArray();
    QJsonArray weatherCodes = daily["weather_code"].toArray();

    for (int i = 0; i < 7 && i < highs.size(); ++i) {
        int high = static_cast<int>(highs.at(i).toDouble());
        int low  = static_cast<int>(lows.at(i).toDouble());
        int rain = rainChance[i].toInt();
        int code = weatherCodes[i].toInt();

        QString emoji = getWeatherEmoji(code, rain);
        QString day = QDate::currentDate().addDays(i + 1).toString("ddd");

        // Emoji at end
        forecast.append(QString("%1 %2°/%3° %4% %5")
            .arg(day)
            .arg(high)
            .arg(low)
            .arg(rain)
            .arg(emoji));
    }

    return( forecast.join("\n") );
}

QString Backend::parseLiveWeather(const QString &json) {
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        setWeather("Invalid weather JSON");
        return("Invalid weather JSON");
    }

    QJsonObject root = doc.object();
    QJsonObject live = root["current"].toObject();

    int liveTemp = static_cast<int>(live["temperature_2m"].toDouble());
    int liveCode = live["weather_code"].toInt();
    QString liveEmoji = getWeatherEmoji(liveCode, 0);

    return( QString("%1°F %2").arg(liveTemp).arg(liveEmoji) );
}
