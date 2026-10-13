/*
 * SnipOCR Copyright (c) 2026 edenl
 * Licensed under PolyForm Noncommercial License 1.0.0
 * Commercial use requires separate authorization from the author.
 */
#pragma once
#include <QString>
#include <QVector>
#include <memory>
#include <vector>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

struct OCRResult {
    std::vector<cv::Point2f> box;
    QString text;
    float score;
};

struct DetBox {
    std::vector<cv::Point2f> points;
    float score;
};

// Recognition bundle: one ONNX rec session + its character dict.
// Mirrors Python's RecBundle. v6=PP-OCRv6 (CJK+EN), ko=Korean,
// ru=Cyrillic/Russian, lat=Latin.
struct RecBundle {
    std::unique_ptr<Ort::Session> session;
    QStringList charDict;
    QByteArray inputName;
    QByteArray outputName;
    bool valid = false;
};

class OCREngine {
public:
    OCREngine();
    ~OCREngine();

    bool loadModel(const QString& packPath);
    bool isLoaded() const;
    bool isMultiModel() const;
    QString lastError() const;

    QVector<OCRResult> recognize(const cv::Mat& imgBgr);

    // Language selection: "auto" runs all 4 models and dispatches.
    void setLanguage(const QString& code);

    float detDbThresh = 0.2f;
    float detDbBoxThresh = 0.45f;
    float detDbUnclipRatio = 1.4f;
    int detLimitSideLen = 960;
    int recBatchNum = 6;
    int recImageH = 48;
    int recImageW = 320;
    float dropScore = 0.40f;

    // Multi-model shared thresholds (matching Python _MULTI_* constants)
    float multiDropScore = 0.40f;
    float multiCharRatioThr = 0.30f;

private:
    std::vector<float> _preprocessDet(const cv::Mat& img, int& outH, int& outW, int& oriH, int& oriW);
    std::vector<DetBox> _postprocessDet(const float* pred, int pH, int pW, int oriH, int oriW);
    std::vector<DetBox> _detect(const cv::Mat& img);

    // Single-model rec inference (kept for backward compat / fallback)
    std::vector<std::pair<QString, float>> _postprocessRec(const float* pred, int batch, int seqLen, int cDim,
                                                           const QStringList& dict);
    std::vector<std::pair<QString, float>> _recognize(const std::vector<cv::Mat>& crops);

    // Multi-model rec: runs v6/ko/ru/lat per crop and dispatches by script ratio.
    std::vector<std::pair<QString, float>> _recognizeMulti(const std::vector<cv::Mat>& crops);

    // Run a single rec bundle on a batch of crops, return per-crop (text, score).
    std::vector<std::pair<QString, float>> _inferBundle(const RecBundle& bundle,
                                                       const std::vector<cv::Mat>& crops);

    // Script-detection helpers (port of Python hangul_ratio / cyrillic_ratio / _has_cn_jp)
    static float _hangulRatio(const QString& text);
    static float _cyrillicRatio(const QString& text);
    static bool _hasCnJp(const QString& text);
    static QString _extractCyrillic(const QString& text);
    static QString _cleanNoiseSymbol(const QString& text);

    cv::Mat _getRotateCropImage(const cv::Mat& img, const std::vector<cv::Point2f>& points);
    std::vector<DetBox> _sortedBoxes(std::vector<DetBox> boxes);

    QString m_error;
    bool m_loaded = false;
    bool m_multiLoaded = false;
    QString m_lang = "auto";

    Ort::Env m_ortEnv;
    std::unique_ptr<Ort::Session> m_detSession;
    Ort::AllocatorWithDefaultOptions m_allocator;

    QString m_detInputName;
    QString m_detOutputName;

    // v6 is the primary bundle (also used as m_recSession equivalent for fallback).
    RecBundle m_v6;
    RecBundle m_ko;
    RecBundle m_ru;
    RecBundle m_lat;
};
