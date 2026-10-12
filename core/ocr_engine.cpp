#include "ocr_engine.h"
#include "model_pack_reader.h"
#include "crypto.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <numeric>

OCREngine::OCREngine()
    : m_ortEnv(ORT_LOGGING_LEVEL_WARNING, "SnipOCR") {}

OCREngine::~OCREngine() {}

bool OCREngine::isLoaded() const { return m_loaded; }
bool OCREngine::isMultiModel() const { return m_multiLoaded; }
QString OCREngine::lastError() const { return m_error; }

void OCREngine::setLanguage(const QString& code) {
    if (code == "auto" || code == "v6" || code == "ko" || code == "ru" || code == "vi")
        m_lang = code;
}

static Ort::Session* createSession(Ort::Env& env, const QByteArray& modelBytes) {
    try {
        Ort::SessionOptions opts;
        opts.SetIntraOpNumThreads(1);
        opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        return new Ort::Session(env, (const void*)modelBytes.constData(),
                                (size_t)modelBytes.size(), opts);
    } catch (const Ort::Exception& e) {
        qWarning() << "ONNX session create failed:" << e.what();
        return nullptr;
    }
}

// Load a rec bundle from the pack: onnx + dict + io names. Best-effort:
// returns false (not throws) when model files are missing, so the engine
// can still run with whatever bundles ARE available.
static bool loadRecBundle(Ort::Env& env, ModelPackReader& reader,
                          const QString& onnxName, const QString& dictName,
                          RecBundle& out) {
    QByteArray onnxBytes = reader.tryGetResource(onnxName);
    if (onnxBytes.isEmpty()) {
        qWarning() << "[OCR] rec onnx missing:" << onnxName;
        return false;
    }
    out.session.reset(createSession(env, onnxBytes));
    if (!out.session) return false;

    QByteArray dictBytes = reader.tryGetResource(dictName);
    if (dictBytes.isEmpty()) {
        qWarning() << "[OCR] dict missing:" << dictName;
        return false;
    }
    out.charDict.clear();
    QTextStream ts(&dictBytes);
    ts.setEncoding(QStringConverter::Utf8);
    while (!ts.atEnd()) {
        QString line = ts.readLine();
        if (!line.isEmpty()) out.charDict.append(line);
    }

    Ort::AllocatorWithDefaultOptions alloc;
    auto in = out.session->GetInputNameAllocated(0, alloc);
    auto on = out.session->GetOutputNameAllocated(0, alloc);
    out.inputName = in.get();
    out.outputName = on.get();
    out.valid = true;
    qDebug() << "[OCR] bundle loaded:" << onnxName << "dict size:" << out.charDict.size();
    return true;
}

bool OCREngine::loadModel(const QString& packPath) {
    m_loaded = false;
    m_multiLoaded = false;
    ModelPackReader reader(packPath);
    if (!reader.open()) {
        m_error = QString("Open pack failed: %1").arg(reader.lastError());
        return false;
    }

    // Detection model (mandatory)
    QByteArray detBytes = reader.tryGetResource("OCRv6/PP-OCRv6_small_det_infer.onnx");
    if (detBytes.isEmpty()) { m_error = "Load det failed"; return false; }
    m_detSession.reset(createSession(m_ortEnv, detBytes));
    if (!m_detSession) { m_error = "Create det session failed"; return false; }

    auto detIn = m_detSession->GetInputNameAllocated(0, m_allocator);
    auto detOut = m_detSession->GetOutputNameAllocated(0, m_allocator);
    m_detInputName = detIn.get();
    m_detOutputName = detOut.get();

    // v6 rec bundle (mandatory - CJK + English)
    bool v6Ok = loadRecBundle(m_ortEnv, reader,
                              "OCRv6/PP-OCRv6_small_rec_infer.onnx",
                              "OCRv6/ppocrv6_small_dict.txt", m_v6);
    if (!v6Ok) { m_error = "Load v6 rec bundle failed"; return false; }

    // Korean / Cyrillic / Latin rec bundles (best-effort: enable multi-model
    // mode only when at least ko AND ru are available; lat is optional).
    bool koOk = loadRecBundle(m_ortEnv, reader,
                              "OCRv5/korean_pp-ocrv5_mobile_rec.onnx",
                              "OCRv5/ppocrv5_korean_dict.txt", m_ko);
    bool ruOk = loadRecBundle(m_ortEnv, reader,
                              "OCRv5/cyrillic_pp-ocrv5_mobile_rec.onnx",
                              "OCRv5/ppocrv5_cyrillic_dict.txt", m_ru);
    bool latOk = loadRecBundle(m_ortEnv, reader,
                               "OCRv5/latin_pp-ocrv5_mobile_rec.onnx",
                               "OCRv5/ppocrv5_latin_dict.txt", m_lat);
    m_multiLoaded = koOk && ruOk;  // lat is optional (used for vi/auto)
    if (m_multiLoaded)
        qDebug() << "[OCR] multi-model mode: v6+ko+ru" << (latOk ? "+lat" : "(no lat)");
    else
        qDebug() << "[OCR] single-model mode (v6 only)";

    m_loaded = true;
    return true;
}

// ==================== Detection preprocessing ====================
std::vector<float> OCREngine::_preprocessDet(const cv::Mat& img, int& outH, int& outW, int& oriH, int& oriW) {
    oriH = img.rows; oriW = img.cols;
    int h = oriH, w = oriW;
    if (std::max(h, w) > detLimitSideLen) {
        if (h > w) { w = (int)((float)w * detLimitSideLen / h); h = detLimitSideLen; }
        else { h = (int)((float)h * detLimitSideLen / w); w = detLimitSideLen; }
    }
    h = (h / 32) * 32; if (h == 0) h = 32;
    w = (w / 32) * 32; if (w == 0) w = 32;
    outH = h; outW = w;

    cv::Mat resized;
    cv::resize(img, resized, cv::Size(w, h));
    const float mean[3] = {0.485f, 0.456f, 0.406f};
    const float std_[3] = {0.229f, 0.224f, 0.225f};
    std::vector<float> result((size_t)3 * h * w);
    for (int y = 0; y < h; y++) {
        const uint8_t* src = resized.ptr<uint8_t>(y);
        for (int x = 0; x < w; x++) {
            for (int c = 0; c < 3; c++) {
                float v = src[x*3 + c] / 255.0f;
                result[c * h * w + y * w + x] = (v - mean[c]) / std_[c];
            }
        }
    }
    return result;
}

static std::vector<cv::Point2f> orderBoxPoints(const std::vector<cv::Point2f>& pts) {
    std::vector<cv::Point2f> box(4);
    float s[4] = {pts[0].y+pts[0].x, pts[1].y+pts[1].x, pts[2].y+pts[2].x, pts[3].y+pts[3].x};
    int iTL = 0, iBR = 0;
    for (int i = 1; i < 4; i++) { if (s[i] < s[iTL]) iTL = i; if (s[i] > s[iBR]) iBR = i; }
    box[0] = pts[iTL]; box[2] = pts[iBR];
    float diff[4] = {pts[0].x-pts[0].y, pts[1].x-pts[1].y, pts[2].x-pts[2].y, pts[3].x-pts[3].y};
    int iTR = -1, iBL = -1;
    for (int i = 0; i < 4; i++) {
        if (i == iTL || i == iBR) continue;
        if (iTR < 0 || diff[i] > diff[iTR]) iTR = i;
    }
    for (int i = 0; i < 4; i++) {
        if (i == iTL || i == iBR || i == iTR) continue;
        iBL = i;
    }
    box[1] = pts[iTR]; box[3] = pts[iBL];
    return box;
}

// ==================== Detection postprocessing ====================
std::vector<DetBox> OCREngine::_postprocessDet(const float* pred, int pH, int pW, int oriH, int oriW) {
    cv::Mat binary(pH, pW, CV_32FC1, (void*)pred);
    binary = (binary > detDbThresh) / 255;
    binary.convertTo(binary, CV_8UC1, 255);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    std::vector<DetBox> boxes;
    int n = std::min((int)contours.size(), 3000);
    for (int i = 0; i < n; ++i) {
        if (contours[i].size() < 3) continue;
        cv::RotatedRect rect = cv::minAreaRect(contours[i]);
        cv::Point2f pts[4];
        rect.points(pts);
        std::vector<cv::Point2f> box = {pts[0], pts[1], pts[2], pts[3]};
        box = orderBoxPoints(box);

        float sside = std::min(rect.size.width, rect.size.height);
        if (sside < 3) continue;

        std::vector<cv::Point> poly;
        for (auto& p : box) poly.push_back(cv::Point((int)p.x, (int)p.y));
        cv::Mat mask = cv::Mat::zeros(pH, pW, CV_8UC1);
        cv::fillPoly(mask, std::vector<std::vector<cv::Point>>{poly}, 255);
        cv::Mat predMat(pH, pW, CV_32FC1, (void*)pred);
        cv::Scalar m = cv::mean(predMat, mask);
        float score = (float)m[0];
        if (score < detDbBoxThresh) continue;

        float rw = rect.size.width, rh = rect.size.height;
        if (rw < 1e-6f || rh < 1e-6f) continue;
        float area = rw * rh;
        float perimeter = 2 * (rw + rh);
        float distance = area * detDbUnclipRatio / perimeter * 1.10f;
        cv::RotatedRect newRect;
        newRect.center = rect.center;
        newRect.size = cv::Size2f(rw + 2*distance, rh + 2*distance);
        newRect.angle = rect.angle;

        cv::Point2f npts[4];
        newRect.points(npts);
        std::vector<cv::Point2f> nbox = {npts[0], npts[1], npts[2], npts[3]};
        cv::RotatedRect rect2 = cv::minAreaRect(nbox);
        float sside2 = std::min(rect2.size.width, rect2.size.height);
        if (sside2 < 5) continue;

        cv::Point2f pts2[4];
        rect2.points(pts2);
        std::vector<cv::Point2f> finalBox = {pts2[0], pts2[1], pts2[2], pts2[3]};
        finalBox = orderBoxPoints(finalBox);

        for (auto& p : finalBox) {
            p.x = std::clamp(std::round(p.x / pW * oriW), 0.0f, (float)oriW);
            p.y = std::clamp(std::round(p.y / pH * oriH), 0.0f, (float)oriH);
        }
        DetBox db; db.points = finalBox; db.score = score;
        boxes.push_back(db);
    }
    return boxes;
}

std::vector<DetBox> OCREngine::_detect(const cv::Mat& img) {
    int oriH, oriW, H, W;
    std::vector<float> input = _preprocessDet(img, H, W, oriH, oriW);

    std::vector<int64_t> inputShape = {1, 3, H, W};
    size_t inputSize = (size_t)3 * H * W;
    Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memInfo, input.data(), inputSize, inputShape.data(), inputShape.size());

    QByteArray detInName = m_detInputName.toUtf8();
    QByteArray detOutName = m_detOutputName.toUtf8();
    const char* inNames[] = { detInName.constData() };
    const char* outNames[] = { detOutName.constData() };

    auto outputTensors = m_detSession->Run(Ort::RunOptions{nullptr}, inNames, &inputTensor, 1, outNames, 1);

    float* outData = outputTensors[0].GetTensorMutableData<float>();
    auto outShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
    int outH = (int)outShape[2], outW = (int)outShape[3];
    return _postprocessDet(outData, outH, outW, oriH, oriW);
}

// ==================== Recognition preprocessing ====================
static std::vector<float> resizeNormImgRec(const cv::Mat& img, int targetH, int targetW) {
    int h = img.rows, w = img.cols;
    float ratio = (float)w / h;
    int rw = (std::ceil(targetH * ratio) > targetW) ? targetW : (int)std::ceil(targetH * ratio);
    if (rw < 1) rw = 1;
    cv::Mat rgb;
    cv::cvtColor(img, rgb, cv::COLOR_BGR2RGB);
    cv::Mat resized;
    cv::resize(rgb, resized, cv::Size(rw, targetH));
    std::vector<float> result((size_t)3 * targetH * targetW, 0.0f);
    for (int c = 0; c < 3; c++) {
        for (int y = 0; y < targetH; y++) {
            const uint8_t* src = resized.ptr<uint8_t>(y);
            for (int x = 0; x < rw; x++) {
                float v = src[x*3 + c] / 255.0f;
                result[c * targetH * targetW + y * targetW + x] = (v - 0.5f) / 0.5f;
            }
        }
    }
    return result;
}

// ==================== Recognition postprocessing (per-bundle dict) ====================
std::vector<std::pair<QString, float>> OCREngine::_postprocessRec(
    const float* pred, int batch, int seqLen, int cDim, const QStringList& dict) {
    int vocabLen = dict.size();
    std::vector<std::pair<QString, float>> results;
    int spaceIdx = vocabLen + 1;
    for (int b = 0; b < batch; ++b) {
        std::vector<int> idxs(seqLen);
        std::vector<float> probs(seqLen);
        for (int t = 0; t < seqLen; ++t) {
            int bestI = 0;
            float bestP = pred[b * seqLen * cDim + t * cDim];
            for (int c = 1; c < cDim; ++c) {
                float v = pred[b * seqLen * cDim + t * cDim + c];
                if (v > bestP) { bestP = v; bestI = c; }
            }
            idxs[t] = bestI; probs[t] = bestP;
        }
        QString text;
        std::vector<float> confs;
        int prev = -1;
        for (int t = 0; t < seqLen; ++t) {
            int idx = idxs[t];
            if (idx == 0) { prev = idx; continue; }
            if (idx == prev) continue;
            prev = idx;
            if (idx == spaceIdx) { text += " "; confs.push_back(probs[t]); }
            else if (idx > 0 && idx <= vocabLen) { text += dict[idx - 1]; confs.push_back(probs[t]); }
        }
        text = text.trimmed();
        float avgConf = confs.empty() ? 0 : std::accumulate(confs.begin(), confs.end(), 0.0f) / confs.size();
        results.push_back({text, text.isEmpty() ? 0.0f : avgConf});
    }
    return results;
}

// Run a single rec bundle on a batch of crops.
std::vector<std::pair<QString, float>>
OCREngine::_inferBundle(const RecBundle& bundle, const std::vector<cv::Mat>& crops) {
    std::vector<std::pair<QString, float>> results(crops.size(), {"", 0.0f});
    if (crops.empty() || !bundle.valid) return results;

    std::vector<int> indices(crops.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int a, int b) {
        return (float)crops[a].cols / std::max(1, crops[a].rows) <
               (float)crops[b].cols / std::max(1, crops[b].rows);
    });

    Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    const char* inNames[] = { bundle.inputName.constData() };
    const char* outNames[] = { bundle.outputName.constData() };

    for (size_t beg = 0; beg < crops.size(); beg += recBatchNum) {
        size_t end = std::min(beg + (size_t)recBatchNum, crops.size());
        int batch = (int)(end - beg);
        std::vector<cv::Mat> batchImgs;
        for (size_t i = beg; i < end; ++i) batchImgs.push_back(crops[indices[i]]);
        int maxWh = 10;
        for (auto& im : batchImgs) {
            int wr = im.cols / std::max(1, im.rows);
            if (wr > maxWh) maxWh = wr;
        }
        int targetW = maxWh * recImageH;

        std::vector<float> inputData((size_t)batch * 3 * recImageH * targetW, 0.0f);
        for (int b = 0; b < batch; ++b) {
            std::vector<float> chw = resizeNormImgRec(batchImgs[b], recImageH, targetW);
            memcpy(inputData.data() + (size_t)b * 3 * recImageH * targetW,
                   chw.data(), (size_t)3 * recImageH * targetW * sizeof(float));
        }
        std::vector<int64_t> inputShape = {batch, 3, recImageH, targetW};
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memInfo, inputData.data(), inputData.size(), inputShape.data(), inputShape.size());
        auto outputTensors = bundle.session->Run(Ort::RunOptions{nullptr}, inNames,
                                                  &inputTensor, 1, outNames, 1);
        float* outData = outputTensors[0].GetTensorMutableData<float>();
        auto outShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
        int outBatch = (int)outShape[0], seqLen = (int)outShape[1], cDim = (int)outShape[2];
        auto recResults = _postprocessRec(outData, outBatch, seqLen, cDim, bundle.charDict);
        for (int r = 0; r < outBatch && beg + r < crops.size(); ++r)
            results[indices[beg + r]] = recResults[r];
    }
    return results;
}

// Single-model rec (v6 only) - backward-compatible fallback.
std::vector<std::pair<QString, float>> OCREngine::_recognize(const std::vector<cv::Mat>& crops) {
    return _inferBundle(m_v6, crops);
}

// ==================== Script-detection helpers (port of Python) ====================
float OCREngine::_hangulRatio(const QString& text) {
    if (text.isEmpty()) return 0.0f;
    int n = 0;
    for (const QChar& ch : text) {
        uint cp = ch.unicode();
        if ((cp >= 0xAC00 && cp <= 0xD7AF) ||
            (cp >= 0x1100 && cp <= 0x11FF) ||
            (cp >= 0x3130 && cp <= 0x318F))
            ++n;
    }
    return (float)n / (float)text.length();
}

float OCREngine::_cyrillicRatio(const QString& text) {
    if (text.isEmpty()) return 0.0f;
    int n = 0;
    for (const QChar& ch : text) {
        uint cp = ch.unicode();
        if (cp >= 0x0400 && cp <= 0x052F) ++n;
    }
    return (float)n / (float)text.length();
}

bool OCREngine::_hasCnJp(const QString& text) {
    for (const QChar& ch : text) {
        uint cp = ch.unicode();
        // CJK Unified Ideographs + Ext A + Hiragana + Katakana
        if ((cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0x3400 && cp <= 0x4DBF) ||
            (cp >= 0x3040 && cp <= 0x30FF))
            return true;
    }
    return false;
}

QString OCREngine::_extractCyrillic(const QString& text) {
    QString out;
    for (const QChar& ch : text) {
        uint cp = ch.unicode();
        if (cp >= 0x0400 && cp <= 0x052F) out += ch;
        else if (ch == ' ') out += ch;
    }
    return out.trimmed();
}

QString OCREngine::_cleanNoiseSymbol(const QString& text) {
    // Strip common OCR noise: zero-width, BOM, replacement char, stray punctuation
    QString out;
    for (const QChar& ch : text) {
        uint cp = ch.unicode();
        if (cp == 0xFEFF || cp == 0x200B || cp == 0x200C || cp == 0x200D ||
            cp == 0xFFFD || cp == 0x00) continue;
        out += ch;
    }
    return out.trimmed();
}

// ==================== Multi-model rec dispatch (port of Python _process_single_crop) ====================
std::vector<std::pair<QString, float>>
OCREngine::_recognizeMulti(const std::vector<cv::Mat>& crops) {
    std::vector<std::pair<QString, float>> results(crops.size(), {"", 0.0f});
    if (crops.empty()) return results;

    // Run all available bundles on the same crops.
    auto v6Res  = _inferBundle(m_v6, crops);
    auto koRes  = m_ko.valid  ? _inferBundle(m_ko, crops)  : std::vector<std::pair<QString, float>>(crops.size(), {"", 0});
    auto ruRes  = m_ru.valid  ? _inferBundle(m_ru, crops)  : std::vector<std::pair<QString, float>>(crops.size(), {"", 0});
    auto latRes = m_lat.valid ? _inferBundle(m_lat, crops) : std::vector<std::pair<QString, float>>(crops.size(), {"", 0});

    for (size_t i = 0; i < crops.size(); ++i) {
        QString tV6  = _cleanNoiseSymbol(v6Res[i].first);
        float  sV6  = v6Res[i].second;
        QString tKo  = _cleanNoiseSymbol(koRes[i].first);
        float  sKo  = koRes[i].second;
        QString tRu  = _cleanNoiseSymbol(ruRes[i].first);
        float  sRu  = ruRes[i].second;
        QString tLat = _cleanNoiseSymbol(latRes[i].first);
        float  sLat = latRes[i].second;

        // Manual mode shortcuts
        if (m_lang == "v6") {
            results[i] = { tV6, sV6 > multiDropScore ? sV6 : 0.0f };
            if (sV6 <= multiDropScore) results[i].first = "";
            continue;
        }
        if (m_lang == "ko") {
            if (sKo <= multiDropScore) { results[i] = {"", 0}; continue; }
            results[i] = { tKo, sKo };
            continue;
        }
        if (m_lang == "ru") {
            if (sRu <= multiDropScore) { results[i] = {"", 0}; continue; }
            results[i] = { tRu, sRu };
            continue;
        }
        if (m_lang == "vi") {
            if (sLat <= multiDropScore) { results[i] = {"", 0}; continue; }
            results[i] = { tLat, sLat };
            continue;
        }

        // ====== Auto mode: parallel 4-model dispatch (Python L1286-L1327) ======
        float koRatio = _hangulRatio(tKo);
        float ruRatio = _cyrillicRatio(tRu);

        bool v6Valid = !tV6.isEmpty()  && sV6  > multiDropScore;
        bool ruValid = ruRatio > multiCharRatioThr && sRu > multiDropScore;

        // has_cn_jp priority: v6 valid + CJK -> v6 authoritative + ru cyrillic append
        if (v6Valid && _hasCnJp(tV6)) {
            QString finalOut = tV6;
            if (ruValid) {
                QString cyr = _extractCyrillic(tRu);
                if (!cyr.isEmpty()) finalOut = finalOut + " " + cyr;
            }
            results[i] = { finalOut, sV6 };
            continue;
        }

        // Otherwise collect candidates, sort by (ratio, score) desc, pick best
        struct Cand { QString text; float score; float ratio; };
        std::vector<Cand> cands;
        if (koRatio > multiCharRatioThr && sKo > multiDropScore && !tKo.isEmpty())
            cands.push_back({tKo, sKo, koRatio});
        if (ruValid)
            cands.push_back({tRu, sRu, ruRatio});
        if (!tLat.isEmpty() && sLat > multiDropScore)
            cands.push_back({tLat, sLat, 0.0f});
        if (v6Valid)
            cands.push_back({tV6, sV6, 0.0f});

        if (cands.empty()) { results[i] = {"", 0}; continue; }
        std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) {
            if (a.ratio != b.ratio) return a.ratio > b.ratio;
            return a.score > b.score;
        });
        results[i] = { cands[0].text, cands[0].score };
    }
    return results;
}

// ==================== Perspective crop ====================
cv::Mat OCREngine::_getRotateCropImage(const cv::Mat& img, const std::vector<cv::Point2f>& points) {
    if (points.size() < 4) return cv::Mat();
    std::vector<cv::Point2f> src = {points[0], points[1], points[2], points[3]};
    float w1 = cv::norm(src[0] - src[1]), w2 = cv::norm(src[3] - src[2]);
    float h1 = cv::norm(src[0] - src[3]), h2 = cv::norm(src[1] - src[2]);
    int w = (int)std::max(w1, w2), h = (int)std::max(h1, h2);
    if (w < 1 || h < 1) return cv::Mat();
    std::vector<cv::Point2f> dst = {cv::Point2f(0,0), cv::Point2f((float)w,0), cv::Point2f((float)w,(float)h), cv::Point2f(0,(float)h)};
    cv::Mat M = cv::getPerspectiveTransform(src, dst);
    cv::Mat cropped;
    cv::warpPerspective(img, cropped, M, cv::Size(w, h));
    if (h > w * 1.5) cv::rotate(cropped, cropped, cv::ROTATE_90_CLOCKWISE);
    return cropped;
}

// ==================== Sort boxes ====================
std::vector<DetBox> OCREngine::_sortedBoxes(std::vector<DetBox> boxes) {
    if (boxes.empty()) return boxes;
    std::sort(boxes.begin(), boxes.end(), [](const DetBox& a, const DetBox& b) {
        return a.points[0].y < b.points[0].y ||
               (a.points[0].y == b.points[0].y && a.points[0].x < b.points[0].x);
    });
    for (size_t i = 0; i + 1 < boxes.size(); ++i)
        for (int j = (int)i; j >= 0; --j)
            if (std::abs(boxes[j+1].points[0].y - boxes[j].points[0].y) < 10 &&
                boxes[j+1].points[0].x < boxes[j].points[0].x)
                std::swap(boxes[j], boxes[j+1]);
            else break;
    return boxes;
}

// ==================== Main recognize ====================
QVector<OCRResult> OCREngine::recognize(const cv::Mat& imgBgr) {
    QVector<OCRResult> results;
    if (!m_loaded || imgBgr.empty()) return results;
    auto detBoxes = _detect(imgBgr);
    if (detBoxes.empty()) return results;
    detBoxes = _sortedBoxes(detBoxes);
    std::vector<cv::Mat> crops;
    crops.reserve(detBoxes.size());
    for (auto& db : detBoxes) {
        crops.push_back(_getRotateCropImage(imgBgr, db.points));
    }
    auto recResults = m_multiLoaded ? _recognizeMulti(crops) : _recognize(crops);
    for (size_t i = 0; i < detBoxes.size() && i < recResults.size(); ++i) {
        if (recResults[i].second < dropScore) continue;
        if (recResults[i].first.trimmed().isEmpty()) continue;
        OCRResult r;
        r.box = detBoxes[i].points;
        r.text = recResults[i].first;
        r.score = recResults[i].second;
        results.push_back(r);
    }
    return results;
}
