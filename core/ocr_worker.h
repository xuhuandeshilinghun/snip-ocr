/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QThread>
#include <QString>
#include <QVector>
#include <opencv2/opencv.hpp>

// OCR background worker thread. Loads model and recognizes text from images.
// Faithfully ports Python core/ocr_worker.py.
class OCRWorker : public QThread {
    Q_OBJECT
public:
    explicit OCRWorker(const QVector<cv::Mat>& images, const QString& modelPath,
                       QObject* parent = nullptr);
    ~OCRWorker() override;

    void cancel();

signals:
    void finished(const QString& text);
    void error(const QString& errMsg);
    void progress(const QString& msg);

protected:
    void run() override;

private:
    QVector<cv::Mat> m_images;
    QString m_modelPath;
    bool m_cancelled = false;
};
