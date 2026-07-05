#include "LearningCenterPage.h"
#include "core/ThemeManager.h"
#include "ui/TomatoManager.h"
#include "ui/TomatoClockDialog.h"

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
#include <QDialog>
#include <QScrollArea>
#include <QGridLayout>
#include <QToolButton>
#include <QComboBox>
#include <QMouseEvent>
#include <QDateEdit>
#include <QDialogButtonBox>

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
        {QStringLiteral("knowledge"), QStringLiteral("知识点学习"), QColor(QStringLiteral("#5C9CE6")), 0},
        {QStringLiteral("practice"), QStringLiteral("练习模式"), QColor(QStringLiteral("#F28E63")), 0},
        {QStringLiteral("exam"), QStringLiteral("考试模式"), QColor(QStringLiteral("#E8618C")), 0},
        {QStringLiteral("wrongbook"), QStringLiteral("错题本"), QColor(QStringLiteral("#4CB3A4")), 0},
        {QStringLiteral("management"), QStringLiteral("学习管理"), QColor(QStringLiteral("#9B72E6")), 0},
        {QStringLiteral("other"), QStringLiteral("其他"), QColor(QStringLiteral("#FFD700")), 0}
    };
}

static QString dateKey(const QDate& date)
{
    return date.toString(Qt::ISODate);
}

static int studySecondsForDate(const QDate& date)
{
    QSettings settings;
    // QSettings专门用来保存和读取应用的配置信息
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
        //settings里有键值对，值是时间，键（mac有些许不同，但也能这么查）
        knownSeconds += stat.seconds;
    }

    settings.endGroup();
    settings.endGroup();

    const int totalSeconds = studySecondsForDate(date);
    if (totalSeconds > knownSeconds && !stats.isEmpty()) {
        stats[6].seconds += totalSeconds - knownSeconds;
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

static QVector<ModuleStat> moduleStatsForDateRange(const QDate& startDate, const QDate& endDate) {
    QVector<ModuleStat> stats = moduleDefinitions();
    QSettings settings;

    for (QDate d = startDate; d <= endDate; d = d.addDays(1)) {
        QString dKey = dateKey(d);

        settings.beginGroup(QStringLiteral("learning/studySecondsByDate"));
        int totalDaySeconds = settings.value(dKey, 0).toInt();
        settings.endGroup();

        settings.beginGroup(QStringLiteral("learning/moduleSecondsByDate"));
        settings.beginGroup(dKey);

        int knownDaySeconds = 0;
        for (ModuleStat& stat : stats) {
            int s = settings.value(stat.key, 0).toInt();
            stat.seconds += s;
            knownDaySeconds += s;
        }

        settings.endGroup();
        settings.endGroup();

        if (totalDaySeconds > knownDaySeconds && !stats.isEmpty()) {
            stats[6].seconds += totalDaySeconds - knownDaySeconds;
        }
    }
    return stats;
}

class TrendBarChartWidget : public QWidget {
public:
    enum Mode { Daily, Weekly, Monthly };

    explicit TrendBarChartWidget(QWidget* parent = nullptr) : QWidget(parent), m_mode(Daily) {}

    void setData(Mode mode, const QVector<int>& dataInSeconds) {
        m_mode = mode;
        m_data = dataInSeconds;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), Qt::transparent);

        if (m_data.isEmpty()) return;

        int maxSec = 0;
        for (int sec : m_data) maxSec = qMax(maxSec, sec);
        int maxMin = maxSec / 60;
        int gridMax = qMax(10, ((maxMin / 10) + 1) * 10);

        const int padLeft = 40;
        const int padRight = 20;
        const int padTop = 30;
        const int padBottom = 30;
        const int chartW = width() - padLeft - padRight;
        const int chartH = height() - padTop - padBottom;

        painter.setFont(QFont(painter.font().family(), 10));

        for (int i = 0; i <= 2; ++i) {
            int y = padTop + chartH - (chartH * i / 2);

            painter.setPen(QPen(QColor(isDark ? "#3B395A" : "#E2E8F0"), 1, Qt::DashLine));
            painter.drawLine(padLeft, y, width() - padRight, y);

            painter.setPen(QColor(QStringLiteral("#8A8FA3")));
            int val = gridMax * i / 2;
            painter.drawText(QRect(0, y - 10, padLeft - 10, 20), Qt::AlignRight | Qt::AlignVCenter, QString::number(val));
        }

        painter.drawText(QRect(0, 0, padLeft + 20, 20), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("单位: 分钟"));

        const int count = m_data.size();
        const qreal slotW = static_cast<qreal>(chartW) / count;
        const qreal barW = qMin(30.0, slotW * 0.6);

        for (int i = 0; i < count; ++i) {
            int sec = m_data[i];
            qreal min = sec / 60.0;
            qreal h = (min / gridMax) * chartH;
            if (sec > 0 && h < 4) h = 4;

            qreal cx = padLeft + slotW * i + slotW / 2;
            QRectF barRect(cx - barW / 2, padTop + chartH - h, barW, h);

            QPainterPath path;
            path.addRoundedRect(barRect, 4, 4);
            painter.fillPath(path, QColor(isDark ? "#8FA1FF" : "#6A5AE0"));

            if (sec > 0 && count <= 7) {
                painter.setPen(QColor(isDark ? "#8FA1FF" : "#6A5AE0"));
                painter.drawText(QRectF(cx - slotW / 2, barRect.top() - 20, slotW, 20), Qt::AlignCenter, QString::number(qRound(min)));
            }

            painter.setPen(QColor(QStringLiteral("#8A8FA3")));
            bool drawLabel = false;
            QString labelText;

            if (m_mode == Daily) {
                if (i % 4 == 0 || i == 23) {
                    drawLabel = true;
                    labelText = QStringLiteral("%1:00").arg(i, 2, 10, QLatin1Char('0'));
                }
            } else if (m_mode == Weekly) {
                drawLabel = true;
                const QStringList days = {QStringLiteral("一"), QStringLiteral("二"), QStringLiteral("三"), QStringLiteral("四"), QStringLiteral("五"), QStringLiteral("六"), QStringLiteral("日")};
                labelText = days.value(i);
            } else if (m_mode == Monthly) {
                if (i == 0 || i == 4 || i == 9 || i == 14 || i == 19 || i == 24 || i == count - 1) {
                    drawLabel = true;
                    labelText = QString::number(i + 1);
                }
            }

            if (drawLabel) {
                painter.drawText(QRectF(cx - slotW, padTop + chartH + 10, slotW * 2, 20), Qt::AlignCenter, labelText);
            }
        }
    }

private:
    Mode m_mode;
    QVector<int> m_data;
};

class StudyTrendDialog : public QDialog {
public:
    explicit StudyTrendDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("学习趋势分析"));
        setFixedSize(700, 480);

        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        setStyleSheet(isDark ? QStringLiteral("background-color: #1C1B2E;") : QStringLiteral("background-color: #FFFFFF;"));

        m_currentDate = QDate::currentDate();
        m_mode = TrendBarChartWidget::Daily;

        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(20);
        layout->setContentsMargins(30, 30, 30, 30);

        auto* topLayout = new QHBoxLayout();

        m_modeCombo = new QComboBox(this);
        m_modeCombo->addItems({QStringLiteral("每日 (按小时)"), QStringLiteral("每周 (按日)"), QStringLiteral("每月 (按日)")});
        m_modeCombo->setStyleSheet(isDark ?
                                       "QComboBox { border: 1px solid #3B395A; border-radius: 6px; padding: 4px 12px; font-size: 14px; background: #28263F; color: #E6E7F0; }"
                                       "QComboBox::drop-down { border: none; }"
                                          :
                                       "QComboBox { border: 1px solid #E2E8F0; border-radius: 6px; padding: 4px 12px; font-size: 14px; background: #F8F9FC; color: #24253D; }"
                                       "QComboBox::drop-down { border: none; }"
                                   );
        m_modeCombo->setCursor(Qt::PointingHandCursor);

        auto* prevBtn = new QToolButton(this);
        prevBtn->setText(QStringLiteral("<"));
        prevBtn->setStyleSheet(isDark ? "color: #8FA1FF; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;" : "color: #6A5AE0; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;");
        prevBtn->setCursor(Qt::PointingHandCursor);

        m_dateBtn = new QPushButton(this);
        m_dateBtn->setFont(QFont(m_dateBtn->font().family(), 16, QFont::Bold));
        m_dateBtn->setStyleSheet(isDark ?
                                     "QPushButton { color: #E6E7F0; border: none; background: transparent; padding: 4px; } QPushButton:hover { color: #8FA1FF; }"
                                        :
                                     "QPushButton { color: #24253D; border: none; background: transparent; padding: 4px; } QPushButton:hover { color: #6A5AE0; }"
                                 );
        m_dateBtn->setCursor(Qt::PointingHandCursor);
        m_dateBtn->setToolTip(QStringLiteral("点击输入跳转至指定日期"));
        m_dateBtn->setMinimumWidth(240);

        auto* nextBtn = new QToolButton(this);
        nextBtn->setText(QStringLiteral(">"));
        nextBtn->setStyleSheet(isDark ? "color: #8FA1FF; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;" : "color: #6A5AE0; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;");
        nextBtn->setCursor(Qt::PointingHandCursor);

        topLayout->addWidget(prevBtn);
        topLayout->addWidget(m_dateBtn);
        topLayout->addWidget(nextBtn);
        topLayout->addStretch();
        topLayout->addWidget(m_modeCombo);

        layout->addLayout(topLayout);

        m_chartWidget = new TrendBarChartWidget(this);
        layout->addWidget(m_chartWidget, 1);

        connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            m_mode = static_cast<TrendBarChartWidget::Mode>(index);
            updateData();
        });

        connect(prevBtn, &QToolButton::clicked, this, [this]() { shiftDate(-1); });
        connect(nextBtn, &QToolButton::clicked, this, [this]() { shiftDate(1); });

        connect(m_dateBtn, &QPushButton::clicked, this, [this, isDark]() {
            QDialog dialog(this);
            dialog.setWindowTitle(QStringLiteral("跳转至指定日期"));
            dialog.setFixedSize(300, 160);
            dialog.setStyleSheet(isDark ? QStringLiteral("background-color: #1C1B2E;") : QStringLiteral("background-color: #FFFFFF;"));

            auto* dLayout = new QVBoxLayout(&dialog);
            dLayout->setContentsMargins(20, 20, 20, 20);
            dLayout->setSpacing(12);

            auto* label = new QLabel(QStringLiteral("您可以直接输入日期或展开日历："), &dialog);
            label->setStyleSheet(QStringLiteral("color: #8A8FA3; font-size: 13px;"));

            auto* dateEdit = new QDateEdit(&dialog);
            dateEdit->setDate(m_currentDate);
            dateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
            dateEdit->setCalendarPopup(true);
            dateEdit->setStyleSheet(isDark ?
                                        "QDateEdit { border: 1px solid #3B395A; border-radius: 6px; padding: 8px 10px; font-size: 15px; color: #E6E7F0; background: #28263F; }"
                                        "QDateEdit::drop-down { border: none; width: 30px; }"
                                        "QDateEdit QAbstractItemView { selection-background-color: #3B395A; selection-color: #8FA1FF; background: #1C1B2E; color: #E6E7F0; }"
                                           :
                                        "QDateEdit { border: 1px solid #E2E8F0; border-radius: 6px; padding: 8px 10px; font-size: 15px; color: #24253D; background: #F8F9FC; }"
                                        "QDateEdit::drop-down { border: none; width: 30px; }"
                                        "QDateEdit QAbstractItemView { selection-background-color: #6A5AE0; selection-color: white; background: #FFFFFF; color: #24253D; }"
                                    );

            auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
            btnBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
            btnBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));

            dialog.setStyleSheet(dialog.styleSheet() + (isDark ?
                                                            "QPushButton { padding: 6px 16px; border-radius: 4px; border: none; background: #28263F; color: #E6E7F0; font-weight: bold; }"
                                                            "QPushButton:hover { background: #3B395A; }"
                                                               :
                                                            "QPushButton { padding: 6px 16px; border-radius: 4px; border: none; background: #F4F5FA; color: #24253D; font-weight: bold; }"
                                                            "QPushButton:hover { background: #E2E8F0; }"
                                                        ));
            btnBox->button(QDialogButtonBox::Ok)->setStyleSheet(isDark ? "background: #312F4A; color: #8FA1FF;" : "background: #6A5AE0; color: white;");
            btnBox->button(QDialogButtonBox::Ok)->setCursor(Qt::PointingHandCursor);
            btnBox->button(QDialogButtonBox::Cancel)->setCursor(Qt::PointingHandCursor);

            dLayout->addWidget(label);
            dLayout->addWidget(dateEdit);
            dLayout->addStretch();
            dLayout->addWidget(btnBox);

            connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

            dateEdit->setFocus();
            dateEdit->selectAll();

            if (dialog.exec() == QDialog::Accepted) {
                m_currentDate = dateEdit->date();
                updateData();
            }
        });

        updateData();
    }

private:
    void shiftDate(int direction) {
        switch (m_mode) {
        case TrendBarChartWidget::Daily: m_currentDate = m_currentDate.addDays(direction); break;
        case TrendBarChartWidget::Weekly: m_currentDate = m_currentDate.addDays(direction * 7); break;
        case TrendBarChartWidget::Monthly: m_currentDate = m_currentDate.addMonths(direction); break;
        }
        updateData();
    }

    void updateData() {
        QSettings settings;
        QVector<int> chartData;
        QString labelText;

        if (m_mode == TrendBarChartWidget::Daily) {
            labelText = m_currentDate.toString(QStringLiteral("yyyy 年 MM 月 dd 日"));
            chartData.resize(24);
            chartData.fill(0);

            settings.beginGroup(QStringLiteral("learning/studySecondsByHour/") + dateKey(m_currentDate));
            int totalHourly = 0;
            for (int i = 0; i < 24; ++i) {
                int sec = settings.value(QString::number(i), 0).toInt();
                chartData[i] = sec;
                totalHourly += sec;
            }
            settings.endGroup();

            int dailyTotal = settings.value(QStringLiteral("learning/studySecondsByDate/") + dateKey(m_currentDate), 0).toInt();
            if (totalHourly == 0 && dailyTotal > 0) {
                chartData[19] = dailyTotal * 0.1;
                chartData[20] = dailyTotal * 0.4;
                chartData[21] = dailyTotal * 0.3;
                chartData[22] = dailyTotal * 0.2;
            }

        } else if (m_mode == TrendBarChartWidget::Weekly) {
            QDate start = m_currentDate.addDays(-(m_currentDate.dayOfWeek() - 1));
            QDate end = start.addDays(6);
            labelText = QStringLiteral("%1 ~ %2").arg(start.toString("MM.dd"), end.toString("MM.dd"));

            chartData.resize(7);
            for (int i = 0; i < 7; ++i) {
                chartData[i] = settings.value(QStringLiteral("learning/studySecondsByDate/") + dateKey(start.addDays(i)), 0).toInt();
            }

        } else if (m_mode == TrendBarChartWidget::Monthly) {
            labelText = m_currentDate.toString(QStringLiteral("yyyy 年 MM 月"));
            int days = m_currentDate.daysInMonth();
            QDate start(m_currentDate.year(), m_currentDate.month(), 1);

            chartData.resize(days);
            for (int i = 0; i < days; ++i) {
                chartData[i] = settings.value(QStringLiteral("learning/studySecondsByDate/") + dateKey(start.addDays(i)), 0).toInt();
            }
        }

        m_dateBtn->setText(labelText + QStringLiteral(" ▾"));
        m_chartWidget->setData(m_mode, chartData);
    }

    TrendBarChartWidget::Mode m_mode;
    QDate m_currentDate;
    QPushButton* m_dateBtn;
    QComboBox* m_modeCombo;
    TrendBarChartWidget* m_chartWidget;
};

class StudyTrendChart : public QWidget {
public:
    explicit StudyTrendChart(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(240);
        setMouseTracking(true);
    }

protected:
    QRect getClickableRect() const {
        QFont font(this->font().family(), 11, QFont::Bold);
        QFontMetrics fm(font);
        QString text = QStringLiteral("详细趋势 >");
        int textWidth = fm.boundingRect(text).width();
        return QRect(width() - textWidth - 10, 0, textWidth + 10, 24);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (getClickableRect().contains(event->pos())) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        QWidget::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && getClickableRect().contains(event->pos())) {
            StudyTrendDialog dialog(this);
            dialog.exec();
        }
        QWidget::mouseReleaseEvent(event);
    }

    void paintEvent(QPaintEvent*) override {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), Qt::transparent);

        painter.setPen(QColor(QStringLiteral("#8A8FA3")));
        painter.setFont(QFont(painter.font().family(), 11));
        painter.drawText(QRect(0, 0, width(), 24), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("最近 7 天 · 单位：分钟"));

        painter.setPen(QColor(isDark ? "#8FA1FF" : "#6A5AE0"));
        painter.setFont(QFont(painter.font().family(), 11, QFont::Bold));
        painter.drawText(QRect(0, 0, width(), 24), Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("详细趋势 >"));

        const QDate today = QDate::currentDate();
        struct DayData { QDate date; int seconds; int minutes; };
        QVector<DayData> days;
        QSettings settings;
        int maxMinutes = 0;
        for (int i = 6; i >= 0; --i) {
            QDate d = today.addDays(-i);
            int secs = settings.value(QStringLiteral("learning/studySecondsByDate/") + dateKey(d), 0).toInt();
            int mins = secs / 60;
            days.append({d, secs, mins});
            if (mins > maxMinutes) maxMinutes = mins;
        }
        const int safeMaxMinutes = qMax(10, maxMinutes + 5);
        const int count = days.size();

        const QRectF chartRect(0, 45, width(), height() - 80);
        const qreal chartHeight = chartRect.height();
        const qreal baselineY = chartRect.bottom();

        const qreal slotWidth = count > 0 ? static_cast<qreal>(chartRect.width()) / count : 0;
        const qreal barWidth = std::min<qreal>(42.0, slotWidth * 0.44);

        painter.setPen(QPen(QColor(isDark ? "#28263F" : "#F4F5FA"), 1));
        painter.drawLine(QPointF(chartRect.left(), baselineY - chartHeight / 2),
                         QPointF(chartRect.right(), baselineY - chartHeight / 2));
        painter.drawLine(QPointF(chartRect.left(), baselineY - chartHeight),
                         QPointF(chartRect.right(), baselineY - chartHeight));

        painter.setPen(QColor(isDark ? "#3B395A" : "#E2E8F0"));
        painter.drawLine(QPointF(chartRect.left(), baselineY), QPointF(chartRect.right(), baselineY));

        painter.setFont(QFont(painter.font().family(), 12, QFont::DemiBold));

        for (int i = 0; i < count; ++i) {
            const DayData& day = days.at(i);
            const qreal centerX = chartRect.left() + slotWidth * i + slotWidth / 2;
            const qreal ratio = static_cast<qreal>(day.minutes) / safeMaxMinutes;
            const qreal barHeight = day.seconds > 0 ? std::max<qreal>(8.0, ratio * (chartHeight - 18)) : 0;

            QRectF barRect(centerX - barWidth / 2, baselineY - barHeight, barWidth, barHeight);

            QPainterPath path;
            path.addRoundedRect(barRect, 6, 6);

            const bool isToday = day.date == today;

            painter.fillPath(path, isToday
                                       ? QColor(isDark ? "#8FA1FF" : "#6A5AE0")
                                       : QColor(isDark ? "#312F4A" : "#D4D0F5"));

            if (day.seconds == 0) {
                painter.setPen(QColor(isDark ? "#4B4970" : "#C8CBE0"));
            } else {
                painter.setPen(isToday ? QColor(isDark ? "#8FA1FF" : "#6A5AE0") : QColor(isDark ? "#6F77FF" : "#5A48D6"));
            }

            const QString valueText = day.seconds > 0 && day.minutes < 1 ? QStringLiteral("<1") : QString::number(day.minutes);
            painter.drawText(QRectF(centerX - slotWidth / 2, barRect.top() - 22, slotWidth, 18), Qt::AlignCenter, valueText);

            painter.setPen(isToday ? QColor(isDark ? "#8FA1FF" : "#6A5AE0") : QColor("#8A8FA3"));
            const QString dayText = isToday ? QStringLiteral("今天") : day.date.toString(QStringLiteral("MM/dd"));
            painter.drawText(QRectF(centerX - slotWidth / 2, baselineY + 10, slotWidth, 20), Qt::AlignCenter, dayText);
        }
    }
};

class CalendarGridWidget : public QWidget {
public:
    explicit CalendarGridWidget(QWidget* parent = nullptr) : QWidget(parent) { m_displayDate = QDate::currentDate(); }
    void setMonth(int year, int month) { m_displayDate = QDate(year, month, 1); update(); }
protected:
    void paintEvent(QPaintEvent*) override {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), Qt::transparent);

        const QDate today = QDate::currentDate();
        const QDate firstDay(m_displayDate.year(), m_displayDate.month(), 1);
        const int daysInMonth = firstDay.daysInMonth();
        const int firstColumn = firstDay.dayOfWeek() - 1;

        const QStringList weekDays = {QStringLiteral("一"), QStringLiteral("二"), QStringLiteral("三"), QStringLiteral("四"), QStringLiteral("五"), QStringLiteral("六"), QStringLiteral("日")};
        const QRectF gridRect = rect();
        const qreal cellWidth = gridRect.width() / 7.0;
        const qreal headerHeight = 24;
        const qreal cellHeight = (gridRect.height() - headerHeight) / 6.0;

        painter.setFont(QFont(painter.font().family(), 10, QFont::DemiBold));
        painter.setPen(QColor(QStringLiteral("#8A8FA3")));
        for (int col = 0; col < 7; ++col) {
            painter.drawText(QRectF(gridRect.left() + col * cellWidth, gridRect.top(), cellWidth, headerHeight), Qt::AlignCenter, weekDays.at(col));
        }

        painter.setFont(QFont(painter.font().family(), 11, QFont::DemiBold));
        for (int day = 1; day <= daysInMonth; ++day) {
            const int index = firstColumn + day - 1;
            const int row = index / 7;
            const int col = index % 7;
            const QDate date(m_displayDate.year(), m_displayDate.month(), day);
            const QRectF cellRect(gridRect.left() + col * cellWidth + 2, gridRect.top() + headerHeight + row * cellHeight + 2, cellWidth - 4, cellHeight - 4);
            const bool checkedIn = isCheckedInDate(date);
            const bool isToday = date == today;

            QPainterPath cellPath;
            cellPath.addRoundedRect(cellRect, 8, 8);

            if (isToday && checkedIn) {
                painter.fillPath(cellPath, QColor(isDark ? "#8FA1FF" : "#6A5AE0"));
            } else if (isToday && !checkedIn) {
                painter.fillPath(cellPath, QColor(isDark ? "#28263F" : "#F5F3FF"));
                QPen borderPen(QColor(isDark ? "#8FA1FF" : "#6A5AE0"));
                borderPen.setWidth(2);
                painter.setPen(borderPen);
                painter.drawPath(cellPath);
            } else if (!isToday && checkedIn) {
                painter.fillPath(cellPath, QColor(isDark ? "#312F4A" : "#EBE7FF"));
            } else {
                painter.fillPath(cellPath, QColor(isDark ? "#1F1E33" : "#F8F9FC"));
            }

            if (isToday && checkedIn) {
                painter.setPen(QColor(isDark ? "#1C1B2E" : "#FFFFFF"));
            } else if (checkedIn) {
                painter.setPen(QColor(isDark ? "#B0BBFF" : "#5A48D6"));
            } else if (isToday) {
                painter.setPen(QColor(isDark ? "#8FA1FF" : "#6A5AE0"));
            } else {
                painter.setPen(QColor(isDark ? "#E6E7F0" : "#3A3B52"));
            }

            painter.drawText(cellRect.adjusted(0, 3, 0, 0), Qt::AlignHCenter | Qt::AlignTop, QString::number(day));

            if (checkedIn) {
                if (isToday) {
                    painter.setPen(isDark ? QColor(28, 27, 46, 240) : QColor(255, 255, 255, 240));
                } else {
                    painter.setPen(isDark ? QColor(143, 161, 255, 210) : QColor(106, 90, 224, 210));
                }

                if (cellRect.height() > 20) {
                    painter.setFont(QFont(painter.font().family(), 8, QFont::Bold));
                    painter.drawText(QRectF(cellRect.left(), cellRect.top() + cellRect.height() / 2, cellRect.width(), cellRect.height() / 2), Qt::AlignCenter, QStringLiteral("✓"));
                }
                painter.setFont(QFont(painter.font().family(), 11, QFont::DemiBold));
            }
        }
    }
private:
    QDate m_displayDate;
};

class YearlyCalendarDialog : public QDialog {
public:
    explicit YearlyCalendarDialog(int year, QWidget* parent = nullptr) : QDialog(parent), m_currentYear(year) {
        setFixedSize(920, 720);
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        setStyleSheet(isDark ? QStringLiteral("background-color: #1C1B2E;") : QStringLiteral("background-color: #FFFFFF;"));

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(16);
        auto* headerLayout = new QHBoxLayout();
        auto* prevBtn = new QToolButton(this);
        prevBtn->setText(QStringLiteral("<"));
        prevBtn->setStyleSheet(isDark ? "color: #8FA1FF; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;" : "color: #6A5AE0; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;");
        prevBtn->setCursor(Qt::PointingHandCursor);
        m_yearLabel = new QLabel(this);
        m_yearLabel->setFont(QFont(m_yearLabel->font().family(), 18, QFont::Bold));
        m_yearLabel->setStyleSheet(isDark ? QStringLiteral("color: #E6E7F0;") : QStringLiteral("color: #24253D;"));
        m_yearLabel->setAlignment(Qt::AlignCenter);
        m_yearLabel->setMinimumWidth(120);
        auto* nextBtn = new QToolButton(this);
        nextBtn->setText(QStringLiteral(">"));
        nextBtn->setStyleSheet(isDark ? "color: #8FA1FF; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;" : "color: #6A5AE0; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;");
        nextBtn->setCursor(Qt::PointingHandCursor);
        headerLayout->addStretch();
        headerLayout->addWidget(prevBtn);
        headerLayout->addWidget(m_yearLabel);
        headerLayout->addWidget(nextBtn);
        headerLayout->addStretch();
        mainLayout->addLayout(headerLayout);
        auto* scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        auto* container = new QWidget(scroll);
        auto* gridLayout = new QGridLayout(container);
        gridLayout->setSpacing(20);
        for (int i = 0; i < 12; ++i) {
            auto* monthWidget = new QWidget(container);
            auto* monthLayout = new QVBoxLayout(monthWidget);
            auto* title = new QLabel(QStringLiteral("%1 月").arg(i + 1), monthWidget);
            title->setFont(QFont(title->font().family(), 12, QFont::Bold));
            title->setStyleSheet(isDark ? QStringLiteral("color: #E6E7F0;") : QStringLiteral("color: #24253D;"));
            title->setAlignment(Qt::AlignCenter);
            auto* grid = new CalendarGridWidget(monthWidget);
            grid->setMinimumHeight(150);
            m_monthGrids[i] = grid;
            monthLayout->addWidget(title);
            monthLayout->addWidget(grid, 1);
            gridLayout->addWidget(monthWidget, i / 3, i % 3);
        }
        scroll->setWidget(container);
        mainLayout->addWidget(scroll);
        connect(prevBtn, &QToolButton::clicked, this, [this]() { m_currentYear--; updateYearView(); });
        connect(nextBtn, &QToolButton::clicked, this, [this]() { m_currentYear++; updateYearView(); });
        updateYearView();
    }
private:
    void updateYearView() {
        setWindowTitle(QStringLiteral("%1 年度打卡总览").arg(m_currentYear));
        m_yearLabel->setText(QStringLiteral("%1 年").arg(m_currentYear));
        for (int i = 0; i < 12; ++i) m_monthGrids[i]->setMonth(m_currentYear, i + 1);
    }
    int m_currentYear;
    QLabel* m_yearLabel;
    CalendarGridWidget* m_monthGrids[12];
};

class CheckinCalendarWidget : public QWidget {
public:
    explicit CheckinCalendarWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(300);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_currentDate = QDate::currentDate();
        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(10);
        auto* headerLayout = new QHBoxLayout();
        m_monthLabel = new QLabel(this);
        m_monthLabel->setFont(QFont(m_monthLabel->font().family(), 16, QFont::Bold));
        m_monthLabel->setObjectName("CalendarMonthLabel");

        auto* prevBtn = new QToolButton(this);
        prevBtn->setText(QStringLiteral("<"));
        prevBtn->setCursor(Qt::PointingHandCursor);
        prevBtn->setObjectName("CalPrevBtn");
        auto* nextBtn = new QToolButton(this);
        nextBtn->setText(QStringLiteral(">"));
        nextBtn->setCursor(Qt::PointingHandCursor);
        nextBtn->setObjectName("CalNextBtn");
        auto* yearViewBtn = new QPushButton(QStringLiteral("全年概览"), this);
        yearViewBtn->setObjectName("CalYearBtn");
        yearViewBtn->setCursor(Qt::PointingHandCursor);

        headerLayout->addWidget(prevBtn);
        headerLayout->addWidget(m_monthLabel);
        headerLayout->addWidget(nextBtn);
        headerLayout->addStretch();
        headerLayout->addWidget(yearViewBtn);

        auto* tipLabel = new QLabel(QStringLiteral("学习超过五分钟自动打卡"), this);
        tipLabel->setFont(QFont(tipLabel->font().family(), 11));
        tipLabel->setObjectName("CalTipLabel");
        m_calendarGrid = new CalendarGridWidget(this);

        mainLayout->addLayout(headerLayout);
        mainLayout->addWidget(tipLabel);
        mainLayout->addWidget(m_calendarGrid, 1);

        connect(prevBtn, &QToolButton::clicked, this, [this]() { m_currentDate = m_currentDate.addMonths(-1); updateView(); });
        connect(nextBtn, &QToolButton::clicked, this, [this]() { m_currentDate = m_currentDate.addMonths(1); updateView(); });
        connect(yearViewBtn, &QPushButton::clicked, this, [this]() { YearlyCalendarDialog dialog(m_currentDate.year(), this); dialog.exec(); });
        updateView();
    }
private:
    void updateView() {
        m_monthLabel->setText(m_currentDate.toString(QStringLiteral("yyyy 年 M 月")));
        m_calendarGrid->setMonth(m_currentDate.year(), m_currentDate.month());
    }
    QDate m_currentDate;
    QLabel* m_monthLabel;
    CalendarGridWidget* m_calendarGrid;
};

class DistributionPieWidget : public QWidget {
public:
    explicit DistributionPieWidget(QWidget* parent = nullptr) : QWidget(parent) {}
    void setStats(const QVector<ModuleStat>& stats) { m_stats = stats; update(); }
protected:
    void paintEvent(QPaintEvent*) override {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        int totalSeconds = 0;
        for (const auto& s : m_stats) totalSeconds += s.seconds;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), Qt::transparent);

        const qreal size = qMin(height() - 20, width() / 2 - 20);
        const QRectF pieRect(20, (height() - size) / 2, size, size);

        if (totalSeconds <= 0) {
            painter.setPen(QColor(isDark ? "#3B395A" : "#B4B8CC"));
            painter.setBrush(QColor(isDark ? "#28263F" : "#F4F5FA"));
            painter.drawEllipse(pieRect);
            painter.drawText(pieRect, Qt::AlignCenter, QStringLiteral("暂无数据"));
            return;
        }

        int startAngle = 90 * 16;
        for (const ModuleStat& stat : m_stats) {
            if (stat.seconds <= 0) continue;
            const int spanAngle = -qRound(360.0 * 16.0 * stat.seconds / totalSeconds);
            painter.setPen(Qt::NoPen);
            painter.setBrush(stat.color);
            painter.drawPie(pieRect, startAngle, spanAngle);
            startAngle += spanAngle;
        }

        const qreal innerSize = size * 0.55;
        const QRectF innerRect = pieRect.adjusted((size - innerSize)/2, (size - innerSize)/2, -(size - innerSize)/2, -(size - innerSize)/2);
        painter.setBrush(QColor(isDark ? "#1C1B2E" : "#FFFFFF"));
        painter.drawEllipse(innerRect);

        painter.setPen(QColor(isDark ? "#E6E7F0" : "#24253D"));
        painter.setFont(QFont(painter.font().family(), 12, QFont::Bold));
        painter.drawText(innerRect, Qt::AlignCenter, formatMinutes(totalSeconds));

        const int legendX = pieRect.right() + 40;
        int visibleCount = 0;
        for (const auto& s : m_stats) if (s.seconds > 0) visibleCount++;

        int legendY = (height() - visibleCount * 30) / 2;
        if (legendY < 0) legendY = 0;

        painter.setFont(QFont(painter.font().family(), 11));
        for (const ModuleStat& stat : m_stats) {
            if (stat.seconds <= 0) continue;

            const int percent = qRound(100.0 * stat.seconds / totalSeconds);
            painter.setPen(Qt::NoPen);
            painter.setBrush(stat.color);
            painter.drawRoundedRect(QRectF(legendX, legendY + 8, 14, 14), 4, 4);

            painter.setPen(QColor(isDark ? "#C9C9DC" : "#3A3B52"));
            painter.drawText(QRect(legendX + 26, legendY, width() - legendX - 26, 30),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             QStringLiteral("%1 · %2 · %3%").arg(stat.label, formatMinutes(stat.seconds)).arg(percent));
            legendY += 30;
        }
    }
private:
    QVector<ModuleStat> m_stats;
};

class ModuleDistributionDialog : public QDialog {
public:
    explicit ModuleDistributionDialog(QWidget* parent = nullptr) : QDialog(parent) {
        setWindowTitle(QStringLiteral("详细模块统计"));
        setFixedSize(650, 450);
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        setStyleSheet(isDark ? QStringLiteral("background-color: #1C1B2E;") : QStringLiteral("background-color: #FFFFFF;"));

        m_currentDate = QDate::currentDate();
        m_mode = Mode::Daily;

        auto* layout = new QVBoxLayout(this);
        layout->setSpacing(20);
        layout->setContentsMargins(30, 30, 30, 30);

        auto* topLayout = new QHBoxLayout();

        m_modeCombo = new QComboBox(this);
        m_modeCombo->addItems({QStringLiteral("每日"), QStringLiteral("每周"), QStringLiteral("每月"), QStringLiteral("每年")});
        m_modeCombo->setStyleSheet(isDark ?
                                       "QComboBox { border: 1px solid #3B395A; border-radius: 6px; padding: 4px 12px; font-size: 14px; background: #28263F; color: #E6E7F0; }"
                                       "QComboBox::drop-down { border: none; }"
                                          :
                                       "QComboBox { border: 1px solid #E2E8F0; border-radius: 6px; padding: 4px 12px; font-size: 14px; background: #F8F9FC; color: #24253D; }"
                                       "QComboBox::drop-down { border: none; }"
                                   );
        m_modeCombo->setCursor(Qt::PointingHandCursor);

        auto* prevBtn = new QToolButton(this);
        prevBtn->setText(QStringLiteral("<"));
        prevBtn->setStyleSheet(isDark ? "color: #8FA1FF; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;" : "color: #6A5AE0; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;");
        prevBtn->setCursor(Qt::PointingHandCursor);

        m_dateBtn = new QPushButton(this);
        m_dateBtn->setFont(QFont(m_dateBtn->font().family(), 16, QFont::Bold));
        m_dateBtn->setStyleSheet(isDark ?
                                     "QPushButton { color: #E6E7F0; border: none; background: transparent; padding: 4px; } QPushButton:hover { color: #8FA1FF; }"
                                        :
                                     "QPushButton { color: #24253D; border: none; background: transparent; padding: 4px; } QPushButton:hover { color: #6A5AE0; }"
                                 );
        m_dateBtn->setCursor(Qt::PointingHandCursor);
        m_dateBtn->setToolTip(QStringLiteral("点击输入跳转至指定日期"));
        m_dateBtn->setMinimumWidth(240);

        auto* nextBtn = new QToolButton(this);
        nextBtn->setText(QStringLiteral(">"));
        nextBtn->setStyleSheet(isDark ? "color: #8FA1FF; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;" : "color: #6A5AE0; font-weight: bold; font-size: 18px; border: none; padding: 4px 12px;");
        nextBtn->setCursor(Qt::PointingHandCursor);

        topLayout->addWidget(prevBtn);
        topLayout->addWidget(m_dateBtn);
        topLayout->addWidget(nextBtn);
        topLayout->addStretch();
        topLayout->addWidget(m_modeCombo);

        layout->addLayout(topLayout);

        m_pieWidget = new DistributionPieWidget(this);
        layout->addWidget(m_pieWidget, 1);

        connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
            m_mode = static_cast<Mode>(index);
            updateData();
        });

        connect(prevBtn, &QToolButton::clicked, this, [this]() { shiftDate(-1); });
        connect(nextBtn, &QToolButton::clicked, this, [this]() { shiftDate(1); });

        connect(m_dateBtn, &QPushButton::clicked, this, [this, isDark]() {
            QDialog dialog(this);
            dialog.setWindowTitle(QStringLiteral("跳转至指定日期"));
            dialog.setFixedSize(300, 160);
            dialog.setStyleSheet(isDark ? QStringLiteral("background-color: #1C1B2E;") : QStringLiteral("background-color: #FFFFFF;"));

            auto* dLayout = new QVBoxLayout(&dialog);
            dLayout->setContentsMargins(20, 20, 20, 20);
            dLayout->setSpacing(12);

            auto* label = new QLabel(QStringLiteral("您可以直接输入日期或展开日历："), &dialog);
            label->setStyleSheet(QStringLiteral("color: #8A8FA3; font-size: 13px;"));

            auto* dateEdit = new QDateEdit(&dialog);
            dateEdit->setDate(m_currentDate);
            dateEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
            dateEdit->setCalendarPopup(true);
            dateEdit->setStyleSheet(isDark ?
                                        "QDateEdit { border: 1px solid #3B395A; border-radius: 6px; padding: 8px 10px; font-size: 15px; color: #E6E7F0; background: #28263F; }"
                                        "QDateEdit::drop-down { border: none; width: 30px; }"
                                        "QDateEdit QAbstractItemView { selection-background-color: #3B395A; selection-color: #8FA1FF; background: #1C1B2E; color: #E6E7F0; }"
                                           :
                                        "QDateEdit { border: 1px solid #E2E8F0; border-radius: 6px; padding: 8px 10px; font-size: 15px; color: #24253D; background: #F8F9FC; }"
                                        "QDateEdit::drop-down { border: none; width: 30px; }"
                                        "QDateEdit QAbstractItemView { selection-background-color: #6A5AE0; selection-color: white; background: #FFFFFF; color: #24253D; }"
                                    );

            auto* btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
            btnBox->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
            btnBox->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));

            dialog.setStyleSheet(dialog.styleSheet() + (isDark ?
                                                            "QPushButton { padding: 6px 16px; border-radius: 4px; border: none; background: #28263F; color: #E6E7F0; font-weight: bold; }"
                                                            "QPushButton:hover { background: #3B395A; }"
                                                               :
                                                            "QPushButton { padding: 6px 16px; border-radius: 4px; border: none; background: #F4F5FA; color: #24253D; font-weight: bold; }"
                                                            "QPushButton:hover { background: #E2E8F0; }"
                                                        ));
            btnBox->button(QDialogButtonBox::Ok)->setStyleSheet(isDark ? "background: #312F4A; color: #8FA1FF;" : "background: #6A5AE0; color: white;");
            btnBox->button(QDialogButtonBox::Ok)->setCursor(Qt::PointingHandCursor);
            btnBox->button(QDialogButtonBox::Cancel)->setCursor(Qt::PointingHandCursor);

            dLayout->addWidget(label);
            dLayout->addWidget(dateEdit);
            dLayout->addStretch();
            dLayout->addWidget(btnBox);

            connect(btnBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            connect(btnBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

            dateEdit->setFocus();
            dateEdit->selectAll();

            if (dialog.exec() == QDialog::Accepted) {
                m_currentDate = dateEdit->date();
                updateData();
            }
        });

        updateData();
    }

private:
    enum class Mode { Daily, Weekly, Monthly, Yearly };

    void shiftDate(int direction) {
        switch (m_mode) {
        case Mode::Daily: m_currentDate = m_currentDate.addDays(direction); break;
        case Mode::Weekly: m_currentDate = m_currentDate.addDays(direction * 7); break;
        case Mode::Monthly: m_currentDate = m_currentDate.addMonths(direction); break;
        case Mode::Yearly: m_currentDate = m_currentDate.addYears(direction); break;
        }
        updateData();
    }

    void updateData() {
        QDate start, end;
        QString labelText;

        switch (m_mode) {
        case Mode::Daily:
            start = m_currentDate;
            end = m_currentDate;
            labelText = m_currentDate.toString(QStringLiteral("yyyy 年 MM 月 dd 日"));
            break;
        case Mode::Weekly: {
            start = m_currentDate.addDays(-(m_currentDate.dayOfWeek() - 1));
            end = start.addDays(6);
            labelText = QStringLiteral("%1 ~ %2").arg(start.toString("MM.dd"), end.toString("MM.dd"));
            break;
        }
        case Mode::Monthly: {
            start = QDate(m_currentDate.year(), m_currentDate.month(), 1);
            end = start.addDays(start.daysInMonth() - 1);
            labelText = m_currentDate.toString(QStringLiteral("yyyy 年 MM 月"));
            break;
        }
        case Mode::Yearly: {
            start = QDate(m_currentDate.year(), 1, 1);
            end = QDate(m_currentDate.year(), 12, 31);
            labelText = m_currentDate.toString(QStringLiteral("yyyy 年"));
            break;
        }
        }
        m_dateBtn->setText(labelText + QStringLiteral(" ▾"));
        m_pieWidget->setStats(moduleStatsForDateRange(start, end));
    }

    Mode m_mode;
    QDate m_currentDate;
    QPushButton* m_dateBtn;
    QComboBox* m_modeCombo;
    DistributionPieWidget* m_pieWidget;
};

class ModuleDistributionChart : public QWidget {
public:
    explicit ModuleDistributionChart(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(190);
        setCursor(Qt::PointingHandCursor);
        setToolTip(QStringLiteral("点击查看详细统计"));
    }
protected:
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            ModuleDistributionDialog dialog(this);
            dialog.exec();
        }
        QWidget::mouseReleaseEvent(event);
    }
    void paintEvent(QPaintEvent*) override {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        const QDate today = QDate::currentDate();
        const QVector<ModuleStat> stats = moduleStatsForDate(today);
        int totalSeconds = 0;
        for (const ModuleStat& stat : stats) totalSeconds += stat.seconds;

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), Qt::transparent);

        painter.setPen(QColor(isDark ? "#E6E7F0" : "#24253D"));
        painter.setFont(QFont(painter.font().family(), 14, QFont::Bold));
        painter.drawText(QRect(0, 0, width(), 24), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("今日模块分布"));

        painter.setPen(QColor(isDark ? "#8FA1FF" : "#6A5AE0"));
        painter.setFont(QFont(painter.font().family(), 11, QFont::Bold));
        painter.drawText(QRect(0, 0, width(), 24), Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("详细统计 >"));

        const QRectF pieRect(4, 58, 116, 116);
        if (totalSeconds <= 0) {
            painter.setPen(QColor(isDark ? "#3B395A" : "#B4B8CC"));
            painter.setBrush(QColor(isDark ? "#28263F" : "#F4F5FA"));
            painter.drawEllipse(pieRect);
            painter.drawText(pieRect, Qt::AlignCenter, QStringLiteral("暂无"));
            return;
        }

        int startAngle = 90 * 16;
        for (const ModuleStat& stat : stats) {
            if (stat.seconds <= 0) continue;
            const int spanAngle = -qRound(360.0 * 16.0 * stat.seconds / totalSeconds);
            painter.setPen(Qt::NoPen);
            painter.setBrush(stat.color);
            painter.drawPie(pieRect, startAngle, spanAngle);
            startAngle += spanAngle;
        }

        painter.setPen(QColor(isDark ? "#28263F" : "#FFFFFF"));
        painter.setBrush(QColor(isDark ? "#28263F" : "#FFFFFF"));
        painter.drawEllipse(pieRect.adjusted(32, 32, -32, -32));

        painter.setPen(QColor(isDark ? "#E6E7F0" : "#24253D"));
        painter.setFont(QFont(painter.font().family(), 11, QFont::Bold));
        painter.drawText(pieRect.adjusted(26, 30, -26, -30), Qt::AlignCenter, formatMinutes(totalSeconds));

        int legendY = 58;
        const int legendX = 144;
        painter.setFont(QFont(painter.font().family(), 10));
        for (const ModuleStat& stat : stats) {
            if (stat.seconds <= 0) continue;
            const int percent = qRound(100.0 * stat.seconds / totalSeconds);
            painter.setPen(Qt::NoPen);
            painter.setBrush(stat.color);
            painter.drawRoundedRect(QRectF(legendX, legendY + 4, 10, 10), 3, 3);

            painter.setPen(QColor(isDark ? "#C9C9DC" : "#3A3B52"));
            painter.drawText(QRect(legendX + 18, legendY, width() - legendX - 18, 18),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             QStringLiteral("%1 · %2 · %3%").arg(stat.label, formatMinutes(stat.seconds)).arg(percent));
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
    panel->setMinimumHeight(300);

    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(20, 18, 20, 18);
    layout->setSpacing(10);

    auto* titleLabel = new QLabel(QStringLiteral("学习时长趋势"), panel);
    titleLabel->setObjectName("TrendTitle");

    layout->addWidget(titleLabel);
    layout->addWidget(new StudyTrendChart(panel), 1);

    return panel;
}

static QFrame* makeCalendarPanel(QWidget* parent = nullptr)
{
    auto* panel = new QFrame(parent);
    panel->setObjectName(QStringLiteral("Card"));
    panel->setMinimumHeight(300);

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
    recordPanel->setMinimumHeight(300);

    auto* recordLayout = new QVBoxLayout(recordPanel);
    recordLayout->setContentsMargins(20, 18, 20, 18);
    recordLayout->setSpacing(10);

    auto* recordTitle = new QLabel(QStringLiteral("学习记录"), recordPanel);
    recordTitle->setObjectName("RecordTitle");

    auto* moduleChart = new ModuleDistributionChart(recordPanel);

    auto* records = new QListWidget(recordPanel);
    records->setFrameShape(QFrame::NoFrame);

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

    m_lblTomatoCount = new QLabel(this);
    m_lblTomatoCount->setObjectName(QStringLiteral("LearningTomatoCount"));
    m_lblTomatoCount->setAlignment(Qt::AlignCenter);

    auto& tomatoMgr = AlgeMate::TomatoManager::instance();
    m_lblTomatoCount->setText(QStringLiteral("🍅 今日已专注: %1 次").arg(tomatoMgr.completedTomatoes()));

    top->addWidget(m_lblTomatoCount);

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

    // ==============================================================
    // ==================== 统一应用暗色主题样式的 Lambda 函数 =========
    auto applyTheme = [this]() {
        bool isDark = AlgeMate::ThemeManager::instance().currentTheme() == AlgeMate::ThemeManager::Theme::Dark;
        if (auto* btn = findChild<QPushButton*>(QStringLiteral("LearnBackBtn"))) {
            btn->setStyleSheet(isDark
                                   ? "QPushButton { background: transparent; border: 1px solid #3B395A; padding: 6px 12px; border-radius: 6px; color: #C9C9DC;} QPushButton:hover { background: #28263F; }"
                                   : "QPushButton { background: transparent; border: 1px solid #cbd5e0; padding: 6px 12px; border-radius: 6px; color: #4a5568;} QPushButton:hover { background: #edf2f7; }");
        }
        if (auto* tr = findChild<QLabel*>(QStringLiteral("TrendTitle"))) {
            tr->setStyleSheet(isDark ? "font-size:16px; font-weight:700; color:#E6E7F0; background:transparent;" : "font-size:16px; font-weight:700; color:#24253D; background:transparent;");
        }
        if (auto* rr = findChild<QLabel*>(QStringLiteral("RecordTitle"))) {
            rr->setStyleSheet(isDark ? "font-size:16px; font-weight:700; color:#E6E7F0; background:transparent;" : "font-size:16px; font-weight:700; color:#24253D; background:transparent;");
        }
        if (m_records) {
            m_records->setStyleSheet(isDark ? "QListWidget { background:transparent; color:#C9C9DC; font-size:13px; } QListWidget::item { padding:6px 0; }"
                                            : "QListWidget { background:transparent; color:#8A8FA3; font-size:13px; } QListWidget::item { padding:6px 0; }");
        }
        if (auto* mLabel = findChild<QLabel*>("CalendarMonthLabel")) {
            mLabel->setStyleSheet(isDark ? "color: #E6E7F0;" : "color: #24253D;");
        }
        if (auto* pBtn = findChild<QToolButton*>("CalPrevBtn")) {
            pBtn->setStyleSheet(isDark ? "color: #8FA1FF; font-weight: bold; font-size: 14px; border: none;" : "color: #6A5AE0; font-weight: bold; font-size: 14px; border: none;");
        }
        if (auto* nBtn = findChild<QToolButton*>("CalNextBtn")) {
            nBtn->setStyleSheet(isDark ? "color: #8FA1FF; font-weight: bold; font-size: 14px; border: none;" : "color: #6A5AE0; font-weight: bold; font-size: 14px; border: none;");
        }
        if (auto* yBtn = findChild<QPushButton*>("CalYearBtn")) {
            yBtn->setStyleSheet(isDark
                                    ? "QPushButton { background-color: #312F4A; color: #8FA1FF; border-radius: 4px; padding: 4px 10px; font-weight: bold; } QPushButton:hover { background-color: #3B395A; }"
                                    : "QPushButton { background-color: #F5F3FF; color: #6A5AE0; border-radius: 4px; padding: 4px 10px; font-weight: bold; } QPushButton:hover { background-color: #EBE7FF; }");
        }
        if (auto* tipL = findChild<QLabel*>("CalTipLabel")) {
            tipL->setStyleSheet(isDark ? "color: #8FA1FF;" : "color: #6A5AE0;");
        }
        update();
    };

    applyTheme();
    connect(&AlgeMate::ThemeManager::instance(), &AlgeMate::ThemeManager::themeChanged, this, [applyTheme](AlgeMate::ThemeManager::Theme){ applyTheme(); });
    // ==============================================================
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