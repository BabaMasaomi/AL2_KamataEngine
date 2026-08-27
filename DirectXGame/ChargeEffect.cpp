#include "ChargeEffect.h"

#include <algorithm>
#include <cassert>
#include <cmath>

using namespace KamataEngine;

namespace {

// 0～1のEaseOut
float EaseOutCubic(float t) {
	t = std::clamp(t, 0.0f, 1.0f);

	float inverse = 1.0f - t;
	return 1.0f - inverse * inverse * inverse;
}

} // namespace

void ChargeEffect::Initialize(Model* model, Camera* camera) {
	assert(model);
	assert(camera);

	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();

	worldTransform_.scale_ = {
	    kStartScale,
	    kStartScale,
	    1.0f,
	};

	alpha_ = 0.0f;
	fadeStartAlpha_ = 0.0f;

	fadeStartScale_ = kEndScale;

	isVisible_ = false;
	wasCharging_ = false;
	pulseTimer_ = 0.0f;
	fadeTimer_ = 0.0f;

	transform_.worldMatrixUpdate(worldTransform_);
}

void ChargeEffect::Update(bool isCharging, bool isChargeReady, float chargeRatio, const Vector3& position) {

	const float deltaTime = 1.0f / 60.0f;

	// プレイヤーの手元を追従
	worldTransform_.translation_ = position;

	if (isCharging) {

		// 溜め始め
		if (!wasCharging_) {
			isVisible_ = true;
			pulseTimer_ = 0.0f;
			fadeTimer_ = 0.0f;

			// 押した瞬間は見えないようにする
			alpha_ = 0.0f;
			fadeStartAlpha_ = 0.0f;
		}

		chargeRatio = std::clamp(chargeRatio, 0.0f, 1.0f);

		float easedRatio = EaseOutCubic(chargeRatio);

		float scale = kStartScale + (kEndScale - kStartScale) * easedRatio;

		if (isChargeReady) {

			pulseTimer_ += deltaTime;

			// 溜め完了後は収束位置で小さく脈動
			float pulse = std::sin(pulseTimer_ * kPulseSpeed) * kPulseScale;

			scale = kEndScale + pulse;

			// 溜め完了を少し強く表示
			alpha_ = 0.88f + std::sin(pulseTimer_ * kPulseSpeed) * 0.10f;

		} else {

			pulseTimer_ = 0.0f;

			if (chargeRatio < kVisibleStartRatio) {

				// 通常攻撃の短押し中は表示しない
				alpha_ = 0.0f;

			} else {

				// 表示開始地点を0、溜め成立地点を1に変換
				float visibleRatio = (chargeRatio - kVisibleStartRatio) / (1.0f - kVisibleStartRatio);

				visibleRatio = std::clamp(visibleRatio, 0.0f, 1.0f);

				// 最初は薄く、溜め成立へ近づくほど濃くする
				alpha_ = kStartAlpha + (kReadyAlpha - kStartAlpha) * EaseOutCubic(visibleRatio);
			}
		}

		worldTransform_.scale_ = {
		    scale,
		    scale,
		    1.0f,
		};

	} else if (wasCharging_) {

		// 現在の薄さを維持したままフェードを開始
		fadeStartAlpha_ = alpha_;
		fadeStartScale_ = worldTransform_.scale_.x;
		fadeTimer_ = kFadeOutTime;

	} else if (isVisible_) {

		fadeTimer_ -= deltaTime;

		float fadeRatio = std::clamp(fadeTimer_ / kFadeOutTime, 0.0f, 1.0f);

		alpha_ = fadeStartAlpha_ * fadeRatio;

		// 消えながら少し広げる
		float fadeScale = fadeStartScale_ + (1.0f - fadeRatio) * 0.20f;

		worldTransform_.scale_ = {
		    fadeScale,
		    fadeScale,
		    1.0f,
		};

		if (fadeTimer_ <= 0.0f) {
			fadeTimer_ = 0.0f;
			alpha_ = 0.0f;
			isVisible_ = false;
		}
	}

	wasCharging_ = isCharging;

	transform_.worldMatrixUpdate(worldTransform_);
}

void ChargeEffect::Draw() {
	if (!isVisible_ || !model_ || !camera_) {
		return;
	}

	model_->SetAlpha(std::clamp(alpha_, 0.0f, 1.0f));

	model_->Draw(worldTransform_, *camera_);
}