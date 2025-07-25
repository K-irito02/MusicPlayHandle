#include "ffmpegdecoder.h"
#include <QFileInfo>
#include <QtMath>
#include <QDateTime>
#include <cstdint>

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
}

FFmpegDecoder::~FFmpegDecoder()
{
    cleanup();
}

bool FFmpegDecoder::initialize()
{
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
        m_audioBuffer.clear();
        
        m_decodeThread = new QThread(this);
        m_decodeTimer = new QTimer();
        m_decodeTimer->moveToThread(m_decodeThread);
        
        connect(m_decodeTimer, &QTimer::timeout, this, &FFmpegDecoder::decodeLoop);
        connect(m_decodeThread, &QThread::started, [this]() {
            // 初始解码间隔，后续将通过性能管理器动态调整
    m_currentDecodeInterval = 20; // 默认20ms
    m_decodeTimer->start(m_currentDecodeInterval);
    
    qDebug() << "FFmpegDecoder: 开始解码，初始间隔:" << m_currentDecodeInterval << "ms";
        });
        return true;
        
    } catch (const std::exception& e) {
        return false;
    } catch (...) {
        return false;
    }
}

void FFmpegDecoder::cleanup()
{
    stopDecoding();
    closeFile();
    
    if (m_decodeTimer) {
        m_decodeTimer->stop();
        delete m_decodeTimer;
        m_decodeTimer = nullptr;
    }
    
    if (m_decodeThread) {
        m_decodeThread->quit();
        m_decodeThread->wait();
        delete m_decodeThread;
        m_decodeThread = nullptr;
    }
    
    cleanupFFmpeg();
    cleanupAudioOutput();
}

bool FFmpegDecoder::openFile(const QString& filePath)
{
    try {
        closeFile();
        
        QMutexLocker locker(&m_mutex);
        
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            emit errorOccurred("文件不存在: " + filePath);
            return false;
        }
        
        m_formatContext = avformat_alloc_context();
        if (!m_formatContext) {
            emit errorOccurred("无法分配格式上下文");
            return false;
        }
        
        if (avformat_open_input(&m_formatContext, filePath.toUtf8().constData(), nullptr, nullptr) < 0) {
            emit errorOccurred("无法打开输入文件");
            cleanupFFmpeg();
            return false;
        }
        
        if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
            emit errorOccurred("无法查找流信息");
            cleanupFFmpeg();
            return false;
        }
        
        m_audioStreamIndex = -1;
        for (unsigned int i = 0; i < m_formatContext->nb_streams; i++) {
            if (m_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                m_audioStreamIndex = i;
                break;
            }
        }
        
        if (m_audioStreamIndex < 0) {
            emit errorOccurred("未找到音频流");
            cleanupFFmpeg();
            return false;
        }
        
        if (!setupCodec()) {
            cleanupFFmpeg();
            return false;
        }
        
        if (!setupAudioOutput()) {
            cleanupFFmpeg();
            return false;
        }
        
        if (!setupResampler()) {
            cleanupFFmpeg();
            return false;
        }
        
        if (m_formatContext) {
            m_duration = m_formatContext->duration * 1000 / AV_TIME_BASE;
            // 使用QueuedConnection确保信号在主线程中处理
            QMetaObject::invokeMethod(this, [this]() {
                emit durationChanged(m_duration);
            }, Qt::QueuedConnection);
        }
        
        m_duration = 0;
        m_currentPosition = 0;
        m_isDecoding.storeRelease(0);
        m_isEndOfFile = false;
        m_currentLevels.fill(0.0);
        m_levelBuffer.clear();
    
        emit durationChanged(m_duration);
        return true;
        
    } catch (const std::exception& e) {
        cleanupFFmpeg();
        return false;
    } catch (...) {
        cleanupFFmpeg();
        return false;
    }
}

void FFmpegDecoder::closeFile()
{
    try {
        QMutexLocker locker(&m_mutex);
        
        if (m_isDecoding.loadAcquire()) {
            m_isDecoding.storeRelease(0);
        }
        
        cleanupFFmpeg();
        cleanupAudioOutput();
        resetState();
        
    } catch (const std::exception& e) {
    } catch (...) {
    }
}

bool FFmpegDecoder::startDecoding()
{
    try {
        QMutexLocker locker(&m_mutex);
        
        if (!m_formatContext) {
            qWarning() << "FFmpegDecoder: 格式上下文未初始化";
            return false;
        }
        
        if (m_isDecoding.loadAcquire()) {
            qDebug() << "FFmpegDecoder: 已经在解码中";
            return true;
        }
        
        // 确保音频输出设备已正确设置
        if (!m_audioSink || !m_audioDevice) {
            qWarning() << "FFmpegDecoder: 音频输出设备未初始化，尝试重新设置";
            if (!setupAudioOutput()) {
                qCritical() << "FFmpegDecoder: 无法设置音频输出设备";
                return false;
            }
        }
        
        m_isDecoding.storeRelease(1);
        m_isEndOfFile = false;
        
        // 确保解码线程安全启动
        if (m_decodeThread) {
            if (!m_decodeThread->isRunning()) {
                m_decodeThread->start();
                qDebug() << "FFmpegDecoder: 解码线程已启动";
            }
        } else {
            qCritical() << "FFmpegDecoder: 解码线程未初始化";
            m_isDecoding.storeRelease(0);
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: startDecoding异常:" << e.what();
        m_isDecoding.storeRelease(0);
        return false;
    } catch (...) {
        qCritical() << "FFmpegDecoder: startDecoding未知异常";
        m_isDecoding.storeRelease(0);
        return false;
    }
}

void FFmpegDecoder::stopDecoding()
{
    try {
        QMutexLocker locker(&m_mutex);
        
        if (!m_isDecoding.loadAcquire()) {
            return;
        }
        
        qDebug() << "FFmpegDecoder: 停止解码";
        m_isDecoding.storeRelease(0);
        
        // 安全停止解码线程
        if (m_decodeThread && m_decodeThread->isRunning()) {
            m_decodeThread->quit();
            if (!m_decodeThread->wait(3000)) { // 等待3秒
                qWarning() << "FFmpegDecoder: 解码线程未能在3秒内停止，强制终止";
                m_decodeThread->terminate();
                m_decodeThread->wait();
            }
        }
        
        // 停止音频输出
        if (m_audioSink) {
            m_audioSink->stop();
        }
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: stopDecoding异常:" << e.what();
    } catch (...) {
        qCritical() << "FFmpegDecoder: stopDecoding未知异常";
    }
}

void FFmpegDecoder::seekTo(qint64 position)
{
    try {
        QMutexLocker locker(&m_mutex);
        
        if (!m_formatContext) {
            return;
        }
        
        m_levelBuffer.clear();
        
        if (m_formatContext && m_codecContext) {
            int64_t targetTime = position * AV_TIME_BASE / 1000;
            if (av_seek_frame(m_formatContext, -1, targetTime, AVSEEK_FLAG_BACKWARD) >= 0) {
                avcodec_flush_buffers(m_codecContext);
                m_currentPosition = position;
                // 使用QueuedConnection确保信号在主线程中处理
                QMetaObject::invokeMethod(this, [this, position]() {
                    emit positionChanged(position);
                }, Qt::QueuedConnection);
            }
        }
        
    } catch (const std::exception& e) {
        qWarning() << "FFmpegDecoder: seekTo异常:" << e.what();
    } catch (...) {
        qWarning() << "FFmpegDecoder: seekTo未知异常";
    }
}

QVector<double> FFmpegDecoder::getCurrentLevels() const
{
    QMutexLocker locker(&m_mutex);
    return m_currentLevels;
}

void FFmpegDecoder::setBalance(double balance)
{
    try {
        QMutexLocker locker(&m_mutex);
        m_balance = qBound(-1.0, balance, 1.0);
    } catch (const std::exception& e) {
    } catch (...) {
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
    if (!m_isDecoding.loadAcquire()) {
        return;
    }
    
    try {
        // 减少锁的持有时间，只在必要时加锁
        QMutexLocker locker(&m_mutex);
        
        if (!m_formatContext || !m_codecContext) {
            qWarning() << "FFmpegDecoder: 解码器上下文未初始化";
            return;
        }
        
        if (!m_packet) {
            qWarning() << "FFmpegDecoder: 数据包未初始化";
            return;
        }
        
        // 临时释放锁，避免长时间持有
        locker.unlock();
        
        int ret = av_read_frame(m_formatContext, m_packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                locker.relock();
                m_isEndOfFile = true;
                qDebug() << "FFmpegDecoder: 到达文件末尾";
                // 使用QueuedConnection确保信号在主线程中处理
                QMetaObject::invokeMethod(this, [this]() {
                    emit decodingFinished();
                }, Qt::QueuedConnection);
            } else {
                char errorMsg[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, errorMsg, AV_ERROR_MAX_STRING_SIZE);
                qWarning() << "FFmpegDecoder: 读取帧失败:" << errorMsg;
            }
            return;
        }
        
        if (m_packet->stream_index != m_audioStreamIndex) {
            av_packet_unref(m_packet);
            return;
        }
        
        ret = avcodec_send_packet(m_codecContext, m_packet);
        if (ret < 0) {
            char errorMsg[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errorMsg, AV_ERROR_MAX_STRING_SIZE);
            qWarning() << "FFmpegDecoder: 发送数据包失败:" << errorMsg;
            av_packet_unref(m_packet);
            return;
        }
        
        int frameCount = 0;
        while (m_isDecoding.loadAcquire()) {
            ret = avcodec_receive_frame(m_codecContext, m_inputFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                char errorMsg[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(ret, errorMsg, AV_ERROR_MAX_STRING_SIZE);
                qWarning() << "FFmpegDecoder: 接收帧失败:" << errorMsg;
                break;
            }
            
            frameCount++;
            
            AVFrame* frameCopy = av_frame_alloc();
            if (!frameCopy) {
                qWarning() << "FFmpegDecoder: 无法分配帧副本";
                break;
            }
            
            ret = av_frame_ref(frameCopy, m_inputFrame);
            if (ret < 0) {
                qWarning() << "FFmpegDecoder: 无法引用帧";
                av_frame_free(&frameCopy);
                break;
            }
            
            // 更新位置信息
            if (m_inputFrame->pts != AV_NOPTS_VALUE) {
                qint64 newPosition = m_inputFrame->pts * av_q2d(m_formatContext->streams[m_audioStreamIndex]->time_base) * 1000;
                if (newPosition != m_currentPosition) {
                    m_currentPosition = newPosition;
                    // 使用QueuedConnection确保信号在主线程中处理，并限制发送频率
                    static qint64 lastEmitTime = 0;
                    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
                    if (currentTime - lastEmitTime > 50) { // 改为50ms间隔，确保进度条流畅更新
                        lastEmitTime = currentTime;
                        QMetaObject::invokeMethod(this, [this, newPosition]() {
                            emit positionChanged(newPosition);
                        }, Qt::QueuedConnection);
                    }
                }
            }
            
            // 处理音频帧
            processAudioFrame(frameCopy);
            av_frame_free(&frameCopy);
            
            // 限制每轮解码的帧数，避免长时间占用CPU
            if (frameCount >= 5) {
                break;
            }
        }
        
        // 添加适当的延迟，避免过度占用CPU
        if (frameCount > 0) {
            QThread::msleep(5); // 减少到5ms延迟，提高响应性
        }
        
        av_packet_unref(m_packet);
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: decodeLoop异常:" << e.what();
    } catch (...) {
        qCritical() << "FFmpegDecoder: decodeLoop未知异常";
    }
}

bool FFmpegDecoder::setupCodec()
{
    try {
        const AVCodecParameters* codecParams = m_formatContext->streams[m_audioStreamIndex]->codecpar;
        const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
        if (!codec) {
            emit errorOccurred("不支持的音频编解码器");
            return false;
        }
        
        m_codecContext = avcodec_alloc_context3(codec);
        if (!m_codecContext) {
            emit errorOccurred("无法分配解码器上下文");
            return false;
        }
        
        if (avcodec_parameters_to_context(m_codecContext, codecParams) < 0) {
            emit errorOccurred("无法复制编解码器参数");
            return false;
        }
        
        if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
            emit errorOccurred("无法打开解码器");
            return false;
        }
        
        m_inputFrame = av_frame_alloc();
        m_outputFrame = av_frame_alloc();
        m_packet = av_packet_alloc();
        
        if (!m_inputFrame || !m_outputFrame || !m_packet) {
            emit errorOccurred("无法分配帧或数据包");
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    } catch (...) {
        return false;
    }
}

bool FFmpegDecoder::setupResampler()
{
    try {
        if (m_codecContext->ch_layout.nb_channels <= 0) {
            emit errorOccurred("无效的声道数");
            return false;
        }
        
        if (m_codecContext->sample_rate <= 0) {
            emit errorOccurred("无效的采样率");
            return false;
        }
        
        int outputSampleRate = m_audioFormat.sampleRate();
        int outputChannels = m_audioFormat.channelCount();
        AVSampleFormat outputSampleFormat;
        
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
                outputSampleFormat = AV_SAMPLE_FMT_S16;
                break;
        }
        
        if (m_swrContext) {
            swr_free(&m_swrContext);
        }
        
        AVChannelLayout outChLayout, inChLayout;
        
        if (outputChannels == 2) {
            av_channel_layout_default(&outChLayout, 2);
        } else {
            av_channel_layout_default(&outChLayout, 1);
        }
        
        av_channel_layout_copy(&inChLayout, &m_codecContext->ch_layout);
        
        int ret = swr_alloc_set_opts2(
            &m_swrContext,
            &outChLayout,
            outputSampleFormat,
            outputSampleRate,
            &inChLayout,
            m_codecContext->sample_fmt,
            m_codecContext->sample_rate,
            0,
            nullptr
        );
        
        if (ret < 0) {
            char errorMsg[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errorMsg, AV_ERROR_MAX_STRING_SIZE);
            emit errorOccurred(QString("创建重采样上下文失败: %1").arg(errorMsg));
            return false;
        }
        
        if (!m_swrContext) {
            emit errorOccurred("无法创建重采样上下文");
            return false;
        }
        
        ret = swr_init(m_swrContext);
        if (ret < 0) {
            char errorMsg[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errorMsg, AV_ERROR_MAX_STRING_SIZE);
            emit errorOccurred(QString("无法初始化重采样器: %1").arg(errorMsg));
            return false;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    } catch (...) {
        return false;
    }
}

void FFmpegDecoder::processAudioFrame(AVFrame* frame)
{
    if (!frame || !m_swrContext || !m_outputFrame) {
        return;
    }
    
    if (!m_isDecoding.loadAcquire()) {
        return;
    }
    
    try {
        int outputSampleRate = m_audioFormat.sampleRate();
        int outputChannels = m_audioFormat.channelCount();
        AVSampleFormat outputSampleFormat;
        
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
                outputSampleFormat = AV_SAMPLE_FMT_S16;
                break;
        }
        
        int outSamples = av_rescale_rnd(
            swr_get_delay(m_swrContext, frame->sample_rate) + frame->nb_samples,
            outputSampleRate, frame->sample_rate, AV_ROUND_UP);
        
        av_frame_unref(m_outputFrame);
        m_outputFrame->format = outputSampleFormat;
        
        if (outputChannels == 2) {
            av_channel_layout_from_mask(&m_outputFrame->ch_layout, AV_CH_LAYOUT_STEREO);
        } else {
            av_channel_layout_from_mask(&m_outputFrame->ch_layout, AV_CH_LAYOUT_MONO);
        }
        
        m_outputFrame->sample_rate = outputSampleRate;
        m_outputFrame->nb_samples = outSamples;
        
        if (av_frame_get_buffer(m_outputFrame, 0) < 0) {
            return;
        }
        
        // 执行重采样
        int samples = swr_convert(m_swrContext, m_outputFrame->data, outSamples,
                                 (const uint8_t**)frame->data, frame->nb_samples);
        
        if (samples > 0) {     
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
    
    // 限制VU表信号发送频率
    static qint64 lastVUEmitTime = 0;
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - lastVUEmitTime > 100) { // 改为100ms间隔，确保VU表正常显示
        lastVUEmitTime = currentTime;
        
        m_currentLevels[0] = leftRMS;
        m_currentLevels[1] = rightRMS;
        
        // 发送音频数据信号
        QMetaObject::invokeMethod(this, [this]() {
            emit audioDataReady(m_currentLevels);
        }, Qt::QueuedConnection);
    }
}

void FFmpegDecoder::cleanupFFmpeg()
{
    try {
        if (m_swrContext) {
            swr_free(&m_swrContext);
            m_swrContext = nullptr;
        }
        
        if (m_codecContext) {
            avcodec_free_context(&m_codecContext);
            m_codecContext = nullptr;
        }
        
        if (m_formatContext) {
            avformat_close_input(&m_formatContext);
            m_formatContext = nullptr;
        }
        
        if (m_inputFrame) {
            av_frame_free(&m_inputFrame);
            m_inputFrame = nullptr;
        }
        
        if (m_outputFrame) {
            av_frame_free(&m_outputFrame);
            m_outputFrame = nullptr;
        }
        
        if (m_packet) {
            av_packet_free(&m_packet);
            m_packet = nullptr;
        }
        
    } catch (const std::exception& e) {
    } catch (...) {
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
    try {
        // 先清理旧的音频输出
        cleanupAudioOutput();
        
        QMediaDevices mediaDevices;
        QList<QAudioDevice> audioOutputs = mediaDevices.audioOutputs();
        
        if (audioOutputs.isEmpty()) {
            qCritical() << "FFmpegDecoder: 没有可用的音频输出设备";
            return false;
        }
        
        QAudioDevice defaultDevice = audioOutputs.first();
        qDebug() << "FFmpegDecoder: 使用音频设备:" << defaultDevice.description();
        
        m_audioFormat = defaultDevice.preferredFormat();
        
        if (!defaultDevice.isFormatSupported(m_audioFormat)) {
            qWarning() << "FFmpegDecoder: 首选格式不支持，使用默认格式";
            m_audioFormat.setSampleRate(44100);
            m_audioFormat.setChannelCount(2);
            m_audioFormat.setSampleFormat(QAudioFormat::Int16);
            
            if (!defaultDevice.isFormatSupported(m_audioFormat)) {
                qCritical() << "FFmpegDecoder: 默认格式也不支持";
                return false;
            }
        }
        
        qDebug() << "FFmpegDecoder: 音频格式 - 采样率:" << m_audioFormat.sampleRate() 
                 << "声道数:" << m_audioFormat.channelCount() 
                 << "格式:" << m_audioFormat.sampleFormat();
        
        m_audioSink = new QAudioSink(defaultDevice, m_audioFormat, this);
        
        if (!m_audioSink) {
            qCritical() << "FFmpegDecoder: 无法创建QAudioSink";
            return false;
        }
        
        m_audioSink->setBufferSize(65536);
        m_audioSink->setVolume(1.0);
        
        m_audioDevice = m_audioSink->start();
        
        if (!m_audioDevice) {
            qCritical() << "FFmpegDecoder: 无法启动音频设备";
            delete m_audioSink;
            m_audioSink = nullptr;
            return false;
        }
        
        qDebug() << "FFmpegDecoder: 音频输出设置成功";
        return true;
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: setupAudioOutput异常:" << e.what();
        cleanupAudioOutput();
        return false;
    } catch (...) {
        qCritical() << "FFmpegDecoder: setupAudioOutput未知异常";
        cleanupAudioOutput();
        return false;
    }
}

void FFmpegDecoder::cleanupAudioOutput()
{
    try {
        if (m_audioSink) {
            m_audioSink->stop();
            delete m_audioSink;
            m_audioSink = nullptr;
        }
        
        m_audioDevice = nullptr;
        m_audioBuffer.clear();
        
        qDebug() << "FFmpegDecoder: 音频输出清理完成";
        
    } catch (const std::exception& e) {
        qCritical() << "FFmpegDecoder: cleanupAudioOutput异常:" << e.what();
    } catch (...) {
        qCritical() << "FFmpegDecoder: cleanupAudioOutput未知异常";
    }
}

void FFmpegDecoder::writeAudioData(const QByteArray& data)
{
    if (!m_audioDevice || !m_isDecoding.loadAcquire()) {
        return;
    }
    
    QByteArray remainingData = data;
    int maxRetries = 10;
    int retryCount = 0;
    
    while (!remainingData.isEmpty() && retryCount < maxRetries) {
        if (m_audioSink->state() == QAudio::StoppedState) {
            break;
        }
        
        qint64 written = m_audioDevice->write(remainingData);
        
        if (written > 0) {
            remainingData = remainingData.mid(written);
            retryCount = 0;
        } else {
            retryCount++;
            if (retryCount < maxRetries) {
                QThread::msleep(1);
            }
        }
    }
    
    if (!remainingData.isEmpty()) {
        if (m_audioBuffer.size() < 131072) {
            m_audioBuffer.append(remainingData);
        }
    }
    
    if (!m_audioBuffer.isEmpty()) {
        QByteArray bufferData = m_audioBuffer;
        m_audioBuffer.clear();
        writeAudioData(bufferData);
    }
}