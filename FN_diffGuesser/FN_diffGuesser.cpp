#include <iostream>
#include <vector>
#include <cmath>
#include <map>
#include <iomanip>
#include <algorithm>
#include <sol/sol.hpp>

struct NoteStat {
    double time;
    double hitX;
    double hitVX;
    double hitVY;
    double speedMetric; // 後で計算する
    bool big;
};

// Accelの変化を記録する構造体
struct AccelEvent {
    double time;
    int type; // 0: Accel(開始), 1: AccelEnd(終了)
    double val;
};

int main() {
    sol::state lua;
    lua.open_libraries();

    lua["package"]["loaded"]["fn-commands"] = true;

    std::vector<NoteStat> notes;
    std::vector<AccelEvent> accelEvents;

    double currentTime = 0.0;
    double currentBPM = 120.0;

    // 初期状態のAccelは1.0とする
    accelEvents.push_back({ 0.0, 0, 1.0 });

    lua.set_function("BPM", [&](double bpm) { currentBPM = bpm; });

    // 時間と値を履歴として保存するだけに変更
    lua.set_function("Accel", [&](double a) {
        accelEvents.push_back({ currentTime, 0, a });
        });
    lua.set_function("AccelEnd", [&](double a) {
        accelEvents.push_back({ currentTime, 1, a });
        });

    lua.set_function("Beat", [](sol::table, sol::optional<double>, sol::optional<double>) {});

    lua.set_function("Step", [&](double num, double denom) {
        currentTime += (num / denom) * 240.0 / currentBPM;
        });

    // 読み込み時はspeedMetricを仮の0.0にしておく
    lua.set_function("Note", [&](double hitX, double hitVX, double hitVY, bool big, sol::optional<bool> fall) {
        notes.push_back({ currentTime, hitX, hitVX, hitVY, 0.0, big });
        });

    lua.set_function("fnChart", [&](sol::table chartData) {
        sol::optional<sol::table> levelsOpt = chartData["levels"];
        if (levelsOpt) {
            sol::table levels = levelsOpt.value();
            for (auto& kv : levels) {
                sol::table level = kv.second.as<sol::table>();
                sol::optional<sol::function> contentOpt = level["content"];
                if (contentOpt) {
                    contentOpt.value()();
                }
            }
        }
        });

    try {
        lua.script_file("chart.lua");
    }
    catch (const sol::error& e) {
        std::cerr << "解析エラー: " << e.what() << "\n";
        return 1;
    }

    if (notes.empty()) {
        std::cout << "ノートが存在しない．\n";
        return 0;
    }

    // --- ここからパス2：各音符の時間のAccelを計算してspeedMetricを確定させる ---
    auto getAccelAt = [&](double t) {
        if (accelEvents.empty()) return 1.0;

        int lastIdx = -1;
        for (int i = 0; i < (int)accelEvents.size(); ++i) {
            if (accelEvents[i].time <= t) {
                lastIdx = i;
            }
            else {
                break; // 時間順に並んでいる前提
            }
        }

        if (lastIdx == -1) return accelEvents[0].val;

        const auto& lastEv = accelEvents[lastIdx];

        // 最後のイベントがAccelEndなら，そのまま一定
        if (lastEv.type == 1) return lastEv.val;

        // 最後のイベントがAccelなら，次に来るAccelEndとの間で線形補間する
        if (lastEv.type == 0) {
            if (lastIdx + 1 < (int)accelEvents.size()) {
                const auto& nextEv = accelEvents[lastIdx + 1];
                if (nextEv.type == 1 && nextEv.time > lastEv.time) {
                    double ratio = (t - lastEv.time) / (nextEv.time - lastEv.time);
                    if (ratio < 0.0) ratio = 0.0;
                    if (ratio > 1.0) ratio = 1.0;
                    return lastEv.val + (nextEv.val - lastEv.val) * ratio;
                }
            }
            return lastEv.val;
        }
        return 1.0;
        };

    for (auto& n : notes) {
        double currentAccel = getAccelAt(n.time);
        n.speedMetric = currentAccel * std::sqrt(n.hitVX * n.hitVX + n.hitVY * n.hitVY);
    }
    // ---------------------------------------------------------------------

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

    // --- パラメータ計算 ---
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

    return 0;
}