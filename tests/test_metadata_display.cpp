#include <QTest>
#include <QSignalSpy>
#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include "../../src/models/song.h"
#include "../../src/audio/audioengine.h"

class TestMetadataDisplay : public QObject
{
    Q_OBJECT

private slots:
    void testFFmpegMetadataExtraction();
    void testCoverArtExtraction();
    void testMetadataDisplayConsistency();
};

void TestMetadataDisplay::testFFmpegMetadataExtraction()
{
    // 测试FFmpeg元数据解析
    QString testFilePath = "test_audio.mp3"; // 需要实际的测试文件
    
    // 测试元数据获取方法
    QString title = Song::getTitleFromMetadata(testFilePath);
    QString artist = Song::getArtistFromMetadata(testFilePath);
    QString album = Song::getAlbumFromMetadata(testFilePath);
    
    qDebug() << "Extracted metadata:";
    qDebug() << "Title:" << title;
    qDebug() << "Artist:" << artist;
    qDebug() << "Album:" << album;
    
    // 验证元数据不为空（如果有测试文件的话）
    if (QFile::exists(testFilePath)) {
        QVERIFY(!title.isEmpty() || !artist.isEmpty());
    }
}

void TestMetadataDisplay::testCoverArtExtraction()
{
    // 测试封面提取
    QString testFilePath = "test_audio.mp3"; // 需要实际的测试文件
    
    if (QFile::exists(testFilePath)) {
        QPixmap cover = Song::extractCoverArt(testFilePath, QSize(300, 300));
        
        qDebug() << "Cover art extracted:" << !cover.isNull();
        
        // 验证封面尺寸
        if (!cover.isNull()) {
            QVERIFY(cover.width() <= 300);
            QVERIFY(cover.height() <= 300);
        }
    }
}

void TestMetadataDisplay::testMetadataDisplayConsistency()
{
    // 测试元数据显示一致性
    QString testFilePath = "test_audio.mp3"; // 需要实际的测试文件
    
    if (QFile::exists(testFilePath)) {
        Song song = Song::fromFile(testFilePath);
        
        // 验证元数据解析
        QVERIFY(song.isValid());
        QVERIFY(!song.title().isEmpty() || !song.artist().isEmpty());
        
        qDebug() << "Song metadata:";
        qDebug() << "Title:" << song.title();
        qDebug() << "Artist:" << song.artist();
        qDebug() << "Album:" << song.album();
    }
}

QTEST_MAIN(TestMetadataDisplay)
#include "test_metadata_display.moc" 