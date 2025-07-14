#ifndef TAGSTRINGS_H
#define TAGSTRINGS_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QLocale>
#include <QTranslator>
#include <QCoreApplication>
#include <QMutex>

/**
 * @brief 标签相关字符串国际化管理类
 * @details 提供标签模块的所有用户界面字符串的国际化支持
 */
class TagStrings : public QObject
{
    Q_OBJECT
    
public:
    explicit TagStrings(QObject* parent = nullptr);
    ~TagStrings();
    
    // 单例模式
    static TagStrings* instance();
    static void cleanup();
    
    /**
     * @brief 初始化国际化系统
     * @param locale 语言区域设置
     * @param translationPath 翻译文件路径
     */
    void initialize(const QLocale& locale = QLocale::system(),
                   const QString& translationPath = QString());
    
    /**
     * @brief 切换语言
     * @param locale 新的语言区域设置
     * @return 是否切换成功
     */
    bool switchLanguage(const QLocale& locale);
    
    /**
     * @brief 获取当前语言
     * @return 当前语言区域设置
     */
    QLocale currentLocale() const;
    
    /**
     * @brief 获取支持的语言列表
     * @return 支持的语言列表
     */
    QList<QLocale> supportedLocales() const;
    
    // ========== 系统标签相关字符串 ==========
    
    /**
     * @brief 系统标签不能编辑
     */
    static QString systemTagCannotEdit() {
        return tr("系统标签不能编辑");
    }
    
    /**
     * @brief 系统标签不能删除
     */
    static QString systemTagCannotDelete() {
        return tr("系统标签不能删除");
    }
    
    /**
     * @brief 我的歌曲
     */
    static QString mySongs() {
        return tr("我的歌曲");
    }
    
    /**
     * @brief 我的收藏
     */
    static QString myFavorites() {
        return tr("我的收藏");
    }
    
    /**
     * @brief 最近播放
     */
    static QString recentPlayed() {
        return tr("最近播放");
    }
    
    /**
     * @brief 本地音乐
     */
    static QString localMusic() {
        return tr("本地音乐");
    }
    
    /**
     * @brief 下载音乐
     */
    static QString downloadedMusic() {
        return tr("下载音乐");
    }
    
    // ========== 标签操作相关字符串 ==========
    
    /**
     * @brief 创建标签
     */
    static QString createTag() {
        return tr("创建标签");
    }
    
    /**
     * @brief 编辑标签
     */
    static QString editTag() {
        return tr("编辑标签");
    }
    
    /**
     * @brief 删除标签
     */
    static QString deleteTag() {
        return tr("删除标签");
    }
    
    /**
     * @brief 重命名标签
     */
    static QString renameTag() {
        return tr("重命名标签");
    }
    
    /**
     * @brief 标签名称
     */
    static QString tagName() {
        return tr("标签名称");
    }
    
    /**
     * @brief 标签描述
     */
    static QString tagDescription() {
        return tr("标签描述");
    }
    
    /**
     * @brief 标签颜色
     */
    static QString tagColor() {
        return tr("标签颜色");
    }
    
    /**
     * @brief 标签图标
     */
    static QString tagIcon() {
        return tr("标签图标");
    }
    
    // ========== 错误和警告消息 ==========
    
    /**
     * @brief 标签名称不能为空
     */
    static QString tagNameCannotBeEmpty() {
        return tr("标签名称不能为空");
    }
    
    /**
     * @brief 标签名称已存在
     */
    static QString tagNameAlreadyExists() {
        return tr("标签名称已存在");
    }
    
    /**
     * @brief 标签名称过长
     * @param maxLength 最大长度
     */
    static QString tagNameTooLong(int maxLength) {
        return tr("标签名称过长，最多%1个字符").arg(maxLength);
    }
    
    /**
     * @brief 标签创建失败
     */
    static QString tagCreationFailed() {
        return tr("标签创建失败");
    }
    
    /**
     * @brief 标签更新失败
     */
    static QString tagUpdateFailed() {
        return tr("标签更新失败");
    }
    
    /**
     * @brief 标签删除失败
     */
    static QString tagDeletionFailed() {
        return tr("标签删除失败");
    }
    
    /**
     * @brief 标签不存在
     */
    static QString tagNotFound() {
        return tr("标签不存在");
    }
    
    /**
     * @brief 无法删除包含歌曲的标签
     */
    static QString cannotDeleteTagWithSongs() {
        return tr("无法删除包含歌曲的标签");
    }
    
    // ========== 确认对话框 ==========
    
    /**
     * @brief 确认删除标签
     * @param tagName 标签名称
     */
    static QString confirmDeleteTag(const QString& tagName) {
        return tr("确定要删除标签 \"%1\" 吗？").arg(tagName);
    }
    
    /**
     * @brief 确认删除标签及其歌曲
     * @param tagName 标签名称
     * @param songCount 歌曲数量
     */
    static QString confirmDeleteTagWithSongs(const QString& tagName, int songCount) {
        return tr("标签 \"%1\" 包含 %2 首歌曲，确定要删除吗？").arg(tagName).arg(songCount);
    }
    
    /**
     * @brief 确认清空标签
     * @param tagName 标签名称
     */
    static QString confirmClearTag(const QString& tagName) {
        return tr("确定要清空标签 \"%1\" 中的所有歌曲吗？").arg(tagName);
    }
    
    // ========== 状态和信息 ==========
    
    /**
     * @brief 标签创建成功
     */
    static QString tagCreatedSuccessfully() {
        return tr("标签创建成功");
    }
    
    /**
     * @brief 标签更新成功
     */
    static QString tagUpdatedSuccessfully() {
        return tr("标签更新成功");
    }
    
    /**
     * @brief 标签删除成功
     */
    static QString tagDeletedSuccessfully() {
        return tr("标签删除成功");
    }
    
    /**
     * @brief 标签为空
     */
    static QString tagIsEmpty() {
        return tr("标签为空");
    }
    
    /**
     * @brief 标签包含歌曲数量
     * @param count 歌曲数量
     */
    static QString tagContainsSongs(int count) {
        return tr("包含 %1 首歌曲").arg(count);
    }
    
    /**
     * @brief 正在加载标签
     */
    static QString loadingTags() {
        return tr("正在加载标签...");
    }
    
    /**
     * @brief 没有找到标签
     */
    static QString noTagsFound() {
        return tr("没有找到标签");
    }
    
    // ========== 菜单和按钮 ==========
    
    /**
     * @brief 确定
     */
    static QString ok() {
        return tr("确定");
    }
    
    /**
     * @brief 取消
     */
    static QString cancel() {
        return tr("取消");
    }
    
    /**
     * @brief 应用
     */
    static QString apply() {
        return tr("应用");
    }
    
    /**
     * @brief 重置
     */
    static QString reset() {
        return tr("重置");
    }
    
    /**
     * @brief 保存
     */
    static QString save() {
        return tr("保存");
    }
    
    /**
     * @brief 关闭
     */
    static QString close() {
        return tr("关闭");
    }
    
    // ========== 工具提示 ==========
    
    /**
     * @brief 双击编辑标签
     */
    static QString doubleClickToEdit() {
        return tr("双击编辑标签");
    }
    
    /**
     * @brief 右键显示菜单
     */
    static QString rightClickForMenu() {
        return tr("右键显示菜单");
    }
    
    /**
     * @brief 拖拽歌曲到标签
     */
    static QString dragSongsToTag() {
        return tr("拖拽歌曲到标签");
    }
    
    // ========== 自定义格式化方法 ==========
    
    /**
     * @brief 格式化时间长度
     * @param seconds 秒数
     */
    static QString formatDuration(int seconds);
    
    /**
     * @brief 格式化文件大小
     * @param bytes 字节数
     */
    static QString formatFileSize(qint64 bytes);
    
    /**
     * @brief 格式化日期时间
     * @param dateTime 日期时间
     */
    static QString formatDateTime(const QDateTime& dateTime);
    
    /**
     * @brief 格式化相对时间
     * @param dateTime 日期时间
     */
    static QString formatRelativeTime(const QDateTime& dateTime);
    
public slots:
    /**
     * @brief 重新加载翻译
     */
    void reloadTranslations();
    
signals:
    /**
     * @brief 语言改变信号
     * @param locale 新的语言区域设置
     */
    void languageChanged(const QLocale& locale);
    
private:
    /**
     * @brief 加载翻译文件
     * @param locale 语言区域设置
     * @return 是否加载成功
     */
    bool loadTranslation(const QLocale& locale);
    
    /**
     * @brief 获取翻译文件路径
     * @param locale 语言区域设置
     * @return 翻译文件路径
     */
    QString getTranslationFilePath(const QLocale& locale) const;
    
private:
    static TagStrings* s_instance;
    mutable QMutex m_mutex;
    
    QLocale m_currentLocale;
    QString m_translationPath;
    QTranslator* m_translator;
    QList<QLocale> m_supportedLocales;
};

// 便利宏定义
#define TAG_TR(text) TagStrings::tr(text)
#define TAG_STR TagStrings::instance()

#endif // TAGSTRINGS_H