#include "ffmpegdecoder.h"
#include <QDebug>
#include <QFileInfo>
#include <QtMath>
#include <cstdint>  // 为int16_t类型

FFmpegDecoder::FFmpegDecoder(QObject* parent)
    : QObject(parent)
    , m_formatContext(nullptr)
    , m_codecContext(nullptr)
    , m_swrContext(nullptr)
    , m_inputFrame(nullptr)
    , m_outputFrame(nullptr)
    , m_packet(nullptr)
    , m_audioStreamIndex(-1)
    , m_duration(0)
    , m_currentPosition(0)
    , m_decodeThread(nullptr)
    , m_decodeTimer(nullptr)
    , m_isDecoding(0)
    , m_isEndOfFile(false)
    , m_currentLevels(2, 0.0)
    , m_balance(0.0)
    , m_audioSink(nullptr)
    , m_audioDevice(nullptr)
{
    qDebug() << "FFmpegDecoder: 构造函数";
}

FFmpegDecoder::~FFmpegDecoder()
{
    qDebug() << "FFmpegDecoder: 析构函数";
    cleanup();
}

bool FFmpegDecoder::initialize()
{
    qDebug() << "FFmpegDecoder: 开始初始化...";
    
    try {
        // 初始化基本成员变量
        m_formatContext = nullptr;
        m_codecContext = nullptr;
        m_swrContext = nullptr;
        m_inputFrame = nullptr;
        m_outputFrame = nullptr;
        m_packet = nullptr;
        m_decodeThread = nullptr;
        m_decodeTimer = nullptr;
        m_audioSink = nullptr;
        m_audioDevice = nullptr;
        
        m_audioStreamIndex = -1;
        m_duration = 0;
        m_currentPosition = 0;
        m_balance = 0.0;
        m_isDecoding.storeRelease(0);
        m_isEndOfFile = false;
        
        qDebug() << "FFmpegDecoder: 成员变量初始化完成";
        
        // 重新启用解码功能，用于VU表数据生成
        qDebug() << "FFmpegDecoder: 创建解码线程...";
        m_decodeThread = new QThread(this);
        m_decodeTimer = new QTimer();
        m_decodeTimer->moveToThread(m_decodeThread);
        
        qDebug() << "FFmpegDecoder: 连接信号槽...";
        connect(m_decodeTimer, &QTimer::timeout, this, &FFmpegDecoder::decodeLoop);
        connect(m_decodeThread, &QThread::started, [this]() {
            qDebug() << "FFmpegDecoder: 解码线程启动，开始定时器";
            m_decodeTimer->start(10); // 10ms间隔，100fps
        });
        
        qDebug() << "FFmpegDecoder: 解码线程设置完成";
        
        qDebug() << "FFmpegDecoder: 初始化完成（模拟模式）";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 初始化异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "FFmpegDecoder: 初始化未知异常";
        return false;
    }
}

void FFmpegDecoder::cleanup()
{
    qDebug() << "FFmpegDecoder: 清理";
    
    stopDecoding();
    closeFile();
    
    // 重新启用线程清理
    qDebug() << "FFmpegDecoder: 清理解码线程...";
    if (m_decodeTimer) {
        m_decodeTimer->stop();
        delete m_decodeTimer;
        m_decodeTimer = nullptr;
        qDebug() << "FFmpegDecoder: 解码定时器已清理";
    }
    
    if (m_decodeThread) {
        m_decodeThread->quit();
        m_decodeThread->wait();
        delete m_decodeThread;
        m_decodeThread = nullptr;
        qDebug() << "FFmpegDecoder: 解码线程已清理";
    }
    
    cleanupFFmpeg();
    cleanupAudioOutput();
}

bool FFmpegDecoder::openFile(const QString& filePath)
{
    qDebug() << "FFmpegDecoder: 开始打开文件:" << filePath;
    
    try {
        // 先关闭之前的文件（不在这里获取锁，避免死锁）
        qDebug() << "FFmpegDecoder: 关闭之前的文件...";
        closeFile();
        
        QMutexLocker locker(&m_mutex);
        
        // 检查文件是否存在
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            qWarning() << "FFmpegDecoder: 文件不存在:" << filePath;
            emit errorOccurred("文件不存在: " + filePath);
            return false;
        }
        
        qDebug() << "FFmpegDecoder: 文件存在，大小:" << fileInfo.size() << "字节";
        
        // 打开输入文件
        qDebug() << "FFmpegDecoder: 分配格式上下文...";
        m_formatContext = avformat_alloc_context();
        if (!m_formatContext) {
            qCritical() << "FFmpegDecoder: 无法分配格式上下文";
            emit errorOccurred("无法分配格式上下文");
            return false;
        }
        
        qDebug() << "FFmpegDecoder: 格式上下文分配成功";
        
        qDebug() << "FFmpegDecoder: 打开输入文件...";
        if (avformat_open_input(&m_formatContext, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
            qCritical() << "FFmpegDecoder: 无法打开输入文件";
            emit errorOccurred("无法打开输入文件");
            cleanupFFmpeg();
            return false;
        }
        
        qDebug() << "FFmpegDecoder: 输入文件打开成功";
        
        // 查找流信息
        qDebug() << "FFmpegDecoder: 查找流信息...";
        if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
            qCritical() << "FFmpegDecoder: 无法查找流信息";
            emit errorOccurred("无法查找流信息");
            cleanupFFmpeg();
            return false;
        }
        
        qDebug() << "FFmpegDecoder: 流信息查找成功，流数量:" << m_formatContext->nb_streams;
        
        // 查找音频流
        qDebug() << "FFmpegDecoder: 查找音频流...";
        m_audioStreamIndex = -1;
        for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
            qDebug() << "FFmpegDecoder: 检查流" << i << "，类型:" << m_formatContext->streams[i]->codecpar->codec_type;
            if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                m_audioStreamIndex = i;
                qDebug() << "FFmpegDecoder: 找到音频流，索引:" << i;
                break;
            }
        }
        
        if (m_audioStreamIndex < 0) {
            qCritical() << "FFmpegDecoder: 未找到音频流";
            emit errorOccurred("未找到音频流");
            cleanupFFmpeg();
            return false;
        }
        
        qDebug() << "FFmpegDecoder: 音频流索引:" << m_audioStreamIndex;
        
        // 重新启用编解码器设置
        qDebug() << "FFmpegDecoder: 设置编解码器...";
        if (!setupCodec()) {
            qWarning() << "FFmpegDecoder: 编解码器设置失败";
            cleanupFFmpeg();
            return false;
        }
        qDebug() << "FFmpegDecoder: 编解码器设置成功";
        
        // 先设置音频输出，获取音频格式信息
        qDebug() << "FFmpegDecoder: 设置音频输出...";
        if (!setupAudioOutput()) {
            qWarning() << "FFmpegDecoder: 音频输出设置失败";
            cleanupFFmpeg();
            return false;
        }
        qDebug() << "FFmpegDecoder: 音频输出设置成功";
        
        // 然后设置重采样器，使用音频格式信息
        qDebug() << "FFmpegDecoder: 设置重采样器...";
        if (!setupResampler()) {
            qWarning() << "FFmpegDecoder: 重采样器设置失败";
            cleanupFFmpeg();
            return false;
        }
        qDebug() << "FFmpegDecoder: 重采样器设置成功";
        
        // 获取时长
        if (m_formatContext->duration != AV_NOPTS_VALUE) {
            m_duration = m_formatContext->duration / 1000; // 转换为毫秒
            qDebug() << "FFmpegDecoder: 获取到时长:" << m_duration << "ms";
        } else {
            qDebug() << "FFmpegDecoder: 无法获取时长信息";
            m_duration = 0;
        }
        
        // 重置状态（但不重置音频流索引）
        m_duration = 0;
        m_currentPosition = 0;
        m_isDecoding.storeRelease(0);
        m_isEndOfFile = false;
        m_currentLevels.fill(0.0);
        m_levelBuffer.clear();
    
        emit durationChanged(m_duration);
        qDebug() << "FFmpegDecoder: 文件打开成功，时长:" << m_duration << "ms";
        
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 打开文件异常:" << e.what();
        cleanupFFmpeg();
        return false;
    } catch (...) {
        qCritical() << "FFmpegDecoder: 打开文件未知异常";
        cleanupFFmpeg();
        return false;
    }
}

void FFmpegDecoder::closeFile()
{
    qDebug() << "FFmpegDecoder: 开始关闭文件...";
    
    try {
        QMutexLocker locker(&m_mutex);
        
        qDebug() << "FFmpegDecoder: 停止解码...";
        // 直接在这里停止解码，不调用stopDecoding()
        if (m_isDecoding.loadAcquire()) {
            qDebug() << "FFmpegDecoder: 设置解码状态为停止...";
            m_isDecoding.storeRelease(0);
            qDebug() << "FFmpegDecoder: 解码停止完成（模拟模式）";
        }
        
        qDebug() << "FFmpegDecoder: 清理FFmpeg资源...";
        cleanupFFmpeg();
        
        qDebug() << "FFmpegDecoder: 清理音频输出...";
        cleanupAudioOutput();
        
        qDebug() << "FFmpegDecoder: 重置状态...";
        resetState();
        
        qDebug() << "FFmpegDecoder: 文件关闭完成";
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 关闭文件异常:" << e.what();
    } catch (...) {
        qCritical() << "FFmpegDecoder: 关闭文件未知异常";
    }
}

bool FFmpegDecoder::startDecoding()
{
    qDebug() << "FFmpegDecoder: 开始解码（模拟模式）...";
    
    try {
        QMutexLocker locker(&m_mutex);
        
        if (!m_formatContext) {
            qWarning() << "FFmpegDecoder: 格式上下文为空，无法开始解码";
            return false;
        }
        
        if (m_isDecoding.loadAcquire()) {
            qDebug() << "FFmpegDecoder: 已经在解码中，跳过";
            return true;
        }
        
        qDebug() << "FFmpegDecoder: 设置解码状态...";
        m_isDecoding.storeRelease(1);
        m_isEndOfFile = false;
        
        qDebug() << "FFmpegDecoder: 解码状态设置完成";
        
        // 启动解码线程
        if (m_decodeThread && !m_decodeThread->isRunning()) {
            qDebug() << "FFmpegDecoder: 启动解码线程...";
            m_decodeThread->start();
            qDebug() << "FFmpegDecoder: 解码线程启动成功";
        } else {
            qWarning() << "FFmpegDecoder: 解码线程已运行或不存在";
        }
        
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 开始解码异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "FFmpegDecoder: 开始解码未知异常";
        return false;
    }
}

void FFmpegDecoder::stopDecoding()
{
    qDebug() << "FFmpegDecoder: 开始停止解码...";
    
    try {
        QMutexLocker locker(&m_mutex);
        
        if (!m_isDecoding.loadAcquire()) {
            qDebug() << "FFmpegDecoder: 未在解码中，跳过停止";
            return;
        }
        
        qDebug() << "FFmpegDecoder: 设置解码状态为停止...";
        m_isDecoding.storeRelease(0);
        
        qDebug() << "FFmpegDecoder: 解码停止完成";
        
        // 停止解码线程
        if (m_decodeThread && m_decodeThread->isRunning()) {
            qDebug() << "FFmpegDecoder: 停止解码线程...";
            m_decodeThread->quit();
            m_decodeThread->wait();
            qDebug() << "FFmpegDecoder: 解码线程停止成功";
        } else {
            qDebug() << "FFmpegDecoder: 解码线程未运行或不存在";
        }
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 停止解码异常:" << e.what();
    } catch (...) {
        qCritical() << "FFmpegDecoder: 停止解码未知异常";
    }
}

void FFmpegDecoder::seekTo(qint64 position)
{
    qDebug() << "FFmpegDecoder: 跳转到位置:" << position << "ms";
    
    try {
        QMutexLocker locker(&m_mutex);
        
        if (!m_formatContext) {
            qWarning() << "FFmpegDecoder: 格式上下文为空，无法跳转";
            return;
        }
        
        if (!m_isDecoding.loadAcquire()) {
            qWarning() << "FFmpegDecoder: 未在解码中，无法跳转";
            return;
        }
        
        qDebug() << "FFmpegDecoder: 清空缓冲区...";
        // 清空缓冲区
        m_levelBuffer.clear();
        
        // 执行跳转
        int64_t timestamp = position * 1000; // 转换为微秒
        qDebug() << "FFmpegDecoder: 计算时间戳:" << timestamp << "微秒";
        
        int ret = av_seek_frame(m_formatContext, m_audioStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
        if (ret >= 0) {
            qDebug() << "FFmpegDecoder: 跳转成功";
            
            // 清空解码器缓冲区
            if (m_codecContext) {
                avcodec_flush_buffers(m_codecContext);
                qDebug() << "FFmpegDecoder: 解码器缓冲区已清空";
            } else {
                qDebug() << "FFmpegDecoder: 编解码器上下文为空，跳过缓冲区清空";
            }
            
            m_currentPosition = position;
            emit positionChanged(m_currentPosition);
            qDebug() << "FFmpegDecoder: 位置已更新为:" << m_currentPosition << "ms";
        } else {
            qWarning() << "FFmpegDecoder: 跳转失败，错误码:" << ret;
        }
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 跳转异常:" << e.what();
    } catch (...) {
        qCritical() << "FFmpegDecoder: 跳转未知异常";
    }
}

QVector<double> FFmpegDecoder::getCurrentLevels() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentLevels;
}

void FFmpegDecoder::setBalance(double balance)
{
    qDebug() << "FFmpegDecoder: 设置平衡值:" << balance;
    
    try {
        QMutexLocker locker(&m_mutex);
        
        double oldBalance = m_balance;
        m_balance = qBound(-1.0, balance, 1.0);
        
        qDebug() << "FFmpegDecoder: 平衡值从" << oldBalance << "更新为" << m_balance;
        
        // 如果正在解码，可以在这里添加实时平衡调整的逻辑
        if (m_isDecoding.loadAcquire()) {
            qDebug() << "FFmpegDecoder: 正在解码中，平衡值将在下次音频处理时生效";
        } else {
            qDebug() << "FFmpegDecoder: 未在解码中，平衡值已保存";
        }
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 设置平衡值异常:" << e.what();
    } catch (...) {
        qCritical() << "FFmpegDecoder: 设置平衡值未知异常";
    }
}

double FFmpegDecoder::getBalance() const
{
    QMutexLocker locker(&m_mutex);
    return m_balance;
}

bool FFmpegDecoder::isDecoding() const
{
    return m_isDecoding.loadAcquire() != 0;
}

qint64 FFmpegDecoder::getDuration() const
{
    QMutexLocker locker(&m_mutex);
    return m_duration;
}

qint64 FFmpegDecoder::getCurrentPosition() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentPosition;
}

bool FFmpegDecoder::isEndOfFile() const
{
    QMutexLocker locker(&m_mutex);
    return m_isEndOfFile;
}

void FFmpegDecoder::decodeLoop()
{
    qDebug() << "FFmpegDecoder: 解码循环开始...";
    
    // 检查解码状态（不需要锁，因为m_isDecoding是原子操作）
    if (!m_isDecoding.loadAcquire()) {
        qDebug() << "FFmpegDecoder: 未在解码中，退出解码循环";
        return;
    }
    
    // 使用局部锁保护FFmpeg操作
    {
        QMutexLocker locker(&m_mutex);
        
        if (!m_formatContext || !m_codecContext) {
            qWarning() << "FFmpegDecoder: 格式上下文或编解码器上下文为空";
            return;
        }
        
        // 读取数据包
        int ret = av_read_frame(m_formatContext, m_packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                qDebug() << "FFmpegDecoder: 到达文件末尾";
                m_isEndOfFile = true;
                emit decodingFinished();
            } else {
                qWarning() << "FFmpegDecoder: 读取数据包失败，错误码:" << ret;
            }
            return;
        }
        
        // 检查是否是音频流
        qDebug() << "FFmpegDecoder: 数据包流索引:" << m_packet->stream_index << "，音频流索引:" << m_audioStreamIndex;
        if (m_packet->stream_index != m_audioStreamIndex) {
            qDebug() << "FFmpegDecoder: 跳过非音频流数据包";
            av_packet_unref(m_packet);
            return;
        }
        
        qDebug() << "FFmpegDecoder: 读取音频数据包，大小:" << m_packet->size << "字节";
        
        // 发送数据包到解码器
        ret = avcodec_send_packet(m_codecContext, m_packet);
        if (ret < 0) {
            qWarning() << "FFmpegDecoder: 发送数据包到解码器失败，错误码:" << ret;
            av_packet_unref(m_packet);
            return;
        }
        
        // 接收解码后的帧
        int frameCount = 0;
        while (true) {
            ret = avcodec_receive_frame(m_codecContext, m_inputFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                qDebug() << "FFmpegDecoder: 需要更多数据或到达末尾";
                break;
            } else if (ret < 0) {
                qWarning() << "FFmpegDecoder: 接收解码帧失败，错误码:" << ret;
                break;
            }
            
            frameCount++;
            qDebug() << "FFmpegDecoder: 接收到解码帧" << frameCount;
            
            // 处理音频帧（在锁外处理，避免死锁）
            AVFrame* frameCopy = av_frame_alloc();
            av_frame_ref(frameCopy, m_inputFrame);
            
            // 更新位置
            if (m_inputFrame->pts != AV_NOPTS_VALUE) {
                m_currentPosition = m_inputFrame->pts * av_q2d(m_formatContext->streams[m_audioStreamIndex]->time_base) * 1000;
                emit positionChanged(m_currentPosition);
            }
            
            // 释放锁，处理音频帧
            locker.unlock();
            processAudioFrame(frameCopy);
            av_frame_free(&frameCopy);
            locker.relock();
        }
        
        if (frameCount > 0) {
            qDebug() << "FFmpegDecoder: 本次解码循环处理了" << frameCount << "个音频帧";
        }
        
        av_packet_unref(m_packet);
    }
}

bool FFmpegDecoder::setupCodec()
{
    qDebug() << "FFmpegDecoder: 开始设置编解码器...";
    
    try {
        // 获取编解码器参数
        const AVCodecParameters* codecParams = m_formatContext->streams[m_audioStreamIndex]->codecpar;
        qDebug() << "FFmpegDecoder: 获取编解码器参数成功";
        
        // 查找解码器
        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) {
            qCritical() << "FFmpegDecoder: 不支持的音频编解码器";
            emit errorOccurred("不支持的音频编解码器");
            return false;
        }
        qDebug() << "FFmpegDecoder: 找到解码器:" << codec->name;
        
        // 创建解码器上下文
        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext) {
            qCritical() << "FFmpegDecoder: 无法分配解码器上下文";
            emit errorOccurred("无法分配解码器上下文");
            return false;
        }
        qDebug() << "FFmpegDecoder: 解码器上下文分配成功";
        
        // 复制参数
        if (avcodec_parameters_to_context(m_codecContext, codecParams) < 0) {
            qCritical() << "FFmpegDecoder: 无法复制编解码器参数";
            emit errorOccurred("无法复制编解码器参数");
            return false;
        }
        qDebug() << "FFmpegDecoder: 编解码器参数复制成功";
        
        // 打开解码器
        if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
            qCritical() << "FFmpegDecoder: 无法打开解码器";
            emit errorOccurred("无法打开解码器");
            return false;
        }
        qDebug() << "FFmpegDecoder: 解码器打开成功";
        
        // 分配帧和数据包
        m_inputFrame = av_frame_alloc();
        m_outputFrame = av_frame_alloc();
        m_packet = av_packet_alloc();
        
        if (!m_inputFrame || !m_outputFrame || !m_packet) {
            qCritical() << "FFmpegDecoder: 无法分配帧或数据包";
            emit errorOccurred("无法分配帧或数据包");
            return false;
        }
        qDebug() << "FFmpegDecoder: 帧和数据包分配成功";
        
        qDebug() << "FFmpegDecoder: 编解码器设置完成";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 设置编解码器异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "FFmpegDecoder: 设置编解码器未知异常";
        return false;
    }
}

bool FFmpegDecoder::setupResampler()
{
    qDebug() << "FFmpegDecoder: 开始设置重采样器...";
    
    try {
        // 检查参数有效性
        if (m_codecContext->ch_layout.nb_channels <= 0) {
            qCritical() << "FFmpegDecoder: 无效的声道数:" << m_codecContext->ch_layout.nb_channels;
            emit errorOccurred("无效的声道数");
            return false;
        }
        
        if (m_codecContext->sample_rate <= 0) {
            qCritical() << "FFmpegDecoder: 无效的采样率:" << m_codecContext->sample_rate;
            emit errorOccurred("无效的采样率");
            return false;
        }
        
        // 获取音频设备格式信息
        int outputSampleRate = m_audioFormat.sampleRate();
        int outputChannels = m_audioFormat.channelCount();
        AVSampleFormat outputSampleFormat;
        
        // 根据QAudioFormat确定FFmpeg采样格式
        switch (m_audioFormat.sampleFormat()) {
            case QAudioFormat::Int16:
                outputSampleFormat = AV_SAMPLE_FMT_S16;
                break;
            case QAudioFormat::Int32:
                outputSampleFormat = AV_SAMPLE_FMT_S32;
                break;
            case QAudioFormat::Float:
                outputSampleFormat = AV_SAMPLE_FMT_FLT;
                break;
            default:
                outputSampleFormat = AV_SAMPLE_FMT_S16; // 默认使用16位
                break;
        }
        
        qDebug() << "FFmpegDecoder: 设置重采样参数...";
        qDebug() << "FFmpegDecoder: 输入声道布局:" << m_codecContext->ch_layout.u.mask;
        qDebug() << "FFmpegDecoder: 输入采样率:" << m_codecContext->sample_rate;
        qDebug() << "FFmpegDecoder: 输入采样格式:" << m_codecContext->sample_fmt;
        qDebug() << "FFmpegDecoder: 输出声道布局:" << (outputChannels == 2 ? "立体声" : "单声道");
        qDebug() << "FFmpegDecoder: 输出采样率:" << outputSampleRate;
        qDebug() << "FFmpegDecoder: 输出采样格式:" << outputSampleFormat;
        
        // 释放之前分配的重采样上下文
        if (m_swrContext) {
            swr_free(&m_swrContext);
        }
        
        // 使用swr_alloc_set_opts2一次性设置所有参数（FFmpeg 6.x版本）
        // 注意：FFmpeg 6.x中需要使用AVChannelLayout结构体
        AVChannelLayout outChLayout, inChLayout;
        
        // 根据输出声道数设置声道布局
        if (outputChannels == 2) {
            av_channel_layout_default(&outChLayout, 2);  // 立体声
        } else {
            av_channel_layout_default(&outChLayout, 1);  // 单声道
        }
        
        av_channel_layout_copy(&inChLayout, &m_codecContext->ch_layout);
        
        int ret = swr_alloc_set_opts2(
            &m_swrContext,                             // 重采样上下文指针
            &outChLayout,                              // 输出声道布局
            outputSampleFormat,                        // 输出采样格式
            outputSampleRate,                          // 输出采样率
            &inChLayout,                               // 输入声道布局
            m_codecContext->sample_fmt,                // 输入采样格式
            m_codecContext->sample_rate,               // 输入采样率
            0,                                         // 日志偏移
            nullptr                                    // 日志上下文
        );
        
        if (ret < 0) {
            char errorMsg[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errorMsg, AV_ERROR_MAX_STRING_SIZE);
            qCritical() << "FFmpegDecoder: 创建重采样上下文失败，错误:" << errorMsg;
            qCritical() << "FFmpegDecoder: 错误代码:" << ret;
            emit errorOccurred(QString("创建重采样上下文失败: %1").arg(errorMsg));
            return false;
        }
        
        if (!m_swrContext) {
            qCritical() << "FFmpegDecoder: 无法创建重采样上下文";
            emit errorOccurred("无法创建重采样上下文");
            return false;
        }
        qDebug() << "FFmpegDecoder: 重采样上下文创建成功";
        
        // 初始化重采样器
        qDebug() << "FFmpegDecoder: 初始化重采样器...";
        ret = swr_init(m_swrContext);
        if (ret < 0) {
            char errorMsg[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errorMsg, AV_ERROR_MAX_STRING_SIZE);
            qCritical() << "FFmpegDecoder: 无法初始化重采样器，错误:" << errorMsg;
            qCritical() << "FFmpegDecoder: 错误代码:" << ret;
            emit errorOccurred(QString("无法初始化重采样器: %1").arg(errorMsg));
            return false;
        }
        qDebug() << "FFmpegDecoder: 重采样器初始化成功";
        
        qDebug() << "FFmpegDecoder: 重采样器设置完成";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 设置重采样器异常:" << e.what();
        return false;
    } catch (...) {
        qCritical() << "FFmpegDecoder: 设置重采样器未知异常";
        return false;
    }
}

void FFmpegDecoder::processAudioFrame(AVFrame* frame)
{
    qDebug() << "FFmpegDecoder: 处理音频帧...";
    
    if (!frame || !m_swrContext || !m_outputFrame) {
        qWarning() << "FFmpegDecoder: 音频帧处理参数无效";
        return;
    }
    
    // 检查解码状态
    if (!m_isDecoding.loadAcquire()) {
        qDebug() << "FFmpegDecoder: 未在解码中，跳过音频帧处理";
        return;
    }
    
    try {
        // 获取音频设备格式信息
        int outputSampleRate = m_audioFormat.sampleRate();
        int outputChannels = m_audioFormat.channelCount();
        AVSampleFormat outputSampleFormat;
        
        // 根据QAudioFormat确定FFmpeg采样格式
        switch (m_audioFormat.sampleFormat()) {
            case QAudioFormat::Int16:
                outputSampleFormat = AV_SAMPLE_FMT_S16;
                break;
            case QAudioFormat::Int32:
                outputSampleFormat = AV_SAMPLE_FMT_S32;
                break;
            case QAudioFormat::Float:
                outputSampleFormat = AV_SAMPLE_FMT_FLT;
                break;
            default:
                outputSampleFormat = AV_SAMPLE_FMT_S16; // 默认使用16位
                break;
        }
        
        // 计算输出帧大小
        int outSamples = av_rescale_rnd(
            swr_get_delay(m_swrContext, frame->sample_rate) + frame->nb_samples,
            outputSampleRate, frame->sample_rate, AV_ROUND_UP);
        
        qDebug() << "FFmpegDecoder: 输出帧大小:" << outSamples << "样本";
        
        // 分配输出帧缓冲区
        av_frame_unref(m_outputFrame);
        m_outputFrame->format = outputSampleFormat;
        
        // 根据输出声道数设置声道布局
        if (outputChannels == 2) {
            av_channel_layout_from_mask(&m_outputFrame->ch_layout, AV_CH_LAYOUT_STEREO);
        } else {
            av_channel_layout_from_mask(&m_outputFrame->ch_layout, AV_CH_LAYOUT_MONO);
        }
        
        m_outputFrame->sample_rate = outputSampleRate;
        m_outputFrame->nb_samples = outSamples;
        
        if (av_frame_get_buffer(m_outputFrame, 0) < 0) {
            qWarning() << "FFmpegDecoder: 无法分配输出帧缓冲区";
            return;
        }
        
        // 执行重采样
        int samples = swr_convert(m_swrContext, m_outputFrame->data, outSamples,
                                 (const uint8_t**)frame->data, frame->nb_samples);
        
        if (samples > 0) {
            qDebug() << "FFmpegDecoder: 重采样成功，样本数:" << samples;
            
            // 应用平衡控制到音频数据（仅对立体声有效）
            if (outputChannels == 2) {
                if (outputSampleFormat == AV_SAMPLE_FMT_FLT) {
                    float* audioData = (float*)m_outputFrame->data[0];
                    for (int i = 0; i < samples; ++i) {
                        float leftSample = audioData[i * 2];
                        float rightSample = audioData[i * 2 + 1];
                        
                        // 应用平衡设置
                        if (m_balance < 0) {
                            // 左声道增强
                            leftSample *= (1.0 + qAbs(m_balance));
                            rightSample *= (1.0 - qAbs(m_balance) * 0.5);
                        } else if (m_balance > 0) {
                            // 右声道增强
                            leftSample *= (1.0 - m_balance * 0.5);
                            rightSample *= (1.0 + m_balance);
                        }
                        
                        // 限制音量防止失真
                        leftSample = qBound(-1.0f, leftSample, 1.0f);
                        rightSample = qBound(-1.0f, rightSample, 1.0f);
                        
                        audioData[i * 2] = leftSample;
                        audioData[i * 2 + 1] = rightSample;
                    }
                } else if (outputSampleFormat == AV_SAMPLE_FMT_S16) {
                    int16_t* audioData = (int16_t*)m_outputFrame->data[0];
                    for (int i = 0; i < samples; ++i) {
                        int16_t leftSample = audioData[i * 2];
                        int16_t rightSample = audioData[i * 2 + 1];
                        
                        // 转换为float进行平衡处理
                        float leftFloat = leftSample / 32768.0f;
                        float rightFloat = rightSample / 32768.0f;
                        
                        // 应用平衡设置
                        if (m_balance < 0) {
                            leftFloat *= (1.0 + qAbs(m_balance));
                            rightFloat *= (1.0 - qAbs(m_balance) * 0.5);
                        } else if (m_balance > 0) {
                            leftFloat *= (1.0 - m_balance * 0.5);
                            rightFloat *= (1.0 + m_balance);
                        }
                        
                        // 限制音量防止失真
                        leftFloat = qBound(-1.0f, leftFloat, 1.0f);
                        rightFloat = qBound(-1.0f, rightFloat, 1.0f);
                        
                        // 转换回int16
                        audioData[i * 2] = static_cast<int16_t>(leftFloat * 32767.0f);
                        audioData[i * 2 + 1] = static_cast<int16_t>(rightFloat * 32767.0f);
                    }
                }
                
                qDebug() << "FFmpegDecoder: 平衡控制应用完成，平衡值:" << m_balance;
            }
            
            // 计算音频数据大小
            int bytesPerSample;
            switch (outputSampleFormat) {
                case AV_SAMPLE_FMT_S16:
                    bytesPerSample = 2;
                    break;
                case AV_SAMPLE_FMT_S32:
                    bytesPerSample = 4;
                    break;
                case AV_SAMPLE_FMT_FLT:
                    bytesPerSample = 4;
                    break;
                default:
                    bytesPerSample = 2;
                    break;
            }
            
            int dataSize = samples * outputChannels * bytesPerSample;
            QByteArray audioBytes((const char*)m_outputFrame->data[0], dataSize);
            qDebug() << "FFmpegDecoder: 音频数据写入大小:" << audioBytes.size() << "字节，采样数:" << samples << "，声道数:" << outputChannels;
            writeAudioData(audioBytes);
            
            // 计算音频电平用于VU表
            if (outputSampleFormat == AV_SAMPLE_FMT_FLT) {
                calculateLevels((float*)m_outputFrame->data[0], samples, outputChannels);
            } else if (outputSampleFormat == AV_SAMPLE_FMT_S16) {
                // 对于int16格式，需要转换为float进行计算
                QVector<float> floatSamples(samples * outputChannels);
                int16_t* int16Data = (int16_t*)m_outputFrame->data[0];
                for (int i = 0; i < samples * outputChannels; ++i) {
                    floatSamples[i] = int16Data[i] / 32768.0f;
                }
                calculateLevels(floatSamples.data(), samples, outputChannels);
            }
            qDebug() << "FFmpegDecoder: VU表电平计算完成";
        } else {
            qWarning() << "FFmpegDecoder: 重采样失败，样本数:" << samples;
        }
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 处理音频帧异常:" << e.what();
    } catch (...) {
        qCritical() << "FFmpegDecoder: 处理音频帧未知异常";
    }
}

void FFmpegDecoder::calculateLevels(const float* samples, int frameCount, int channels)
{
    if (!samples || frameCount <= 0 || channels <= 0) {
        return;
    }
    
    // 计算RMS值
    double leftRMS = 0.0;
    double rightRMS = 0.0;
    
    for (int i = 0; i < frameCount; ++i) {
        if (channels == 1) {
            // 单声道
            double sample = samples[i];
            leftRMS += sample * sample;
            rightRMS += sample * sample;
        } else if (channels >= 2) {
            // 立体声
            double leftSample = samples[i * 2];
            double rightSample = samples[i * 2 + 1];
            leftRMS += leftSample * leftSample;
            rightRMS += rightSample * rightSample;
        }
    }
    
    // 计算RMS
    leftRMS = sqrt(leftRMS / frameCount);
    rightRMS = sqrt(rightRMS / frameCount);
    
    // 限制在0-1范围内
    leftRMS = qBound(0.0, leftRMS, 1.0);
    rightRMS = qBound(0.0, rightRMS, 1.0);
    
    // 更新电平
    m_currentLevels[0] = leftRMS;
    m_currentLevels[1] = rightRMS;
    
    // 发送信号
    emit audioDataReady(m_currentLevels);
}

void FFmpegDecoder::cleanupFFmpeg()
{
    qDebug() << "FFmpegDecoder: 开始清理FFmpeg资源...";
    
    try {
        if (m_swrContext) {
            qDebug() << "FFmpegDecoder: 释放重采样上下文";
            swr_free(&m_swrContext);
            m_swrContext = nullptr;
        }
        
        if (m_codecContext) {
            qDebug() << "FFmpegDecoder: 释放编解码器上下文";
            avcodec_free_context(&m_codecContext);
            m_codecContext = nullptr;
        }
        
        if (m_formatContext) {
            qDebug() << "FFmpegDecoder: 关闭并释放格式上下文";
            avformat_close_input(&m_formatContext);
            // 注意：avformat_close_input已经释放了m_formatContext，不需要再次调用avformat_free_context
            m_formatContext = nullptr;
        }
        
        if (m_inputFrame) {
            qDebug() << "FFmpegDecoder: 释放输入帧";
            av_frame_free(&m_inputFrame);
            m_inputFrame = nullptr;
        }
        
        if (m_outputFrame) {
            qDebug() << "FFmpegDecoder: 释放输出帧";
            av_frame_free(&m_outputFrame);
            m_outputFrame = nullptr;
        }
        
        if (m_packet) {
            qDebug() << "FFmpegDecoder: 释放数据包";
            av_packet_free(&m_packet);
            m_packet = nullptr;
        }
        
        qDebug() << "FFmpegDecoder: FFmpeg资源清理完成";
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: 清理FFmpeg资源异常:" << e.what();
    } catch (...) {
        qCritical() << "FFmpegDecoder: 清理FFmpeg资源未知异常";
    }
}

void FFmpegDecoder::resetState()
{
    m_audioStreamIndex = -1;
    m_duration = 0;
    m_currentPosition = 0;
    m_isDecoding.storeRelease(0);
    m_isEndOfFile = false;
    m_currentLevels.fill(0.0);
    m_levelBuffer.clear();
}

// ==================== 音频输出方法 ====================

bool FFmpegDecoder::setupAudioOutput()
{
    qDebug() << "FFmpegDecoder: 设置音频输出";
    
    // 获取默认音频设备
    QMediaDevices mediaDevices;
    QList<QAudioDevice> audioOutputs = mediaDevices.audioOutputs();
    
    if (audioOutputs.isEmpty()) {
        qWarning() << "FFmpegDecoder: 没有找到音频输出设备";
        return false;
    }
    
    QAudioDevice defaultDevice = audioOutputs.first();
    qDebug() << "FFmpegDecoder: 使用音频设备:" << defaultDevice.description();
    
    // 使用设备首选格式，而不是强制设置
    m_audioFormat = defaultDevice.preferredFormat();
    
    qDebug() << "FFmpegDecoder: 设备首选格式 - 采样率:" << m_audioFormat.sampleRate() 
             << "，声道数:" << m_audioFormat.channelCount() 
             << "，采样格式:" << m_audioFormat.sampleFormat();
    
    // 确保格式兼容性，如果设备不支持首选格式，回退到基本格式
    if (!defaultDevice.isFormatSupported(m_audioFormat)) {
        qWarning() << "FFmpegDecoder: 设备不支持首选格式，使用基本格式";
        m_audioFormat.setSampleRate(44100);
        m_audioFormat.setChannelCount(2);
        m_audioFormat.setSampleFormat(QAudioFormat::Int16);
        
        // 再次检查基本格式是否支持
        if (!defaultDevice.isFormatSupported(m_audioFormat)) {
            qCritical() << "FFmpegDecoder: 设备不支持基本音频格式";
            return false;
        }
    }
    
    qDebug() << "FFmpegDecoder: 最终使用音频格式 - 采样率:" << m_audioFormat.sampleRate() 
             << "，声道数:" << m_audioFormat.channelCount() 
             << "，采样格式:" << m_audioFormat.sampleFormat();
    
    // 创建音频输出
    m_audioSink = new QAudioSink(defaultDevice, m_audioFormat, this);
    
    if (!m_audioSink) {
        qWarning() << "FFmpegDecoder: 无法创建音频输出";
        return false;
    }
    
    // 设置缓冲区大小（32KB）
    m_audioSink->setBufferSize(32768);
    qDebug() << "FFmpegDecoder: 设置音频缓冲区大小:" << m_audioSink->bufferSize() << "字节";
    
    // 设置音量
    m_audioSink->setVolume(1.0);
    
    // 获取音频设备
    m_audioDevice = m_audioSink->start();
    
    if (!m_audioDevice) {
        qWarning() << "FFmpegDecoder: 无法启动音频设备";
        return false;
    }
    
    qDebug() << "FFmpegDecoder: 音频输出设置成功";
    return true;
}

void FFmpegDecoder::cleanupAudioOutput()
{
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }
    
    m_audioDevice = nullptr;
    m_audioBuffer.clear();
}

void FFmpegDecoder::writeAudioData(const QByteArray& data)
{
    // 检查音频设备和解码状态
    if (!m_audioDevice || !m_isDecoding.loadAcquire()) {
        return;
    }
    
    // 直接写入音频数据，不使用异步
    qint64 written = m_audioDevice->write(data);
    
    // 检查写入是否成功
    if (written != data.size()) {
        qWarning() << "FFmpegDecoder: 音频数据写入不完整:" << written << "/" << data.size() << "字节";
        
        // 如果写入不完整，尝试分块写入
        if (written > 0) {
            QByteArray remainingData = data.mid(written);
            qint64 remainingWritten = m_audioDevice->write(remainingData);
            if (remainingWritten != remainingData.size()) {
                qWarning() << "FFmpegDecoder: 剩余数据写入失败:" << remainingWritten << "/" << remainingData.size() << "字节";
            }
        }
    } else {
        qDebug() << "FFmpegDecoder: 音频数据写入成功:" << written << "字节";
    }
} 