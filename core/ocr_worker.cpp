#include "ocr_worker.h"
#include "ocr_engine.h"

#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>

OCRWorker::OCRWorker(const QVector<cv::Mat>& images, const QString& modelPath,
                       QObject* parent)
    : QThread(parent)
    , m_images(images)
    , m_modelPath(modelPath)
{
}

OCRWorker::~OCRWorker()
{
    // Ensure thread is stopped before destruction
    if (isRunning()) {
        cancel();
        wait(5000);
    }
}

void OCRWorker::cancel()
{
    m_cancelled = true;
}

void OCRWorker::run()
{
    try {
        if (m_cancelled) return;
        emit progress("");

        if (!QFileInfo::exists(m_modelPath)) {
            emit error("Model file not found: " + m_modelPath);
            return;
        }

        OCREngine ocr;
        if (!ocr.loadModel(m_modelPath)) {
            emit error(ocr.lastError());
            return;
        }

        QStringList allText;
        for (int i = 0; i < m_images.size(); ++i) {
            if (m_cancelled) return;
            emit progress(QStringLiteral("Recognizing %1/%2").arg(i + 1).arg(m_images.size()));
            auto results = ocr.recognize(m_images[i]);
            for (const auto& r : results) {
                if (!r.text.isEmpty())
                    allText << r.text;
            }
        }

        if (m_cancelled) return;
        emit finished(allText.join("\n"));
    } catch (const std::exception& e) {
        emit error(QString::fromUtf8(e.what()));
    }
}
