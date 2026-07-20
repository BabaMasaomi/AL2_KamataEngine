#pragma once
#include <random>

// 乱数生成エンジン
inline std::random_device seedGenerator;
// メルセンヌ・ツイスターエンジン(64bit版)
inline std::mt19937_64 randomEngine(seedGenerator());