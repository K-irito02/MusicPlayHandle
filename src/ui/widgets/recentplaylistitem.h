#ifndef RECENTPLAYLISTITEM_H
#define RECENTPLAYLISTITEM_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDateTime>
#include "../../models/song.h"

/**
 * @brief 最近播放列表项组件
 * 
 * 用于在"最近播放"标签中显示歌曲信息，包括播放时间
 */
class RecentPlayListItem : public QWidget
{
    Q_OBJECT

public:
    explicit RecentPlayListItem(const Song& song, const QDateTime& playTime, QWidget* parent = nullptr);
    ~RecentPlayListItem();

    /**
     * @brief 获取歌曲信息
     * @return 歌曲对象
     */
    Song song() const { return m_song; }

    /**
     * @brief 获取播放时间
     * @return 播放时间
     */
    QDateTime playTime() const { return m_playTime; }

    /**
     * @brief 设置播放时间
     * @param playTime 播放时间
     */
    void setPlayTime(const QDateTime& playTime);

    /**
     * @brief 更新显示内容
     */
    void updateDisplay();

    /**
     * @brief 设置选中状态
     * @param selected 是否选中
     */
    void setSelected(bool selected);

    /**
     * @brief 获取选中状态
     * @return 是否选中
     */
    bool isSelected() const { return m_selected; }

signals:
    /**
     * @brief 项目被点击信号
     * @param song 歌曲对象
     */
    void itemClicked(const Song& song);

    /**
     * @brief 项目被双击信号
     * @param song 歌曲对象
     */
    void itemDoubleClicked(const Song& song);

    /**
     * @brief 播放按钮被点击信号
     * @param song 歌曲对象
     */
    void playButtonClicked(const Song& song);

protected:
    /**
     * @brief 鼠标点击事件
     * @param event 鼠标事件
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标双击事件
     * @param event 鼠标事件
     */
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /**
     * @brief 鼠标进入事件
     * @param event 鼠标事件
     */
    void enterEvent(QEnterEvent* event) override;

    /**
     * @brief 鼠标离开事件
     * @param event 鼠标事件
     */
    void leaveEvent(QEvent* event) override;

    /**
     * @brief 绘制事件
     * @param event 绘制事件
     */
    void paintEvent(QPaintEvent* event) override;

private:
    void setupUI();
    void setupConnections();
    void updateStyle();

private:
    Song m_song;                    ///< 歌曲对象
    QDateTime m_playTime;           ///< 播放时间
    bool m_selected;                ///< 选中状态
    bool m_hovered;                 ///< 悬停状态

    // UI组件
    QHBoxLayout* m_layout;          ///< 主布局
    QLabel* m_iconLabel;            ///< 图标标签
    QLabel* m_titleLabel;           ///< 标题标签
    QLabel* m_artistLabel;          ///< 艺术家标签
    QLabel* m_timeLabel;            ///< 时间标签
    QPushButton* m_playButton;      ///< 播放按钮
    QPushButton* m_menuButton;      ///< 菜单按钮

    static const int ITEM_HEIGHT = 50;  ///< 项目高度
};

#endif // RECENTPLAYLISTITEM_H 