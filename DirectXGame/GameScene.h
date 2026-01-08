#pragma once
#include "KamataEngine.h"
#include "Player.h"
#include <vector>

class GameScene {
public:
	/*-------------------- コンストラクタ&デストラクタ --------------------*/
	GameScene();
	~GameScene();

	/*-------------------- メンバ関数 --------------------*/
	// 初期化
	void Initialize();

	// 更新
	void Update();

	// 描画
	void Draw();

private:
	/*-------------------- メンバ変数等 --------------------*/
	// privateにしておく必要があるやつ
	// テクスチャハンドル
	uint32_t textureHandle_ = 0;

	// 3Dモデル
	KamataEngine::Model* model_ = nullptr;

	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTrasform_;

	// カメラ
	KamataEngine::Camera camera_;

	// プレイヤー
	Player* player_ = nullptr;

	// ブロックの3Dモデル
	KamataEngine::Model* modelBlocks_ = nullptr;

	// ブロック用可変個配列
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTrasformBlocks_;

	// デバッグカメラ有効
	bool isDebugCameraActive_ = false;

	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
};
