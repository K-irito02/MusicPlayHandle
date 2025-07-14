#ifndef PLAYINTERFACE_H
#define PLAYINTERFACE_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QPixmap>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsItem>

QT_BEGIN_NAMESPACE
namespace Ui {
class PlayInterface;
}
QT_END_NAMESPACE

class PlayInterfaceController;

class PlayInterface : public QWidget
{
    Q_OBJECT

public:
    PlayInterface(QWidget *parent = nullptr);
    ~PlayInterface();

    // 播放控制
    void setPlaybackState(bool isPlaying);
    void setCurrentTime(qint64 time);
    void setTotalTime(qint64 time);
    void setVolume(int volume);
    void setBalance(int balance);
    void setMuted(bool muted);
    
    // 歌曲信息
    void setSongTitle(const QString& title);
    void setSongArtist(const QString& artist);
    void setSongAlbum(const QString& album);
    void setSongCover(const QPixmap& cover);
    void setLyrics(const QString& lyrics);
    
    // 可视化
    void updateWaveform(const QVector<float>& data);
    void updateSpectrum(const QVector<float>& data);
    void updateVUMeter(float leftLevel, float rightLevel);
    
    // 显示模式
    void setDisplayMode(int mode); // 0: 歌词, 1: 封面, 2: 可视化
    
    // 均衡器
    void setEqualizerValues(const QVector<int>& values);
    QVector<int> getEqualizerValues() const;

    bool isMuted() const { return m_isMuted; }

signals:
    void playPauseClicked();
    void stopClicked();
    void previousClicked();
    void nextClicked();
    void volumeChanged(int volume);
    void balanceChanged(int balance);
    void positionChanged(qint64 position);
    void muteToggled(bool muted);
    void equalizerChanged(const QVector<int>& values);
    void displayModeChanged(int mode);

private slots:
    void onPlayPauseClicked();
    void onStopClicked();
    void onPreviousClicked();
    void onNextClicked();
    void onVolumeSliderChanged(int value);
    void onBalanceSliderChanged(int value);
    void onPositionSliderChanged(int value);
    void onMuteButtonClicked();
    void onEqualizerSliderChanged(int value);
    void onDisplayModeChanged();
    void onUpdateTimer();

private:
    Ui::PlayInterface *ui;
    PlayInterfaceController* m_controller;
    
    QTimer* m_updateTimer;
    QGraphicsView* m_waveformView;
    QGraphicsView* m_spectrumView;
    QGraphicsScene* m_waveformScene;
    QGraphicsScene* m_spectrumScene;
    
    bool m_isPlaying;
    qint64 m_currentTime;
    qint64 m_totalTime;
    int m_volume;
    int m_balance;
    bool m_isMuted;
    int m_displayMode;
    
    void setupConnections();
    void setupUI();
    void setupVisualization();
    void updateTimeDisplay();
    void updateVolumeDisplay();
    void updatePlaybackControls();
    void drawWaveform(const QVector<float>& data);
    void drawSpectrum(const QVector<float>& data);
    void drawVUMeter(float leftLevel, float rightLevel);
    QString formatTime(qint64 milliseconds) const;
};

#endif // PLAYINTERFACE_H 