#pragma once
#include "KamataEngine.h"
#include "Transform.h"

class BackgroundEnemy {
public:
	// 初期化
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	// 更新
	void Update();

	// 描画
	void Draw();

private:
	// 移動方向などを再設定
	void ResetMovement();

	// 画面外へ出た個体を反対側へ戻す
	void WrapAroundScreen();

	// 指定範囲の乱数
	static float RandomFloat(float minValue, float maxValue);

private:
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::WorldTransform worldTransform_;
	Transform transform_;

	// 1フレームごとの移動量
	KamataEngine::Vector3 velocity_{};

	// 1フレームごとの回転量
	KamataEngine::Vector3 angularVelocity_{};

	// 動きを変更するまでの時間
	float movementTimer_ = 0.0f;
	float movementChangeTime_ = 2.0f;

	// カメラ中心から見た活動範囲
	static constexpr float kAreaHalfWidth = 25.0f;
	static constexpr float kAreaHalfHeight = 14.0f;

	// 背景として配置するZ座標
	static constexpr float kMinDepth = 12.0f;
	static constexpr float kMaxDepth = 22.0f;

	// 移動速度
	static constexpr float kMinMoveSpeed = 0.015f;
	static constexpr float kMaxMoveSpeed = 0.050f;

	// 回転速度
	static constexpr float kMinRotateSpeed = 0.005f;
	static constexpr float kMaxRotateSpeed = 0.030f;

	// 背景敵の色・透明度
	KamataEngine::ObjectColor objectColor_;

	// 透明度
	static constexpr float kAlpha = 0.35f;
};