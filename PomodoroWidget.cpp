// Copyright (C) 2026 Sean Moon
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "PomodoroWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

// ---------------------------------------------------------------------------
// Internal display widget
// ---------------------------------------------------------------------------

enum class Phase { Work, ShortBreak, LongBreak };

class PomodoroDisplay : public QWidget {
    Q_OBJECT
public:
    PomodoroDisplay(int workMin, int shortMin, int longMin, int total, QWidget* parent)
        : QWidget(parent),
          workSecs_(workMin * 60),
          shortBreakSecs_(shortMin * 60),
          longBreakSecs_(longMin * 60),
          totalPomodoros_(total),
          secondsLeft_(workMin * 60) {
        buildUi();

        timer_ = new QTimer(this);
        timer_->setInterval(1000);
        connect(timer_, &QTimer::timeout, this, &PomodoroDisplay::tick);
        connect(startButton_, &QPushButton::clicked, this, &PomodoroDisplay::toggleStartPause);
        connect(resetButton_, &QPushButton::clicked, this, &PomodoroDisplay::reset);

        updateAll();
    }

    int totalPomodoros() const { return totalPomodoros_; }

signals:
    void pomodoroCompleted();

private slots:
    void tick() {
        if (secondsLeft_ > 0) {
            --secondsLeft_;
            updateTimerLabel();
        } else {
            transitionToNextPhase();
        }
    }

    void toggleStartPause() {
        if (running_) {
            timer_->stop();
            running_ = false;
            startButton_->setText("▶  Resume");
        } else {
            timer_->start();
            running_ = true;
            startButton_->setText("⏸  Pause");
        }
    }

    void reset() {
        timer_->stop();
        running_ = false;
        secondsLeft_ = phaseDuration();
        updateTimerLabel();
        startButton_->setText("▶  Start");
    }

private:
    void buildUi() {
        const QString btnStyle =
            "QPushButton {"
            "  background-color: #26263a;"
            "  color: #c8cee8;"
            "  border: 1px solid #3e3e60;"
            "  border-radius: 6px;"
            "  padding: 5px 16px;"
            "  font-size: 12px;"
            "}"
            "QPushButton:hover { background-color: #303050; }"
            "QPushButton:pressed { background-color: #1e1e30; }";

        auto* vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(12, 14, 12, 14);
        vbox->setSpacing(6);

        // Phase label
        phaseLabel_ = new QLabel(this);
        phaseLabel_->setAlignment(Qt::AlignCenter);

        // Timer display
        timerLabel_ = new QLabel(this);
        timerLabel_->setAlignment(Qt::AlignCenter);
        timerLabel_->setStyleSheet(
            "color: #e8eaf0; font-size: 40px; font-weight: bold; background: transparent;");
        QFont mono = timerLabel_->font();
        mono.setStyleHint(QFont::Monospace);
        timerLabel_->setFont(mono);

        // Pomodoro dots
        auto* dotsRow    = new QWidget(this);
        auto* dotsLayout = new QHBoxLayout(dotsRow);
        dotsLayout->setContentsMargins(0, 4, 0, 4);
        dotsLayout->setSpacing(10);
        dotsLayout->setAlignment(Qt::AlignCenter);
        dotsRow->setStyleSheet("background: transparent;");
        for (int i = 0; i < 4; i++) {
            auto* dot = new QLabel("○", dotsRow);
            dot->setAlignment(Qt::AlignCenter);
            dotsLayout->addWidget(dot);
            dots_.append(dot);
        }

        // Start / Reset buttons
        auto* buttonsRow    = new QWidget(this);
        auto* buttonsLayout = new QHBoxLayout(buttonsRow);
        buttonsLayout->setContentsMargins(0, 0, 0, 0);
        buttonsLayout->setSpacing(8);
        buttonsLayout->setAlignment(Qt::AlignCenter);
        buttonsRow->setStyleSheet("background: transparent;");

        startButton_ = new QPushButton("▶  Start", buttonsRow);
        startButton_->setStyleSheet(btnStyle);
        startButton_->setCursor(Qt::PointingHandCursor);

        resetButton_ = new QPushButton("↺  Reset", buttonsRow);
        resetButton_->setStyleSheet(btnStyle);
        resetButton_->setCursor(Qt::PointingHandCursor);

        buttonsLayout->addWidget(startButton_);
        buttonsLayout->addWidget(resetButton_);

        // Total count
        totalLabel_ = new QLabel(this);
        totalLabel_->setAlignment(Qt::AlignCenter);
        totalLabel_->setStyleSheet(
            "color: #606080; font-size: 11px; background: transparent;");

        vbox->addStretch(1);
        vbox->addWidget(phaseLabel_);
        vbox->addWidget(timerLabel_);
        vbox->addWidget(dotsRow);
        vbox->addSpacing(4);
        vbox->addWidget(buttonsRow);
        vbox->addSpacing(4);
        vbox->addWidget(totalLabel_);
        vbox->addStretch(1);
    }

    void transitionToNextPhase() {
        timer_->stop();
        running_ = false;

        if (phase_ == Phase::Work) {
            ++pomodorosInCycle_;
            ++totalPomodoros_;
            emit pomodoroCompleted();

            if (pomodorosInCycle_ >= 4) {
                phase_       = Phase::LongBreak;
                secondsLeft_ = longBreakSecs_;
            } else {
                phase_       = Phase::ShortBreak;
                secondsLeft_ = shortBreakSecs_;
            }
        } else if (phase_ == Phase::LongBreak) {
            pomodorosInCycle_ = 0;
            phase_            = Phase::Work;
            secondsLeft_      = workSecs_;
        } else {
            phase_       = Phase::Work;
            secondsLeft_ = workSecs_;
        }

        updateAll();
    }

    int phaseDuration() const {
        switch (phase_) {
            case Phase::Work:       return workSecs_;
            case Phase::ShortBreak: return shortBreakSecs_;
            case Phase::LongBreak:  return longBreakSecs_;
        }
        return workSecs_;
    }

    void updateTimerLabel() {
        timerLabel_->setText(QString("%1:%2")
            .arg(secondsLeft_ / 60, 2, 10, QChar('0'))
            .arg(secondsLeft_ % 60, 2, 10, QChar('0')));
    }

    void updatePhaseLabel() {
        switch (phase_) {
            case Phase::Work:
                phaseLabel_->setText("WORK");
                phaseLabel_->setStyleSheet(
                    "color: #e05555; font-size: 11px; font-weight: bold;"
                    " letter-spacing: 3px; background: transparent;");
                break;
            case Phase::ShortBreak:
                phaseLabel_->setText("SHORT BREAK");
                phaseLabel_->setStyleSheet(
                    "color: #55b055; font-size: 11px; font-weight: bold;"
                    " letter-spacing: 3px; background: transparent;");
                break;
            case Phase::LongBreak:
                phaseLabel_->setText("LONG BREAK");
                phaseLabel_->setStyleSheet(
                    "color: #5599cc; font-size: 11px; font-weight: bold;"
                    " letter-spacing: 3px; background: transparent;");
                break;
        }
    }

    void updateDots() {
        // pomodorosInCycle_ ranges 0..4; all 4 filled during/after long break
        for (int i = 0; i < 4; i++) {
            if (i < pomodorosInCycle_) {
                dots_[i]->setText("●");
                dots_[i]->setStyleSheet(
                    "color: #e05555; font-size: 16px; background: transparent;");
            } else {
                dots_[i]->setText("○");
                dots_[i]->setStyleSheet(
                    "color: #404060; font-size: 16px; background: transparent;");
            }
        }
    }

    void updateTotalLabel() {
        totalLabel_->setText(
            totalPomodoros_ == 1
                ? "1 pomodoro completed"
                : QString("%1 pomodoros completed").arg(totalPomodoros_));
    }

    void updateAll() {
        updateTimerLabel();
        updatePhaseLabel();
        updateDots();
        updateTotalLabel();
        startButton_->setText("▶  Start");
    }

    QTimer*         timer_;
    QLabel*         phaseLabel_;
    QLabel*         timerLabel_;
    QList<QLabel*>  dots_;
    QPushButton*    startButton_;
    QPushButton*    resetButton_;
    QLabel*         totalLabel_;

    Phase phase_            = Phase::Work;
    int   secondsLeft_;
    bool  running_          = false;
    int   pomodorosInCycle_ = 0;
    int   totalPomodoros_;

    int workSecs_;
    int shortBreakSecs_;
    int longBreakSecs_;
};

// ---------------------------------------------------------------------------
// PomodoroWidget (plugin entry point)
// ---------------------------------------------------------------------------

PomodoroWidget::PomodoroWidget(QObject* parent) : QObject(parent) {}

void PomodoroWidget::initialize(dashboard::WidgetContext* /*context*/) {}

QWidget* PomodoroWidget::createWidget(QWidget* parent) {
    auto* display = new PomodoroDisplay(
        workMinutes_, shortBreakMinutes_, longBreakMinutes_, totalPomodoros_, parent);

    // Keep totalPomodoros_ in sync so serialize() always has the latest count
    QObject::connect(display, &PomodoroDisplay::pomodoroCompleted, display, [this]() {
        ++totalPomodoros_;
    });

    return display;
}

QJsonObject PomodoroWidget::serialize() const {
    return {
        {"workMinutes",       workMinutes_},
        {"shortBreakMinutes", shortBreakMinutes_},
        {"longBreakMinutes",  longBreakMinutes_},
        {"totalPomodoros",    totalPomodoros_},
    };
}

void PomodoroWidget::deserialize(const QJsonObject& data) {
    workMinutes_       = data.value("workMinutes").toInt(25);
    shortBreakMinutes_ = data.value("shortBreakMinutes").toInt(5);
    longBreakMinutes_  = data.value("longBreakMinutes").toInt(15);
    totalPomodoros_    = data.value("totalPomodoros").toInt(0);
}

dashboard::WidgetMetadata PomodoroWidget::metadata() const {
    return {
        .name        = "Pomodoro",
        .version     = "1.0.0",
        .author      = "Dashboard",
        .description = "Focus timer based on the Pomodoro Technique",
        .minSize     = QSize(200, 240),
        .maxSize     = QSize(400, 400),
        .defaultSize = QSize(240, 280),
    };
}

#include "PomodoroWidget.moc"
