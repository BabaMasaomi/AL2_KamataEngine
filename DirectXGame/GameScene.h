#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Player.h"
#include "Skydome.h"
#include "Transform.h"
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

	// 表示ブロックの生成
	void GenerateBlocks();

private:
	// privateにしておく必要があるやつ
	// カメラ
	KamataEngine::Camera camera_;

	// マップチップフィールド
	MapChipField* mapChipField_;

	// Translateクラス内の関数を使える様にする
	Transform transform_;

	/*-------------------- プレイヤー --------------------*/
	// プレイヤーの3Dモデル
	KamataEngine::Model* model_ = nullptr;

	// プレイヤーのワールドトランスフォーム
	KamataEngine::WorldTransform worldTrasform_;

	// プレイヤー
	Player* player_ = nullptr;

	/*-------------------- 天球 --------------------*/
	// 天球の3Dモデル
	KamataEngine::Model* modelSkydome_ = nullptr;

	// 天球のワールドトランスフォーム
	KamataEngine::WorldTransform worldTrasformSkydome_;

	// 天球
	Skydome* skydome_ = nullptr;

	/*-------------------- ブロック --------------------*/
	// ブロックの3Dモデル
	KamataEngine::Model* modelBlocks_ = nullptr;

	// ブロック用可変個配列
	std::vector<std::vector<KamataEngine::WorldTransform*>> worldTransformBlocks_;

	/*-------------------- デバッグ --------------------*/
	// デバッグカメラ有効
	bool isDebugCameraActive_ = false;

	// デバッグカメラ
	KamataEngine::DebugCamera* debugCamera_ = nullptr;
};
