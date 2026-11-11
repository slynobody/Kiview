// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

#include <KZip>

#include <QDir>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QPdfWriter>
#include <QProcess>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::Literals::StringLiterals;

/**
 * Starts the kiview binary, offscreen, on files of each kind, and checks that it keeps running
 * without QML errors.
 */
class SmokeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testOpenFiles_data();
    void testOpenFiles();

private:
    QString path(const QString &name) const
    {
        return m_dir.filePath(name);
    }

    QTemporaryDir m_dir;
};

void SmokeTest::initTestCase()
{
    QVERIFY(m_dir.isValid());

    QFile text(path(u"text.txt"_s));
    QVERIFY(text.open(QIODevice::WriteOnly));
    text.write("Some text\nwith two lines\n");
    text.close();

    QFile script(path(u"script"_s));
    QVERIFY(script.open(QIODevice::WriteOnly));
    script.write("#!/bin/sh\necho hello\n");
    script.close();

    QImage image(64, 48, QImage::Format_RGB32);
    image.fill(Qt::red);
    QVERIFY(image.save(path(u"image.png"_s)));

    QPdfWriter pdf(path(u"document.pdf"_s));
    QPainter painter(&pdf);
    painter.drawText(QPointF(100, 100), u"A page of a PDF"_s);
    QVERIFY(painter.end());

    QVERIFY(QDir(m_dir.path()).mkdir(u"folder"_s));
    QFile inFolder(path(u"folder/inside.txt"_s));
    QVERIFY(inFolder.open(QIODevice::WriteOnly));
    inFolder.close();

    KZip zip(path(u"archive.zip"_s));
    QVERIFY(zip.open(QIODevice::WriteOnly));
    QVERIFY(zip.writeFile(u"inside.txt"_s, QByteArray("text in an archive\n")));
    QVERIFY(zip.close());

    QFile unknown(path(u"unknown.xyz"_s));
    QVERIFY(unknown.open(QIODevice::WriteOnly));
    unknown.write(QByteArray(32, '\0'));
    unknown.close();
}

void SmokeTest::testOpenFiles_data()
{
    QTest::addColumn<QStringList>("files");

    QTest::newRow("text") << QStringList{u"text.txt"_s};
    QTest::newRow("text without extension") << QStringList{u"script"_s};
    QTest::newRow("image") << QStringList{u"image.png"_s};
    QTest::newRow("pdf") << QStringList{u"document.pdf"_s};
    QTest::newRow("folder") << QStringList{u"folder"_s};
    QTest::newRow("archive") << QStringList{u"archive.zip"_s};
    QTest::newRow("unknown") << QStringList{u"unknown.xyz"_s};
    QTest::newRow("several files") << QStringList{u"text.txt"_s, u"image.png"_s, u"archive.zip"_s};
}

void SmokeTest::testOpenFiles()
{
    QFETCH(QStringList, files);

    QStringList arguments;
    for (const QString &file : std::as_const(files)) {
        arguments << path(file);
    }

    QProcess kiview;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    environment.insert(u"QT_QPA_PLATFORM"_s, u"offscreen"_s);
    environment.insert(u"QT_FORCE_STDERR_LOGGING"_s, u"1"_s);
    kiview.setProcessEnvironment(environment);
    kiview.setProcessChannelMode(QProcess::MergedChannels);
    kiview.start(QStringLiteral(KIVIEW_BINARY), arguments);
    QVERIFY2(kiview.waitForStarted(), qPrintable(kiview.errorString()));

    // Kiview runs until it is closed, so a process that ends within this time has failed.
    const bool ended = kiview.waitForFinished(4000);
    const QString output = QString::fromLocal8Bit(kiview.readAll());
    QVERIFY2(!ended, qPrintable(u"kiview ended with %1:\n%2"_s.arg(kiview.exitCode()).arg(output)));

    // The QML files of Kiview are in its resources, so their warnings name a qrc URL.
    const QStringList lines = output.split(u'\n');
    for (const QString &line : lines) {
        QVERIFY2(!line.contains(u"qrc:/"_s), qPrintable(line));
        QVERIFY2(!line.contains(u"AddressSanitizer"_s), qPrintable(output));
    }

    kiview.terminate();
    QVERIFY(kiview.waitForFinished(5000));
}

QTEST_MAIN(SmokeTest)

#include "smoketest.moc"
