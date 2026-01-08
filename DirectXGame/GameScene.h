#pragma once
#include "KamataEngine.h"
#include "Player.h"

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

	// サウンドデータハンドル
	uint32_t soundDataHandle_ = 0;

	// ImGuiで値を入力する変数
	float inputFloat3[3] = {0, 0, 0};

	// 自機
	Player* player_ = nullptr;
};
