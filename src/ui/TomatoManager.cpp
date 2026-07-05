#include "TomatoManager.h"
#include <QTime>

namespace AlgeMate {

TomatoManager& TomatoManager::instance() {
    static TomatoManager inst;
    return inst;
}

TomatoManager::TomatoManager(QObject* parent) : QObject(parent) {
    timer_ = new QTimer(this);
    timer_->setInterval(1000); // 1秒触发一次
    connect(timer_, &QTimer::timeout, this, &TomatoManager::onTimerTick);
}

void TomatoManager::setConfig(int focusMin, int shortBreakMin, int longBreakMin, int longBreakInterval) {
    focusDuration_ = focusMin * 60;
    shortBreakDuration_ = shortBreakMin * 60;
    longBreakDuration_ = longBreakMin * 60;
    longBreakInterval_ = longBreakInterval;
}

void TomatoManager::startFocus() {
    switchState(State::Focus);
}

void TomatoManager::pause() {
    if (timer_->isActive()) {
        timer_->stop();
    }
}

void TomatoManager::resume() {
    if (!timer_->isActive() && currentState_ != State::Idle) {
        timer_->start();
    }
}

void TomatoManager::reset() {
    timer_->stop();
    switchState(State::Idle);
}

void TomatoManager::switchState(State newState) {
    currentState_ = newState;

    switch (currentState_) {
    case State::Idle:
        remainingSeconds_ = 0;
        break;
    case State::Focus:
        remainingSeconds_ = focusDuration_;
        break;
    case State::ShortBreak:
        remainingSeconds_ = shortBreakDuration_;
        break;
    case State::LongBreak:
        remainingSeconds_ = longBreakDuration_;
        break;
    }

    emit statusChanged(currentState_);
    emit tick(remainingSeconds_, remainingTimeStr());

    if (currentState_ != State::Idle) {
        timer_->start();
    }
}

void TomatoManager::onTimerTick() {
    if (remainingSeconds_ > 0) {
        remainingSeconds_--;
        emit tick(remainingSeconds_, remainingTimeStr());
    } else {
        timer_->stop();
        emit finished(); // 倒计时结束通知

        // 自动状态流转逻辑
        if (currentState_ == State::Focus) {
            completedTomatoes_++;
            // 判断是进入长休还是短休
            if (completedTomatoes_ % longBreakInterval_ == 0) {
                switchState(State::LongBreak);
            } else {
                switchState(State::ShortBreak);
            }
        } else {
            // 休息结束，自动切回空闲（或者你也可以配置成自动进入下一个专注期）
            switchState(State::Idle);
        }
    }
}

QString TomatoManager::remainingTimeStr() const {
    int mins = remainingSeconds_ / 60;
    int secs = remainingSeconds_ % 60;
    return QString("%1:%2")
        .arg(mins, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

} // namespace AlgeMate