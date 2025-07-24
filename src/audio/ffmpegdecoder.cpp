#include "ffmpegdecoder.h"
#include <QFileInfo>
#include <QtMath>
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
            m_decodeTimer->start(10); // 10ms间隔，100fps
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
        
        if (m_formatContext->duration != AV_NOPTS_VALUE) {
            m_duration = m_formatContext->duration / 1000; // 转换为毫秒
        } else {
            m_duration = 0;
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
            return false;
        }
        
        if (m_isDecoding.loadAcquire()) {
            return true;
        }
        
        m_isDecoding.storeRelease(1);
        m_isEndOfFile = false;
        
        if (m_decodeThread && !m_decodeThread->isRunning()) {
            m_decodeThread->start();
        }
        
        return true;
        
    } catch (const std::exception& e) {
        return false;
    } catch (...) {
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
        
        m_isDecoding.storeRelease(0);
        
        if (m_decodeThread && m_decodeThread->isRunning()) {
            m_decodeThread->quit();
            m_decodeThread->wait();
        }
        
    } catch (const std::exception& e) {
    } catch (...) {
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
        
        int64_t timestamp = position * 1000; // 转换为微秒
        int ret = av_seek_frame(m_formatContext, m_audioStreamIndex, timestamp, AVSEEK_FLAG_BACKWARD);
        if (ret >= 0) {
            if (m_codecContext) {
                avcodec_flush_buffers(m_codecContext);
            }
            
            m_currentPosition = position;
            emit positionChanged(m_currentPosition);
        } else {
            char errorBuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(ret, errorBuf, AV_ERROR_MAX_STRING_SIZE);
        }
        
    } catch (const std::exception& e) {
    } catch (...) {
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
    
    {
        QMutexLocker locker(&m_mutex);
        
        if (!m_formatContext || !m_codecContext) {
            return;
        }
        
        int ret = av_read_frame(m_formatContext, m_packet);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                m_isEndOfFile = true;
                emit decodingFinished();
            }
            return;
        }
        
        if (m_packet->stream_index != m_audioStreamIndex) {
            av_packet_unref(m_packet);
            return;
        }
        
        ret = avcodec_send_packet(m_codecContext, m_packet);
        if (ret < 0) {
            av_packet_unref(m_packet);
            return;
        }
        
        int frameCount = 0;
        while (true) {
            ret = avcodec_receive_frame(m_codecContext, m_inputFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break;
            } else if (ret < 0) {
                break;
            }
            
            frameCount++;
            
            AVFrame* frameCopy = av_frame_alloc();
            av_frame_ref(frameCopy, m_inputFrame);
            
            if (m_inputFrame->pts != AV_NOPTS_VALUE) {
                qint64 newPosition = m_inputFrame->pts * av_q2d(m_formatContext->streams[m_audioStreamIndex]->time_base) * 1000;
                if (newPosition != m_currentPosition) {
                    m_currentPosition = newPosition;
                    emit positionChanged(m_currentPosition);
                }
            }
            
            locker.unlock();
            processAudioFrame(frameCopy);
            av_frame_free(&frameCopy);
            locker.relock();
        }
        
        if (frameCount > 0 && m_audioFormat.sampleRate() > 0) {
            int samplesPerFrame = frameCount * m_audioFormat.channelCount();
            int delayMs = (samplesPerFrame * 1000) / m_audioFormat.sampleRate();
            if (delayMs > 0 && delayMs < 50) {
                QThread::msleep(delayMs);
            }
        }
        
        av_packet_unref(m_packet);
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
    QMediaDevices mediaDevices;
    QList<QAudioDevice> audioOutputs = mediaDevices.audioOutputs();
    
    if (audioOutputs.isEmpty()) {
        return false;
    }
    
    QAudioDevice defaultDevice = audioOutputs.first();
    m_audioFormat = defaultDevice.preferredFormat();
    
    if (!defaultDevice.isFormatSupported(m_audioFormat)) {
        m_audioFormat.setSampleRate(44100);
        m_audioFormat.setChannelCount(2);
        m_audioFormat.setSampleFormat(QAudioFormat::Int16);
        
        if (!defaultDevice.isFormatSupported(m_audioFormat)) {
            return false;
        }
    }
    
    m_audioSink = new QAudioSink(defaultDevice, m_audioFormat, this);
    
    if (!m_audioSink) {
        return false;
    }
    
    m_audioSink->setBufferSize(65536);
    m_audioSink->setVolume(1.0);
    
    m_audioDevice = m_audioSink->start();
    
    if (!m_audioDevice) {
        return false;
    }
    
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