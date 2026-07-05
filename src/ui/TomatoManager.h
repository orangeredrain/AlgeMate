#pragma once

#include <QObject>
#include <QTimer>

namespace AlgeMate {

class TomatoManager : public QObject {
    Q_OBJECT

public:
    // 番茄钟的三种状态
    enum class State {
        Idle,       // 空闲/未开始
        Focus,      // 专注中
        ShortBreak, // 短暂休息
        LongBreak   // 长时间休息
    };
    Q_ENUM(State)

    static TomatoManager& instance();

    // 控制接口
    void startFocus();          // 开始专注
    void pause();               // 暂停
    void resume();              // 恢复
    void reset();               // 重置（回到空闲）

    // 配置与状态获取
    void setConfig(int focusMin, int shortBreakMin, int longBreakMin, int longBreakInterval);
    State currentState() const { return currentState_; }
    int remainingSeconds() const { return remainingSeconds_; }
    int completedTomatoes() const { return completedTomatoes_; }


    QString remainingTimeStr() const; // 返回类似 "24:59" 的字符串

signals:
    void statusChanged(TomatoManager::State state);
    void tick(int remainingSeconds, const QString& timeStr);
    void finished(); // 单个番茄钟或休息结束时触发

private:
    explicit TomatoManager(QObject* parent = nullptr);
    ~TomatoManager() = default;
    TomatoManager(const TomatoManager&) = delete;
    TomatoManager& operator=(const TomatoManager&) = delete;

    void switchState(State newState);

private slots:
    void onTimerTick();

private:
    QTimer* timer_;
    State currentState_ = State::Idle;

    // 时间配置（秒）
    int focusDuration_ = 25 * 60;
    int shortBreakDuration_ = 5 * 60;
    int longBreakDuration_ = 15 * 60;

    int longBreakInterval_ = 4; // 每 4 个番茄钟进行一次大休息
    int completedTomatoes_ = 0;  // 已完成的番茄钟数量

    int remainingSeconds_ = 0;   // 当前阶段剩余秒数
};

} // namespace AlgeMate