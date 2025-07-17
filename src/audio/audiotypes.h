#ifndef AUDIOTYPES_H
#define AUDIOTYPES_H

#include <QMetaType>
#include <QString>
#include <QStringList>

namespace AudioTypes {

/**
 * @brief 音频播放状态枚举
 */
enum class AudioState {
    Stopped,    ///< 停止状态
    Playing,    ///< 播放状态
    Paused,     ///< 暂停状态
    Loading,    ///< 加载状态
    Error       ///< 错误状态
};

/**
 * @brief 播放模式枚举
 */
enum class PlayMode {
    Loop,           ///< 列表循环
    Random,         ///< 随机播放
    RepeatOne       ///< 单曲循环
};

/**
 * @brief 缓冲状态枚举（替代 Qt6 中已移除的 QMediaPlayer::BufferStatus）
 */
enum class BufferStatus {
    Empty,      ///< 缓冲区为空
    Buffering,  ///< 正在缓冲
    Buffered    ///< 缓冲完成
};

/**
 * @brief 音频格式枚举
 */
enum class AudioFormat {
    Unknown,    ///< 未知格式
    MP3,        ///< MP3格式
    FLAC,       ///< FLAC格式
    WAV,        ///< WAV格式
    OGG,        ///< OGG格式
    AAC,        ///< AAC格式
    WMA,        ///< WMA格式
    ALAC,       ///< ALAC格式
    APE,        ///< APE格式
    M4A         ///< M4A格式
};

/**
 * @brief 音频质量枚举
 */
enum class AudioQuality {
    Low,        ///< 低质量
    Medium,     ///< 中等质量
    High,       ///< 高质量
    Lossless    ///< 无损质量
};

/**
 * @brief 均衡器预设枚举
 */
enum class EqualizerPreset {
    Default,    ///< 默认
    Pop,        ///< 流行
    Rock,       ///< 摇滚
    Jazz,       ///< 爵士
    Classical,  ///< 古典
    Electronic, ///< 电子
    Bass,       ///< 重低音
    Treble,     ///< 高音
    Vocal,      ///< 人声
    Custom      ///< 自定义
};

/**
 * @brief 音频错误类型枚举
 */
enum class AudioError {
    NoError,            ///< 无错误
    FileNotFound,       ///< 文件未找到
    FormatNotSupported, ///< 格式不支持
    DecodingError,      ///< 解码错误
    DeviceError,        ///< 设备错误
    NetworkError,       ///< 网络错误
    PermissionError,    ///< 权限错误
    UnknownError        ///< 未知错误
};

/**
 * @brief 音频输出设备类型枚举
 */
enum class AudioDevice {
    Default,        ///< 默认设备
    Speakers,       ///< 扬声器
    Headphones,     ///< 耳机
    Bluetooth,      ///< 蓝牙设备
    USB,            ///< USB设备
    HDMI            ///< HDMI设备
};

/**
 * @brief 音频分析类型枚举
 */
enum class AnalysisType {
    None,           ///< 无分析
    Spectrum,       ///< 频谱分析
    Waveform,       ///< 波形分析
    VU,             ///< VU表
    Peak,           ///< 峰值检测
    All             ///< 全部分析
};

/**
 * @brief 音频采样率枚举
 */
enum class SampleRate {
    Rate8000 = 8000,
    Rate11025 = 11025,
    Rate16000 = 16000,
    Rate22050 = 22050,
    Rate32000 = 32000,
    Rate44100 = 44100,
    Rate48000 = 48000,
    Rate88200 = 88200,
    Rate96000 = 96000,
    Rate176400 = 176400,
    Rate192000 = 192000
};

/**
 * @brief 音频比特率枚举
 */
enum class BitRate {
    Rate32 = 32,
    Rate64 = 64,
    Rate96 = 96,
    Rate128 = 128,
    Rate160 = 160,
    Rate192 = 192,
    Rate224 = 224,
    Rate256 = 256,
    Rate320 = 320,
    RateVariable = -1
};

/**
 * @brief 音频通道枚举
 */
enum class AudioChannel {
    Mono = 1,       ///< 单声道
    Stereo = 2,     ///< 立体声
    Surround = 6,   ///< 5.1环绕声
    Surround7 = 8   ///< 7.1环绕声
};

/**
 * @brief 音频状态转换为字符串
 * @param state 音频状态
 * @return 状态字符串
 */
inline QString audioStateToString(AudioState state) {
    switch (state) {
        case AudioState::Stopped: return "Stopped";
        case AudioState::Playing: return "Playing";
        case AudioState::Paused: return "Paused";
        case AudioState::Loading: return "Loading";
        case AudioState::Error: return "Error";
        default: return "Unknown";
    }
}

/**
 * @brief 播放模式转换为字符串
 * @param mode 播放模式
 * @return 模式字符串
 */
inline QString playModeToString(PlayMode mode) {
    switch (mode) {
        case PlayMode::Loop: return "Loop";
        case PlayMode::Random: return "Random";
        case PlayMode::RepeatOne: return "RepeatOne";
        default: return "Loop";
    }
}

/**
 * @brief 缓冲状态转换为字符串
 * @param status 缓冲状态
 * @return 状态字符串
 */
inline QString bufferStatusToString(BufferStatus status) {
    switch (status) {
        case BufferStatus::Empty: return "Empty";
        case BufferStatus::Buffering: return "Buffering";
        case BufferStatus::Buffered: return "Buffered";
        default: return "Unknown";
    }
}

/**
 * @brief 音频格式转换为字符串
 * @param format 音频格式
 * @return 格式字符串
 */
inline QString audioFormatToString(AudioFormat format) {
    switch (format) {
        case AudioFormat::MP3: return "MP3";
        case AudioFormat::FLAC: return "FLAC";
        case AudioFormat::WAV: return "WAV";
        case AudioFormat::OGG: return "OGG";
        case AudioFormat::AAC: return "AAC";
        case AudioFormat::WMA: return "WMA";
        case AudioFormat::ALAC: return "ALAC";
        case AudioFormat::APE: return "APE";
        case AudioFormat::M4A: return "M4A";
        default: return "Unknown";
    }
}

/**
 * @brief 从文件扩展名获取音频格式
 * @param extension 文件扩展名
 * @return 音频格式
 */
inline AudioFormat audioFormatFromExtension(const QString& extension) {
    QString ext = extension.toLower();
    if (ext == "mp3") return AudioFormat::MP3;
    if (ext == "flac") return AudioFormat::FLAC;
    if (ext == "wav") return AudioFormat::WAV;
    if (ext == "ogg") return AudioFormat::OGG;
    if (ext == "aac") return AudioFormat::AAC;
    if (ext == "wma") return AudioFormat::WMA;
    if (ext == "alac") return AudioFormat::ALAC;
    if (ext == "ape") return AudioFormat::APE;
    if (ext == "m4a") return AudioFormat::M4A;
    return AudioFormat::Unknown;
}

/**
 * @brief 获取支持的音频格式扩展名列表
 * @return 扩展名列表
 */
inline QStringList supportedAudioExtensions() {
    return {"mp3", "flac", "wav", "ogg", "aac", "wma", "alac", "ape", "m4a"};
}

/**
 * @brief 获取支持的音频格式过滤器
 * @return 过滤器字符串
 */
inline QString supportedAudioFilter() {
    return "Audio Files (*.mp3 *.flac *.wav *.ogg *.aac *.wma *.alac *.ape *.m4a)";
}

} // namespace AudioTypes

// 注册元类型以便在信号槽中使用
Q_DECLARE_METATYPE(AudioTypes::AudioState)
Q_DECLARE_METATYPE(AudioTypes::PlayMode)
Q_DECLARE_METATYPE(AudioTypes::BufferStatus)
Q_DECLARE_METATYPE(AudioTypes::AudioFormat)
Q_DECLARE_METATYPE(AudioTypes::AudioQuality)
Q_DECLARE_METATYPE(AudioTypes::EqualizerPreset)
Q_DECLARE_METATYPE(AudioTypes::AudioError)
Q_DECLARE_METATYPE(AudioTypes::AudioDevice)
Q_DECLARE_METATYPE(AudioTypes::AnalysisType)
Q_DECLARE_METATYPE(AudioTypes::SampleRate)
Q_DECLARE_METATYPE(AudioTypes::BitRate)
Q_DECLARE_METATYPE(AudioTypes::AudioChannel)

#endif // AUDIOTYPES_H