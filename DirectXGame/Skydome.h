#pragma once
#include "KamataEngine.h"

class Skydome {
public:
	// コンストラクタ&デストラクタ
	Skydome();
	~Skydome();

	/// <summary>
	/// 天球の初期化
	/// </summary>
	/// <param name="model">3Dモデル</param>
	/// <param name="camera">カメラ</param>
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera);

	/// <summary>
	/// 天球の更新
	/// </summary>
	void Update();

	/// <summary>
	/// 天球の描画
	/// </summary>
	void Draw();

	// 天球の回転速度を設定
	void SetRotationSpeed(float speed) { rotationSpeed_ = speed; }

private:
	// カメラ
	KamataEngine::Camera* cameraSkydome_ = nullptr;

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransformSkydome_;

	// モデル
	KamataEngine::Model* modelSkydome_ = nullptr;

	// 1フレームごとのY回転量
	float rotationSpeed_ = 0.0f;
};
