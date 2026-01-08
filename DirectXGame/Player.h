#pragma once
#include "KamataEngine.h"

class Player {
public:
	Player();
	~Player();

	/// <summary>
	/// 自機の初期化関数
	/// </summary>
	/// <param name="model">3Dモデル</param>
	/// <param name="camera">カメラ</param>
	void Intialize(KamataEngine::Model* model,KamataEngine::Camera* camera);

	/// <summary>
	/// 自機の更新
	/// </summary>
	void Update();

	/// <summary>
	/// 自機の描画
	/// </summary>
	void Draw();

private:
	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;

	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

	// モデル
	KamataEngine::Model* model_ = nullptr;

	// テクスチャハンドル
	uint32_t textureHandle_ = 0u;
};