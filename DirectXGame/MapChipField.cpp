#include "MapChipField.h"
#include <cassert>
#include <map>
// ↓csvファイル読み込みに使うinclude↓
#include <fstream>
#include <sstream>

// コンストラクタ&デストラクタ
MapChipField::MapChipField() {}
MapChipField::~MapChipField() {}

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

// csvファイルから取得したデータを基に空白かブロックか設定
namespace { // 無名名前空間(グローバル変数の名前の衝突を避けられて安心)
std::map<std::string, MapChipType> mapChipTable = {
    {"0", MapChipType::kBlank},
    {"1", MapChipType::kBlock},
};

}

// マップチップデータのリセット
void MapChipField::ResetMapChipData() {
	mapChipData_.data.clear();
	mapChipData_.data.resize(kNumBlockVirchical);
	for (std::vector<MapChipType>& mapChipDataLine : mapChipData_.data) {
		mapChipDataLine.resize(kNumBlockHorizontal);
	}
}

// マップチップデータのの読み込み
void MapChipField::LoadMapChipCsv(const std::string filePath) {
	// マップチップデータのリセット
	ResetMapChipData();

	// ファイルを開く
	std::ifstream file;
	file.open(filePath);
	assert(file.is_open());

	// マップチップcsv
	std::stringstream mapChipCsv;
	// ファイルの内容を文字列ストリームにコピー
	mapChipCsv << file.rdbuf();
	// ファイルを閉じる
	file.close();

	// csvからマップチップデータを読み込む
	for (uint32_t i = 0; i < kNumBlockVirchical; ++i) {
		// y方向
		std::string line;
		getline(mapChipCsv, line);

		// 1行分の文字列をストリームに変換して解析しやすくする
		std::istringstream lineStream(line);

		for (uint32_t j = 0; j < kNumBlockHorizontal; ++j) {
			// x方向
			std::string word;
			std::getline(lineStream, word, ',');

			if (mapChipTable.contains(word)) {
				mapChipData_.data[i][j] = mapChipTable[word];
			}
		}
	}
}

// マップチップ種別の取得
MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
	// csvファイルの範囲外を指定してしまった場合、kBlankで返す(安全性の向上)
	if (xIndex < 0 || kNumBlockHorizontal - 1 < xIndex) {
		return MapChipType::kBlank;
	}
	if (yIndex < 0 || kNumBlockVirchical - 1 < yIndex) {
		return MapChipType::kBlank;
	}

	return mapChipData_.data[yIndex][xIndex];
}

// マップチップ座標の取得
Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) { return Vector3(kBlockWidth * xIndex, kBlockHeight * (kNumBlockVirchical - 1 - yIndex), 0); }