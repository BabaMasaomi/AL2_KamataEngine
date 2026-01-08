#pragma once
#include "KamataEngine.h"

// vvv ここグローバル vvv
enum class MapChipType {
	kBlank, // 空白
	kBlock, // ブロック
};

struct MapChipData {
	// 可変個配列
	std::vector<std::vector<MapChipType>> data;
};

class MapChipField {
public:
	// コンストラクタ&デストラクタ
	MapChipField();
	~MapChipField();

	// 定数の定義
	// 1ブロックのサイズ
	static inline const float kBlockWidth = 2.0f;
	static inline const float kBlockHeight = 2.0f;

	// ブロックの個数
	static inline const uint32_t kNumBlockVirchical = 20;
	static inline const uint32_t kNumBlockHorizontal = 100;

	// マップチップデータのリセット
	void ResetMapChipData();

	// マップチップデータのの読み込み
	void LoadMapChipCsv(const std::string filePath);

	// マップチップ種別の取得
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);

	// マップチップ座標の取得
	KamataEngine::Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);

private:
	// 構造体型変数
	MapChipData mapChipData_;
};
