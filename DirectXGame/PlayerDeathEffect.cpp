#include "PlayerDeathEffect.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include <random>

using namespace KamataEngine;

void PlayerDeathEffect::Initialize(Model* model, Camera* camera) {

	assert(model);
	assert(camera);

	model_ = model;
	camera_ = camera;

	for (PlayerDeathLight& light : lights_) {
		light.worldTransform.Initialize();

		light.worldTransform.scale_ = {};
		light.velocity = {};
		light.initialScale = 1.0f;
		light.alphaRate = 1.0f;
		light.isActive = false;

		transform_.worldMatrixUpdate(light.worldTransform);
	}

	effectTimer_ = 0.0f;
	isPlaying_ = false;
	isFinished_ = true;
}

void PlayerDeathEffect::Start(const Vector3& position) {

	effectTimer_ = 0.0f;
	isPlaying_ = true;
	isFinished_ = false;

	// 毎回少し異なる散り方にする
	static std::mt19937 randomEngine{std::random_device{}()};

	std::uniform_real_distribution<float> angleOffsetDistribution(-0.18f, 0.18f);

	std::uniform_real_distribution<float> speedDistribution(kMinSpeed, kMaxSpeed);

	std::uniform_real_distribution<float> scaleDistribution(kMinScale, kMaxScale);

	std::uniform_real_distribution<float> alphaDistribution(0.70f, 1.0f);

	for (size_t i = 0; i < lights_.size(); ++i) {
		PlayerDeathLight& light = lights_[i];

		float baseAngle = std::numbers::pi_v<float> * 2.0f * static_cast<float>(i) / static_cast<float>(lights_.size());

		float angle = baseAngle + angleOffsetDistribution(randomEngine);

		float speed = speedDistribution(randomEngine);

		light.velocity = {
		    std::cos(angle) * speed,
		    std::sin(angle) * speed + 0.8f,
		    0.0f,
		};

		light.initialScale = scaleDistribution(randomEngine);

		light.alphaRate = alphaDistribution(randomEngine);

		light.worldTransform.translation_ = position;

		// プレイヤーや敵より少し手前
		light.worldTransform.translation_.z -= kFrontOffsetZ;

		light.worldTransform.rotation_ = {};

		light.worldTransform.scale_ = {
		    light.initialScale,
		    light.initialScale,
		    1.0f,
		};

		light.isActive = true;

		transform_.worldMatrixUpdate(light.worldTransform);
	}
}

void PlayerDeathEffect::Update() {
	if (!isPlaying_) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f;

	effectTimer_ += deltaTime;

	float t = std::clamp(effectTimer_ / kEffectTime, 0.0f, 1.0f);

	for (PlayerDeathLight& light : lights_) {
		if (!light.isActive) {
			continue;
		}

		light.worldTransform.translation_.x += light.velocity.x * deltaTime;

		light.worldTransform.translation_.y += light.velocity.y * deltaTime;

		// 徐々に減速
		light.velocity.x *= kVelocityAttenuation;
		light.velocity.y *= kVelocityAttenuation;

		// 最初は大きく、徐々に小さくする
		float scaleRate = 1.0f - t;
		scaleRate *= scaleRate;

		float scale = light.initialScale * scaleRate;

		light.worldTransform.scale_ = {
		    scale,
		    scale,
		    1.0f,
		};

		transform_.worldMatrixUpdate(light.worldTransform);

		if (t >= 1.0f) {
			light.isActive = false;
		}
	}

	if (t >= 1.0f) {
		isPlaying_ = false;
		isFinished_ = true;
	}
}

void PlayerDeathEffect::Draw() {
	if (!isPlaying_ || !model_ || !camera_) {
		return;
	}

	float t = std::clamp(effectTimer_ / kEffectTime, 0.0f, 1.0f);

	// 後半で急速に薄くする
	float fadeRate = 1.0f - t;
	fadeRate *= fadeRate;

	for (const PlayerDeathLight& light : lights_) {
		if (!light.isActive) {
			continue;
		}

		float alpha = kMaxAlpha * light.alphaRate * fadeRate;

		model_->SetAlpha(alpha);

		model_->Draw(light.worldTransform, *camera_);
	}

	// hitEffectModel_は共有されるため戻しておく
	model_->SetAlpha(1.0f);
}