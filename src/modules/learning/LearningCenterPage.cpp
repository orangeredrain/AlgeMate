#include "LearningCenterPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDate>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QVector>
#include <QShowEvent>
#include <QStringList>
#include <QtMath>

#include <algorithm>

namespace AlgeMate::Learning {

namespace {

constexpr int kAutoCheckinSeconds = 5 * 60;

struct ModuleStat {
    QString key;
    QString label;
    QColor color;
    int seconds = 0;
};

static QVector<ModuleStat> moduleDefinitions()
{
    return {
        {QStringLiteral("overview"), QStringLiteral("学习中心"), QColor(QStringLiteral("#6A5AE0")), 0},
        {QStringLiteral("knowledge"), QStringLiteral("知识点学习"), QColor(QStringLiteral("#2E9F68")), 0},
        {QStringLiteral("practice"), QStringLiteral("练习模式"), QColor(QStringLiteral("#F59E0B")), 0},
        {QStringLiteral("exam"), QStringLiteral("考试模式"), QColor(QStringLiteral("#EF476F")), 0},
        {QStringLiteral("wrongbook"), QStringLiteral("错题本"), QColor(QStringLiteral("#4F8EF7")), 0},
        {QStringLiteral("management"), QStringLiteral("学习管理"), QColor(QStringLiteral("#8A63D2")), 0}
    };
}

static QString dateKey(const QDate& date)
{
    return date.toString(Qt::ISODate);
}

static int studySecondsForDate(const QDate& date)
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("learning/studySecondsByDate"));
    const int seconds = settings.value(dateKey(date), 0).toInt();
    settings.endGroup();
    return seconds;
}

static bool isCheckedInDate(const QDate& date)
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("learning/checkinsByDate"));
    const bool checkedIn = settings.value(dateKey(date), false).toBool();
    settings.endGroup();
    return checkedIn || studySecondsForDate(date) > kAutoCheckinSeconds;
}

static QString formatMinutes(int seconds)
{
    const int minutes = seconds / 60;
    if (minutes < 1) {
        return QStringLiteral("不足 1 分钟");
    }
    return QStringLiteral("%1 分钟").arg(minutes);
}

static QVector<ModuleStat> moduleStatsForDate(const QDate& date)
{
    QVector<ModuleStat> stats = moduleDefinitions();

    QSettings settings;
    settings.beginGroup(QStringLiteral("learning/moduleSecondsByDate"));
    settings.beginGroup(dateKey(date));

    int knownSeconds = 0;
    for (ModuleStat& stat : stats) {
        stat.seconds = settings.value(stat.key, 0).toInt();
        knownSeconds += stat.seconds;
    }

    settings.endGroup();
    settings.endGroup();

    const int totalSeconds = studySecondsForDate(date);
    if (totalSeconds > knownSeconds && !stats.isEmpty()) {
        stats[0].seconds += totalSeconds - knownSeconds;
    }

    return stats;
}

static QString moduleSummaryForDate(const QDate& date)
{
    QStringList parts;
    const QVector<ModuleStat> stats = moduleStatsForDate(date);
    for (const ModuleStat& stat : stats) {
        if (stat.seconds <= 0) {
            continue;
        }
        parts << QStringLiteral("%1 %2").arg(stat.label, formatMinutes(stat.seconds));
    }

    return parts.isEmpty() ? QStringLiteral("暂无模块分布") : parts.join(QStringLiteral(" · "));
}

class StudyTrendChart : public QWidget {
public:
    explicit StudyTrendChart(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(220);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        struct DayData {
            QDate date;
            int seconds = 0;
            int minutes = 0;
        };

        QVector<DayData> days;
        days.reserve(7);

        QSettings settings;
        settings.beginGroup(QStringLiteral("learning/studySecondsByDate"));

        const QDate today = QDate::currentDate();
        int maxMinutes = 0;
        for (int i = 6; i >= 0; --i) {
            const QDate date = today.addDays(-i);
            const int seconds = settings.value(date.toString(Qt::ISODate), 0).toInt();
            const int minutes = seconds / 60;
            days.push_back({date, seconds, minutes});
            maxMinutes = std::max(maxMinutes, minutes);
        }

        settings.endGroup();

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), Qt::transparent);

        const QRect chartRect = rect().adjusted(24, 24, -24, -42);
        const int baselineY = chartRect.bottom();
        const int chartHeight = chartRect.height();
        const int safeMaxMinutes = std::max(maxMinutes, 1);

        QPen gridPen(QColor(QStringLiteral("#EEF0F6")));
        gridPen.setWidth(1);
        painter.setPen(gridPen);
        for (int i = 0; i <= 3; ++i) {
            const int y = chartRect.top() + chartHeight * i / 3;
            painter.drawLine(chartRect.left(), y, chartRect.right(), y);
        }

        painter.setPen(QColor(QStringLiteral("#B4B8CC")));
        painter.setFont(QFont(painter.font().family(), 10));
        painter.drawText(chartRect.left(), chartRect.top() - 6,
                         QStringLiteral("最近 7 天 · 单位：分钟"));

        const int count = days.size();
        const qreal slotWidth = count > 0 ? static_cast<qreal>(chartRect.width()) / count : 0;
        const qreal barWidth = std::min<qreal>(42, slotWidth * 0.44);

        for (int i = 0; i < count; ++i) {
            const DayData& day = days.at(i);
            const qreal centerX = chartRect.left() + slotWidth * i + slotWidth / 2;
            const qreal ratio = static_cast<qreal>(day.minutes) / safeMaxMinutes;
            const qreal barHeight = day.seconds > 0
                ? std::max<qreal>(8, ratio * (chartHeight - 18))
                : 0;

            QRectF barRect(centerX - barWidth / 2,
                           baselineY - barHeight,
                           barWidth,
                           barHeight);

            QPainterPath path;
            path.addRoundedRect(barRect, 6, 6);

            const bool isToday = day.date == today;
            painter.fillPath(path, isToday
                ? QColor(QStringLiteral("#6A5AE0"))
                : QColor(QStringLiteral("#A9A2F3")));

            painter.setPen(QColor(QStringLiteral("#6A5AE0")));
            const QString valueText = day.seconds > 0 && day.minutes < 1
                ? QStringLiteral("<1")
                : QString::number(day.minutes);
            painter.drawText(QRectF(centerX - slotWidth / 2, barRect.top() - 22,
                                    slotWidth, 18),
                             Qt::AlignCenter,
                             valueText);

            painter.setPen(QColor(QStringLiteral("#8A8FA3")));
            const QString dayText = isToday
                ? QStringLiteral("今天")
                : day.date.toString(QStringLiteral("MM/dd"));
            painter.drawText(QRectF(centerX - slotWidth / 2, baselineY + 10,
                                    slotWidth, 20),
                             Qt::AlignCenter,
                             dayText);
        }

        const bool hasStudyData = std::any_of(days.cbegin(), days.cend(), [](const DayData& day) {
            return day.seconds > 0;
        });

        if (!hasStudyData) {
            painter.setPen(QColor(QStringLiteral("#B4B8CC")));
            painter.drawText(chartRect, Qt::AlignCenter,
                             QStringLiteral("暂无学习时长数据，进入学习中心后会开始累计。"));
        }
    }
};

class CheckinCalendarWidget : public QWidget {
public:
    explicit CheckinCalendarWidget(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(320);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), Qt::transparent);

        const QDate today = QDate::currentDate();
        const QDate firstDay(today.year(), today.month(), 1);
        const int daysInMonth = firstDay.daysInMonth();
        const int firstColumn = firstDay.dayOfWeek() - 1;

        painter.setPen(QColor(QStringLiteral("#24253D")));
        painter.setFont(QFont(painter.font().family(), 16, QFont::Bold));
        painter.drawText(QRect(4, 0, width() - 8, 26),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         today.toString(QStringLiteral("yyyy 年 M 月")));

        painter.setPen(QColor(QStringLiteral("#6A5AE0")));
        painter.setFont(QFont(painter.font().family(), 11));
        painter.drawText(QRect(4, 30, width() - 8, 22),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("学习超过五分钟自动打卡"));

        const QStringList weekDays = {
            QStringLiteral("一"), QStringLiteral("二"), QStringLiteral("三"),
            QStringLiteral("四"), QStringLiteral("五"), QStringLiteral("六"),
            QStringLiteral("日")
        };

        const QRect gridRect = rect().adjusted(0, 66, 0, -4);
        const qreal cellWidth = gridRect.width() / 7.0;
        const qreal headerHeight = 24;
        const qreal cellHeight = (gridRect.height() - headerHeight) / 6.0;

        painter.setFont(QFont(painter.font().family(), 10, QFont::DemiBold));
        painter.setPen(QColor(QStringLiteral("#8A8FA3")));
        for (int col = 0; col < 7; ++col) {
            painter.drawText(QRectF(gridRect.left() + col * cellWidth,
                                    gridRect.top(),
                                    cellWidth,
                                    headerHeight),
                             Qt::AlignCenter,
                             weekDays.at(col));
        }

        painter.setFont(QFont(painter.font().family(), 11, QFont::DemiBold));
        for (int day = 1; day <= daysInMonth; ++day) {
            const int index = firstColumn + day - 1;
            const int row = index / 7;
            const int col = index % 7;
            const QDate date(today.year(), today.month(), day);

            const QRectF cellRect(gridRect.left() + col * cellWidth + 4,
                                  gridRect.top() + headerHeight + row * cellHeight + 4,
                                  cellWidth - 8,
                                  cellHeight - 8);

            const bool checkedIn = isCheckedInDate(date);
            const bool isToday = date == today;

            QPainterPath cellPath;
            cellPath.addRoundedRect(cellRect, 8, 8);

            if (checkedIn) {
                painter.fillPath(cellPath, QColor(QStringLiteral("#EEF8F1")));
            } else if (isToday) {
                painter.fillPath(cellPath, QColor(QStringLiteral("#F5F3FF")));
            } else {
                painter.fillPath(cellPath, QColor(QStringLiteral("#F8F9FC")));
            }

            if (isToday) {
                QPen borderPen(QColor(QStringLiteral("#6A5AE0")));
                borderPen.setWidth(2);
                painter.setPen(borderPen);
                painter.drawPath(cellPath);
            }

            painter.setPen(checkedIn
                ? QColor(QStringLiteral("#1D8F4B"))
                : QColor(QStringLiteral("#3A3B52")));
            painter.drawText(cellRect.adjusted(0, 3, 0, 0),
                             Qt::AlignHCenter | Qt::AlignTop,
                             QString::number(day));

            if (checkedIn) {
                painter.setFont(QFont(painter.font().family(), 8, QFont::DemiBold));
                painter.drawText(QRectF(cellRect.left(),
                                        cellRect.top() + 17,
                                        cellRect.width(),
                                        13),
                                 Qt::AlignHCenter | Qt::AlignTop,
                                 QStringLiteral("已打卡"));
                painter.setFont(QFont(painter.font().family(), 11, QFont::DemiBold));
            }
        }
    }
};

class ModuleDistributionChart : public QWidget {
public:
    explicit ModuleDistributionChart(QWidget* parent = nullptr) : QWidget(parent)
    {
        setMinimumHeight(190);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        const QDate today = QDate::currentDate();
        const QVector<ModuleStat> stats = moduleStatsForDate(today);
        int totalSeconds = 0;
        for (const ModuleStat& stat : stats) {
            totalSeconds += stat.seconds;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), Qt::transparent);

        painter.setPen(QColor(QStringLiteral("#24253D")));
        painter.setFont(QFont(painter.font().family(), 14, QFont::Bold));
        painter.drawText(QRect(0, 0, width(), 24),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("今日模块分布"));

        // painter.setFont(QFont(painter.font().family(), 10));
        // painter.setPen(QColor(QStringLiteral("#8A8FA3")));
        // painter.drawText(QRect(0, 26, width(), 20),
        //                  Qt::AlignLeft | Qt::AlignVCenter,
        //                  QStringLiteral("按当前页面自动累计学习时间"));

        const QRectF pieRect(4, 58, 116, 116);
        if (totalSeconds <= 0) {
            painter.setPen(QColor(QStringLiteral("#B4B8CC")));
            painter.setBrush(QColor(QStringLiteral("#F4F5FA")));
            painter.drawEllipse(pieRect);
            painter.drawText(pieRect, Qt::AlignCenter, QStringLiteral("暂无"));
            return;
        }

        int startAngle = 90 * 16;
        for (const ModuleStat& stat : stats) {
            if (stat.seconds <= 0) {
                continue;
            }
            const int spanAngle = -qRound(360.0 * 16.0 * stat.seconds / totalSeconds);
            painter.setPen(Qt::NoPen);
            painter.setBrush(stat.color);
            painter.drawPie(pieRect, startAngle, spanAngle);
            startAngle += spanAngle;
        }

        painter.setPen(QColor(QStringLiteral("#FFFFFF")));
        painter.setBrush(QColor(QStringLiteral("#FFFFFF")));
        painter.drawEllipse(pieRect.adjusted(32, 32, -32, -32));

        painter.setPen(QColor(QStringLiteral("#24253D")));
        painter.setFont(QFont(painter.font().family(), 11, QFont::Bold));
        painter.drawText(pieRect.adjusted(26, 30, -26, -30),
                         Qt::AlignCenter,
                         formatMinutes(totalSeconds));

        int legendY = 58;
        const int legendX = 144;
        painter.setFont(QFont(painter.font().family(), 10));
        for (const ModuleStat& stat : stats) {
            if (stat.seconds <= 0) {
                continue;
            }

            const int percent = qRound(100.0 * stat.seconds / totalSeconds);
            painter.setPen(Qt::NoPen);
            painter.setBrush(stat.color);
            painter.drawRoundedRect(QRectF(legendX, legendY + 4, 10, 10), 3, 3);

            painter.setPen(QColor(QStringLiteral("#3A3B52")));
            painter.drawText(QRect(legendX + 18, legendY, width() - legendX - 18, 18),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             QStringLiteral("%1 · %2 · %3%")
                                 .arg(stat.label, formatMinutes(stat.seconds))
                                 .arg(percent));
            legendY += 22;
        }
    }
};

} // namespace

static QPushButton* makeBackBtn(QWidget* parent = nullptr) {
    auto* btn = new QPushButton(QStringLiteral("← 返回"), parent);
    btn->setObjectName(QStringLiteral("LearnBackBtn"));
    return btn;
}

static QFrame* makeTrendPanel(QWidget* parent = nullptr)
{
    auto* panel = new QFrame(parent);
    panel->setObjectName(QStringLiteral("Card"));
    panel->setMinimumHeight(260);

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);

    auto* titleLabel = new QLabel(QStringLiteral("学习时长趋势"), panel);
    titleLabel->setStyleSheet(QStringLiteral(
        "font-size:16px; font-weight:700; color:#24253D; background:transparent;"));

    layout->addWidget(titleLabel);
    layout->addWidget(new StudyTrendChart(panel), 1);

    return panel;
}

static QFrame* makeCalendarPanel(QWidget* parent = nullptr)
{
    auto* panel = new QFrame(parent);
    panel->setObjectName(QStringLiteral("Card"));
    panel->setMinimumHeight(390);

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);
    layout->addWidget(new CheckinCalendarWidget(panel), 1);

    return panel;
}

static void populateStudyRecords(QListWidget* records)
{
    records->clear();

    const QDate today = QDate::currentDate();
    for (int i = 0; i < 30; ++i) {
        const QDate date = today.addDays(-i);
        if (!isCheckedInDate(date)) {
            continue;
        }

        auto* item = new QListWidgetItem(
            QStringLiteral("%1  已自动打卡 · %2\n%3")
                .arg(date == today ? QStringLiteral("今天") : date.toString(QStringLiteral("MM/dd")))
                .arg(formatMinutes(studySecondsForDate(date)))
                .arg(moduleSummaryForDate(date)));
        item->setSizeHint(QSize(0, 48));
        records->addItem(item);
    }

    if (records->count() == 0) {
        records->addItem(QStringLiteral("暂无学习记录"));
    }
}

static QFrame* makeRecordPanel(QListWidget** recordsOut, QWidget** chartOut,
                               QWidget* parent = nullptr)
{
    auto* recordPanel = new QFrame(parent);
    recordPanel->setObjectName(QStringLiteral("Card"));
    recordPanel->setMinimumHeight(180);
    auto* recordLayout = new QVBoxLayout(recordPanel);
    recordLayout->setContentsMargins(20, 18, 20, 18);
    recordLayout->setSpacing(10);

    auto* recordTitle = new QLabel(QStringLiteral("学习记录"), recordPanel);
    recordTitle->setStyleSheet(QStringLiteral(
        "font-size:16px; font-weight:700; color:#24253D; background:transparent;"));

    auto* moduleChart = new ModuleDistributionChart(recordPanel);

    auto* records = new QListWidget(recordPanel);
    records->setFrameShape(QFrame::NoFrame);
    records->setStyleSheet(QStringLiteral(
        "QListWidget { background:transparent; color:#8A8FA3; font-size:13px; }"
        "QListWidget::item { padding:6px 0; }"));
    populateStudyRecords(records);
    if (recordsOut) {
        *recordsOut = records;
    }
    if (chartOut) {
        *chartOut = moduleChart;
    }

    recordLayout->addWidget(recordTitle);
    recordLayout->addWidget(moduleChart);
    recordLayout->addWidget(records, 1);

    return recordPanel;
}

LearningCenterPage::LearningCenterPage(QWidget* parent) : QWidget(parent)
{
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(24, 16, 24, 24);
    lay->setSpacing(20);

    auto* top = new QHBoxLayout;
    auto* back = makeBackBtn(this);
    connect(back, &QPushButton::clicked, this, &LearningCenterPage::backRequested);
    auto* t = new QLabel(QStringLiteral("学习管理中心"));
    t->setObjectName(QStringLiteral("PageTitle"));
    top->addWidget(back);
    top->addWidget(t);
    top->addStretch();

    auto* trendPanel = makeTrendPanel(this);

    auto* bottomRow = new QHBoxLayout;
    bottomRow->setSpacing(16);

    auto* calendarPanel = makeCalendarPanel(this);
    auto* recordPanel = makeRecordPanel(&m_records, &m_moduleChart, this);

    bottomRow->addWidget(calendarPanel, 1);
    bottomRow->addWidget(recordPanel, 1);

    lay->addLayout(top);
    lay->addWidget(trendPanel, 1);
    lay->addLayout(bottomRow);
}

void LearningCenterPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    refreshData();
}

void LearningCenterPage::refreshData()
{
    refreshRecords();
    if (m_moduleChart) {
        m_moduleChart->update();
    }
    update();
}

void LearningCenterPage::refreshRecords()
{
    if (m_records) {
        populateStudyRecords(m_records);
    }
}

} // namespace AlgeMate::Learning
