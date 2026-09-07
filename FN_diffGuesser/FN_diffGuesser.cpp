#include <iostream>
#include <vector>
#include <cmath>
#include <map>
#include <iomanip>
#include <algorithm>
#include <string>
#include <sstream>
#include <optional>
#include <nlohmann/json.hpp>

#define NOMINMAX
#include <windows.h>
#include <wininet.h>

struct NoteStat {
    double time;
    double hitX;
    double hitVX;
    double hitVY;
    double speedMetric;
    bool big;
};

void analyzeNotes(const std::vector<NoteStat>& notes) {
    if (notes.empty()) {
        std::cout << "ノートが存在しない．\n";
        return;
    }

    double duration = notes.back().time - notes.front().time;
    double density = (duration > 0.0) ? (notes.size() / duration) : 0.0;
    double count = static_cast<double>(notes.size());

    double speedMax = notes[0].speedMetric, speedMin = notes[0].speedMetric, speedSum = 0.0;
    double hitXSum = 0.0;
    double hitVXMax = notes[0].hitVX, hitVXMin = notes[0].hitVX, hitVXSum = 0.0;
    double hitVYMax = notes[0].hitVY, hitVYMin = notes[0].hitVY, hitVYSum = 0.0;

    std::map<double, int> speedMetricCounts, hitVXCounts, hitVYCounts;

    for (const auto& n : notes) {
        if (n.speedMetric > speedMax) speedMax = n.speedMetric;
        if (n.speedMetric < speedMin) speedMin = n.speedMetric;
        speedSum += n.speedMetric;

        if (n.hitVX > hitVXMax) hitVXMax = n.hitVX;
        if (n.hitVX < hitVXMin) hitVXMin = n.hitVX;
        hitVXSum += n.hitVX;

        if (n.hitVY > hitVYMax) hitVYMax = n.hitVY;
        if (n.hitVY < hitVYMin) hitVYMin = n.hitVY;
        hitVYSum += n.hitVY;

        hitXSum += n.hitX;

        speedMetricCounts[std::round(n.speedMetric * 10000.0) / 10000.0]++;
        hitVXCounts[std::round(n.hitVX * 10000.0) / 10000.0]++;
        hitVYCounts[std::round(n.hitVY * 10000.0) / 10000.0]++;
    }

    auto getMode = [](const std::map<double, int>& counts) {
        double mode = 0.0;
        int maxCount = 0;
        for (const auto& kv : counts) {
            if (kv.second > maxCount) {
                maxCount = kv.second;
                mode = kv.first;
            }
        }
        return mode;
        };

    double modeSpeedMetric = getMode(speedMetricCounts);
    double modeHitVX = getMode(hitVXCounts);
    double modeHitVY = getMode(hitVYCounts);

    double speedMean = speedSum / count;
    double hitXMean = hitXSum / count;
    double hitVXMean = hitVXSum / count;
    double hitVYMean = hitVYSum / count;

    double speedVarSum = 0.0, hitXVarSum = 0.0, hitVXVarSum = 0.0, hitVYVarSum = 0.0;
    for (const auto& n : notes) {
        speedVarSum += (n.speedMetric - speedMean) * (n.speedMetric - speedMean);
        hitXVarSum += (n.hitX - hitXMean) * (n.hitX - hitXMean);
        hitVXVarSum += (n.hitVX - hitVXMean) * (n.hitVX - hitVXMean);
        hitVYVarSum += (n.hitVY - hitVYMean) * (n.hitVY - hitVYMean);
    }
    double speedVar = speedVarSum / count, hitXVar = hitXVarSum / count;
    double hitVXVar = hitVXVarSum / count, hitVYVar = hitVYVarSum / count;

    auto getMaxDensity = [&](double window) {
        double maxD = 0.0;
        size_t l = 0, r = 0;
        while (l < notes.size()) {
            while (r < notes.size() && (notes[r].time - notes[l].time) <= window) r++;
            double current = (r - l) / window;
            if (current > maxD) maxD = current;
            l++;
        }
        return maxD;
        };
    double maxDensity1s = getMaxDensity(1.0);
    double maxDensity5s = getMaxDensity(5.0);

    double hitXDiffSum = 0.0, speedMetricDiffSum = 0.0;
    double hitXDiffMean = 0.0, hitXDiffVar = 0.0;
    double speedMetricDiffMean = 0.0, speedMetricDiffVar = 0.0, modeSpeedMetricDiff = 0.0;
    std::map<double, int> speedMetricDiffCounts;

    size_t diffCount = notes.size() > 1 ? notes.size() - 1 : 0;
    if (diffCount > 0) {
        for (size_t i = 1; i < notes.size(); ++i) {
            hitXDiffSum += std::abs(notes[i].hitX - notes[i - 1].hitX);
            double sDiff = std::abs(notes[i].speedMetric - notes[i - 1].speedMetric);
            speedMetricDiffSum += sDiff;
            speedMetricDiffCounts[std::round(sDiff * 10000.0) / 10000.0]++;
        }
        hitXDiffMean = hitXDiffSum / diffCount;
        speedMetricDiffMean = speedMetricDiffSum / diffCount;

        double hitXDiffVarSum = 0.0, speedMetricDiffVarSum = 0.0;
        for (size_t i = 1; i < notes.size(); ++i) {
            double hDiff = std::abs(notes[i].hitX - notes[i - 1].hitX);
            hitXDiffVarSum += (hDiff - hitXDiffMean) * (hDiff - hitXDiffMean);

            double sDiff = std::abs(notes[i].speedMetric - notes[i - 1].speedMetric);
            speedMetricDiffVarSum += (sDiff - speedMetricDiffMean) * (sDiff - speedMetricDiffMean);
        }
        hitXDiffVar = hitXDiffVarSum / diffCount;
        speedMetricDiffVar = speedMetricDiffVarSum / diffCount;
        modeSpeedMetricDiff = getMode(speedMetricDiffCounts);
    }

    std::map<int, int> simultaneousCounts;
    int currentCount = 1, simultaneousScore = 0;
    for (size_t i = 1; i < notes.size(); ++i) {
        if (notes[i].time == notes[i - 1].time) {
            currentCount++;
        }
        else {
            if (currentCount >= 2) simultaneousCounts[currentCount]++;
            currentCount = 1;
        }
    }
    if (currentCount >= 2) simultaneousCounts[currentCount]++;

    for (const auto& kv : simultaneousCounts) {
        simultaneousScore += kv.first * kv.second;
    }

    size_t bigTrueCount = 0, bigAdjacentCount01 = 0, bigAdjacentCount02 = 0;
    for (size_t i = 0; i < notes.size(); ++i) {
        if (notes[i].big) bigTrueCount++;
        for (size_t j = i + 1; j < notes.size(); ++j) {
            double timeDiff = notes[j].time - notes[i].time;
            if (timeDiff <= 0.2) {
                if (notes[i].big || notes[j].big) {
                    bigAdjacentCount02++;
                    if (timeDiff <= 0.1) bigAdjacentCount01++;
                }
            }
            else break;
        }
    }
    double bigTrueRatio = (count > 0) ? (static_cast<double>(bigTrueCount) / count) : 0.0;

    double bigNps = duration > 0.0 ? (count + bigTrueCount) / duration : 0.0;

    double rawNotesBase = std::round((std::log(std::max(1.0, bigNps)) / std::log(5.0)) * 100.0);
    double valNotes = std::round(std::min(200.0, rawNotesBase));
    double rawNotes = std::round(rawNotesBase);

    double rawPeakBase = std::round(maxDensity1s * maxDensity5s * 0.7);
    double valPeak = std::round(std::min(200.0, rawPeakBase));
    double rawPeak = std::round(rawPeakBase);

    double rawBigBase = std::round(std::min(50.0, count * bigTrueCount / 800.0) + (bigAdjacentCount01 * bigAdjacentCount01) / 3000.0 + bigAdjacentCount02 / 10.0);
    double valBig = std::round(std::min(200.0, rawBigBase));
    double rawBig = std::round(rawBigBase);

    double rawScrollBase = std::pow(modeSpeedMetric, 4.0) / 8000000000.0;
    double valScroll = std::round(std::min(200.0, rawScrollBase));
    double rawScroll = std::round(rawScrollBase);

    double rawSpreadCalc = (hitXDiffVar * 10.0) * (hitVXVar * 10.0);
    double rawSpreadBase = 30.0 * std::log(rawSpreadCalc + 1.0);
    double valSpread = std::round(std::min(200.0, rawSpreadBase));
    double rawSpread = std::round(rawSpreadBase);

    double rawChordBase = simultaneousScore * 0.3;
    double valChord = std::round(std::min(200.0, rawChordBase));
    double rawChord = std::round(rawChordBase);

    double params[6] = { valNotes, valPeak, valBig, valScroll, valSpread, valChord };
    double maxParam = params[0];
    double sumParams = 0.0;
    for (int i = 0; i < 6; ++i) {
        if (params[i] > maxParam) maxParam = params[i];
        sumParams += params[i];
    }
    double rawDifficulty = maxParam * 0.06 + (sumParams - maxParam) * 0.015;
    double finalDifficulty = std::min(20.0, std::floor(rawDifficulty));

    std::cout << "基本情報\n";
    std::cout << "  総ノート数: " << count << " / 曲の長さ: " << duration << " 秒\n";
    std::cout << "  密度: [全体] " << density << " / [1秒最大] " << maxDensity1s << " / [5秒最大] " << maxDensity5s << " (notes/sec)\n\n";

    std::cout << "各パラメータの統計 (最大 / 最小 / 平均 / 最頻 / 分散)\n";
    std::cout << "  速度(Accel*v): " << speedMax << " / " << speedMin << " / " << speedMean << " / " << modeSpeedMetric << " / " << speedVar << "\n";
    std::cout << "  hitVX        : " << hitVXMax << " / " << hitVXMin << " / " << hitVXMean << " / " << modeHitVX << " / " << hitVXVar << "\n";
    std::cout << "  hitVY        : " << hitVYMax << " / " << hitVYMin << " / " << hitVYMean << " / " << modeHitVY << " / " << hitVYVar << "\n";
    std::cout << "  hitX         : (最大・最小略) / " << hitXMean << " / (略) / " << hitXVar << "\n\n";

    std::cout << "ひとつ前の音符との差分(絶対値) (平均 / 最頻 / 分散)\n";
    std::cout << "  速度(Accel*v): " << speedMetricDiffMean << " / " << modeSpeedMetricDiff << " / " << speedMetricDiffVar << "\n";
    std::cout << "  hitX         : " << hitXDiffMean << " / (略) / " << hitXDiffVar << "\n\n";

    std::cout << "同時押し\n";
    if (simultaneousCounts.empty()) {
        std::cout << "  なし\n";
    }
    else {
        std::cout << "  ";
        for (const auto& kv : simultaneousCounts) {
            std::cout << "[" << kv.first << "点: " << kv.second << "回] ";
        }
        std::cout << "\n  スコア: " << simultaneousScore << "\n";
    }
    std::cout << "\n";

    std::cout << "Big音符 (第4引数)\n";
    std::cout << "  trueの回数: " << bigTrueCount << " (" << bigTrueRatio * 100.0 << "%)\n";
    std::cout << "  Big隣接ペア数: [0.1秒以内] " << bigAdjacentCount01 << " / [0.2秒以内] " << bigAdjacentCount02 << "\n\n";

    std::cout << "評価パラメータ\n";
    std::cout << "NOTES: " << valNotes << "(" << rawNotes << ")\n";
    std::cout << "PEAK: " << valPeak << "(" << rawPeak << ")\n";
    std::cout << "BIG: " << valBig << "(" << rawBig << ")\n";
    std::cout << "SCROLL: " << valScroll << "(" << rawScroll << ")\n";
    std::cout << "SPREAD: " << valSpread << "(" << rawSpread << ")\n";
    std::cout << "CHORD: " << valChord << "(" << rawChord << ")\n\n";

    std::cout << "難易度(生): " << rawDifficulty << "\n";
    std::cout << "難易度(最終): " << finalDifficulty << "\n";
}

const std::string host = "nikochan.utcode.net";

std::optional<std::vector<uint8_t>> fetchHttps(const std::string& path) {
    struct RaiiHandle {
        HINTERNET handle;
        RaiiHandle(HINTERNET handle) : handle(handle) {}
        ~RaiiHandle() { if (handle) InternetCloseHandle(handle); }
    };
    RaiiHandle hInternet = InternetOpenA("ChartAnalyzer/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet.handle) {
        std::cerr << "InternetOpen failed: " << GetLastError() << std::endl;
        return std::nullopt;
    }

    RaiiHandle hConnect = InternetConnectA(hInternet.handle, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect.handle) {
        std::cerr << "InternetConnect failed: " << GetLastError() << std::endl;
        return std::nullopt;
    }

    DWORD flags = INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
        INTERNET_FLAG_IGNORE_CERT_CN_INVALID | INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;

    RaiiHandle hRequest = HttpOpenRequestA(hConnect.handle, "GET", path.c_str(), NULL, NULL, NULL, flags, 0);
    if (!hRequest.handle) {
        std::cerr << "HttpOpenRequest failed: " << GetLastError() << std::endl;
        return std::nullopt;
    }

    std::string headers = "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\nAccept: */*\r\n";
    BOOL sent = HttpSendRequestA(hRequest.handle, headers.c_str(), static_cast<DWORD>(headers.length()), NULL, 0);
    if (!sent) {
        DWORD err = GetLastError();
        std::cerr << "HttpSendRequest failed with error code: " << err << std::endl;
        return std::nullopt;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    HttpQueryInfoA(hRequest.handle, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &statusCodeSize, NULL);
    if (statusCode != 200) {
        std::cerr << "HTTP status: " << statusCode;
        return std::nullopt;
    }

    std::vector<uint8_t> result;
    char buffer[8192];
    DWORD bytesRead = 0;
    while (InternetReadFile(hRequest.handle, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        result.insert(result.end(), buffer, buffer + bytesRead);
    }

    return result;
}

std::string utf8ToAnsi(const std::string& utf8Str) {
    if (utf8Str.empty()) return "";

    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), (int)utf8Str.size(), NULL, 0);
    if (wlen <= 0) return "";

    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), (int)utf8Str.size(), &wstr[0], wlen);

    int alen = WideCharToMultiByte(CP_ACP, 0, wstr.data(), wlen, NULL, 0, NULL, NULL);
    if (alen <= 0) return "";

    std::string astr(alen, 0);
    WideCharToMultiByte(CP_ACP, 0, wstr.data(), wlen, &astr[0], alen, NULL, NULL);

    return astr;
}

int main(int argc, char* argv[]) {
    std::cout << "cid を入力してください: ";
    std::string cid;
    std::getline(std::cin, cid);

    auto rawBytes = fetchHttps("/api/brief/" + cid);
    if (!rawBytes) return 1;
    std::string jsonStr((*rawBytes).begin(), (*rawBytes).end());
    auto brief = nlohmann::json::parse(jsonStr);

    std::string title = brief.value("title", "");
    std::string composer = brief.value("composer", "");
    std::string chartCreator = brief.value("chartCreator", "");
    std::cout << "\n";
    std::cout << "タイトル      : " << utf8ToAnsi(title) << std::endl;
    std::cout << "作曲者        : " << utf8ToAnsi(composer) << std::endl;
    std::cout << "譜面制作者    : " << utf8ToAnsi(chartCreator) << std::endl;

    const auto& levels = brief["levels"];

    for (size_t lvIndex = 0; lvIndex < levels.size(); ++lvIndex) {
        if (levels[lvIndex].value("unlisted", false)) continue;

        std::string lvName = levels[lvIndex].value("name", "");
        std::string lvType = levels[lvIndex].value("type", "Single");
        int diff = levels[lvIndex].value("difficulty", 0);
        std::cout << "\n========================================\n";
        std::cout << "レベル [" << lvIndex << "] "
            << utf8ToAnsi(lvName.empty() ? "" : lvName + " ")
            << "(" << lvType << ") "
            << "公式難易度: " << diff
            << " の解析を開始"
            << std::endl;
        std::cout << "\n";

        auto rawBytesLv = fetchHttps("/api/seqFile/" + cid + "/" + std::to_string(lvIndex));
        if (!rawBytesLv) continue;
        nlohmann::json chart = nlohmann::json::from_msgpack(*rawBytesLv);

        std::vector<NoteStat> notes;
        if (chart.contains("notes") && chart["notes"].is_array()) {
            for (const auto& n : chart["notes"]) {
                double t = n.value("hitTimeSec", 0.0);
                double hitX = n.value("targetX", 0.5) * 10.0 - 5.0;
                double hitVX = n.value("vx", 0.0) * 4.0;
                double hitVY = n.value("vy", 0.0) * 4.0;
                bool big = n.value("big", false);

                double accel = n["display"][0].value("du", 1.0 / 120.0) * 120.0;
                double speed = accel * std::sqrt(hitVX * hitVX + hitVY * hitVY);

                notes.push_back({ t, hitX, hitVX, hitVY, speed, big });
            }
        }

        analyzeNotes(notes);
    }
}