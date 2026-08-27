#pragma once

#include "KamataEngine.h"
#include "Transform.h"

#include <array>

struct PlayerDeathLight {
	KamataEngine::WorldTransform worldTransform;

	KamataEngine::Vector3 velocity = {};

	float initialScale = 1.0f;
	float alphaRate = 1.0f;

	bool isActive = false;
};

class PlayerDeathEffect {
public:
	// 初期化
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera);

	// 演出開始
	void Start(const KamataEngine::Vector3& position);

	// 更新
	void Update();

	// 描画
	void Draw();

	bool IsFinished() const { return isFinished_; }

private:
	Transform transform_;

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	static constexpr size_t kLightCount = 18;

	std::array<PlayerDeathLight, kLightCount> lights_{};

	float effectTimer_ = 0.0f;

	bool isPlaying_ = false;
	bool isFinished_ = true;

	// 演出全体の時間
	static constexpr float kEffectTime = 0.85f;

	// 光の最低・最高速度
	static constexpr float kMinSpeed = 4.5f;
	static constexpr float kMaxSpeed = 12.0f;

	// 光の大きさ
	static constexpr float kMinScale = 0.55f;
	static constexpr float kMaxScale = 0.95f;

	// 最大透明度
	static constexpr float kMaxAlpha = 0.75f;

	// 移動速度の減衰率
	static constexpr float kVelocityAttenuation = 0.965f;

	// プレイヤーより手前へ出す距離
	static constexpr float kFrontOffsetZ = 0.25f;
};