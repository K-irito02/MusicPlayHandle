#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QVector>
#include <QTimer>
#include <QAudioSink>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QAtomicInt>

// FFmpeg头文件
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
}

class FFmpegDecoder : public QObject
{
    Q_OBJECT

public:
    explicit FFmpegDecoder(QObject* parent = nullptr);
    ~FFmpegDecoder();

    // 初始化和清理
    bool initialize();
    void cleanup();

    // 音频文件操作
    bool openFile(const QString& filePath);
    void closeFile();

    // 播放控制
    bool startDecoding();
    void stopDecoding();
    void seekTo(qint64 position);

    // 音频数据处理
    QVector<double> getCurrentLevels() const;
    void setBalance(double balance);
    double getBalance() const;

    // 状态查询
    bool isDecoding() const;
    qint64 getDuration() const;
    qint64 getCurrentPosition() const;
    bool isEndOfFile() const;

signals:
    void audioDataReady(const QVector<double>& levels);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void decodingFinished();
    void errorOccurred(const QString& error);

private slots:
    void decodeLoop();

private:
    // FFmpeg相关
    AVFormatContext* m_formatContext;
    AVCodecContext* m_codecContext;
    SwrContext* m_swrContext;
    AVFrame* m_inputFrame;
    AVFrame* m_outputFrame;
    AVPacket* m_packet;
    
    // 音频流信息
    int m_audioStreamIndex;
    qint64 m_duration;
    qint64 m_currentPosition;
    
    // 解码线程
    QThread* m_decodeThread;
    QTimer* m_decodeTimer;
    QAtomicInt m_isDecoding;
    bool m_isEndOfFile;
    
    // 音频数据
    QVector<double> m_currentLevels;
    QQueue<QVector<double>> m_levelBuffer;
    double m_balance;
    
    // 线程安全
    mutable QMutex m_mutex;
    
    // 内部方法
    bool setupCodec();
    bool setupResampler();
    void processAudioFrame(AVFrame* frame);
    void calculateLevels(const float* samples, int frameCount, int channels);
    void cleanupFFmpeg();
    void resetState();
    

    
    // 音频输出
    QAudioSink* m_audioSink;
    QIODevice* m_audioDevice;
    QAudioFormat m_audioFormat;
    QByteArray m_audioBuffer;
    
    // 音频输出方法
    bool setupAudioOutput();
    void cleanupAudioOutput();
    void writeAudioData(const QByteArray& data);
};

#endif // FFMPEGDECODER_H 