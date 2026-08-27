#pragma once

#include "KamataEngine.h"
#include "Transform.h"

class ChargeEffect {
public:
	// 初期化
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera);

	// 更新
	void Update(bool isCharging, bool isChargeReady, float chargeRatio, const KamataEngine::Vector3& position);

	// 描画
	void Draw();

private:
	Transform transform_;

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	KamataEngine::WorldTransform worldTransform_;

	// 前フレームでも溜めていたか
	bool wasCharging_ = false;

	// 描画中か
	bool isVisible_ = false;

	// 溜め完了後の脈動時間
	float pulseTimer_ = 0.0f;

	// フェード時間
	float fadeTimer_ = 0.0f;

	// 透明度
	float alpha_ = 0.0f;

	// 収束前の大きさ
	static constexpr float kStartScale = 5.0f;

	// 収束後の大きさ
	static constexpr float kEndScale = 1.15f;

	// フェードアウト時間
	static constexpr float kFadeOutTime = 0.10f;

	// 溜め完了後の脈動幅
	static constexpr float kPulseScale = 0.12f;

	// 脈動速度
	static constexpr float kPulseSpeed = 12.0f;
};