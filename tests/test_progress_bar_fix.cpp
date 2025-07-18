#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QDebug>
#include "../src/ui/widgets/musicprogressbar.h"
#include "../src/audio/audioengine.h"

class ProgressBarTestWindow : public QMainWindow
{
    Q_OBJECT

public:
    ProgressBarTestWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("进度条拖动功能测试");
        setGeometry(100, 100, 600, 200);

        QWidget *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        QVBoxLayout *layout = new QVBoxLayout(centralWidget);

        // 创建测试按钮
        QPushButton *testButton = new QPushButton("测试进度条", this);
        layout->addWidget(testButton);

        // 创建音乐进度条
        m_progressBar = new MusicProgressBar(this);
        layout->addWidget(m_progressBar);

        // 连接信号
        connect(testButton, &QPushButton::clicked, this, &ProgressBarTestWindow::startTest);
        connect(m_progressBar, &MusicProgressBar::seekRequested, this, &ProgressBarTestWindow::onSeekRequested);
        connect(m_progressBar, &MusicProgressBar::positionChanged, this, &ProgressBarTestWindow::onPositionChanged);

        // 创建模拟播放定时器
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &ProgressBarTestWindow::updatePosition);

        // 设置初始状态
        m_progressBar->setDuration(300000); // 5分钟
        m_progressBar->setPosition(0);
        m_currentPosition = 0;
    }

private slots:
    void startTest()
    {
        qDebug() << "[测试] 开始进度条测试";
        m_timer->start(1000); // 每秒更新一次
    }

    void updatePosition()
    {
        m_currentPosition += 1000; // 增加1秒
        if (m_currentPosition > 300000) {
            m_currentPosition = 0;
        }
        m_progressBar->updatePosition(m_currentPosition);
        qDebug() << "[测试] 更新位置:" << m_currentPosition << "ms";
    }

    void onSeekRequested(qint64 position)
    {
        qDebug() << "[测试] 收到跳转请求:" << position << "ms";
        m_currentPosition = position;
        m_progressBar->updatePosition(position);
    }

    void onPositionChanged(qint64 position)
    {
        qDebug() << "[测试] 位置改变:" << position << "ms";
    }

private:
    MusicProgressBar *m_progressBar;
    QTimer *m_timer;
    qint64 m_currentPosition;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    ProgressBarTestWindow window;
    window.show();

    return app.exec();
}

#include "test_progress_bar_fix.moc" 