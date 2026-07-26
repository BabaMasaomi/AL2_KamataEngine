#include "HitEffect.h"
#include "Math.h"
#include <random>

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

// 静的メンバ変数の実体
Model* HitEffect::model_ = nullptr;
Camera* HitEffect::camera_ = nullptr;

/// <summary>
/// 初期化
/// </summary>
/// <param name="pos">エフェクトの発生座標</param>
void HitEffect::Initialise(Vector3 pos) {
	circleWorldTransform_.Initialize();
	// 円形エフェクト
	circleWorldTransform_.translation_ = pos;
	circleWorldTransform_.scale_ = {2.0f, 2.0f, 2.0f};

	// 乱数範囲を指定
	std::uniform_real_distribution<float> rotationDistribution(0.0f, 100.0f);
	// 楕円エフェクト
	for (WorldTransform& worldTransform : ellipseWorldTransform_) {
		worldTransform.scale_ = {4.0f, 0.3f, 2.0f};
		// 楕円エフェクトの傾きの乱数
		float ellipseRotate = rotationDistribution(randomEngine);
		worldTransform.rotation_ = {0.0f, 0.0f, ellipseRotate};
		worldTransform.translation_ = pos;

		worldTransform.Initialize();
	}
}

/// <summary>
/// 更新処理
/// </summary>
void HitEffect::UpDate() {
	//  Behavior変更
	if (behaviorRequest_ != behavior_) {
		behavior_ = behaviorRequest_;

		switch (behavior_) {
			//  エフェクト発生の初期化
		case HitEffectBehavior::kExpand:
			BehaviorExpandInitialize();
			break;
			// エフェクトフェードアウトの初期化
		case HitEffectBehavior::kFadeOut:
			BehaviorFadeOutInitialize();
			break;
		}
	}

	//
	switch (behavior_) {
		// エフェクト発生の更新
	case HitEffectBehavior::kExpand:
		BehaviorExpandUpdate();
		break;
		// エフェクトフェードアウトの更新
	case HitEffectBehavior::kFadeOut:
		BehaviorFadeOutUpdate();
		break;
	}

	// 行列を定数バッファに転送
	// 円エフェクト
	transform_.worldMatrixUpdate(circleWorldTransform_);
	// 楕円エフェクト
	for (WorldTransform& worldTransform : ellipseWorldTransform_) {
		transform_.worldMatrixUpdate(worldTransform);
	}
}

// エフェクト発生の初期化
void HitEffect::BehaviorExpandInitialize() {
	expandTimer_ = 0.0f;

	circleWorldTransform_.scale_ = {2.0f, 2.0f, 2.0f};
}

// エフェクト発生の更新
void HitEffect::BehaviorExpandUpdate() {
	// タイマーを加算(1/60秒)
	expandTimer_ += 1.0f / 60.0f;
	// イージングでScaleを変更
	float t = std::clamp(expandTimer_ / kExpandTime_, 0.0f, 1.0f);
	float s = EaseOut(2.0f, 3.5f, t);
	// スケール変更
	circleWorldTransform_.scale_ = {s, s, s};
	// フェードアウトに移行
	if (t >= 1.0f) {
		behaviorRequest_ = HitEffectBehavior::kFadeOut;
	}
}

// エフェクトフェードアウトの初期化
void HitEffect::BehaviorFadeOutInitialize() {
	fadeTimer_ = 0.0f;
	alpha_ = 1.0f;
}

// エフェクトフェードアウトの更新
void HitEffect::BehaviorFadeOutUpdate() {
	// タイマーを加算(1/60秒)
	fadeTimer_ += 1.0f / 60.0f;
	// イージングでalpha値を変更
	float t = std::clamp(fadeTimer_ / kFadeTime_, 0.0f, 1.0f);
	// 透明度を変更
	alpha_ = EaseOut(1.0f, 0.0f, t);
	// modelに反映させる
	model_->SetAlpha(alpha_);
	// フェードアウトが終わったら消す
	if (t >= 1.0f) {
		isDead_ = true;
	}
}

/// <summary>
/// 描画処理
/// </summary>
void HitEffect::Draw() {
	// モデルの描画
	// 円エフェクト
	model_->Draw(circleWorldTransform_, *camera_);
	// 楕円エフェクト
	for (WorldTransform& worldTransform : ellipseWorldTransform_) {
		model_->Draw(worldTransform, *camera_);
	}
}

/// <summary>
/// インスタンスの生成と初期化
/// </summary>
/// <param name="pos">エフェクトの発生座標</param>
/// <returns></returns>
HitEffect* HitEffect::Create(Vector3 pos) {
	// インスタンス生成
	HitEffect* instance = new HitEffect();
	// newの失敗を検出
	assert(instance);
	// インスタンスの初期化
	instance->Initialise(pos);
	// 初期化したインスタンスを返す
	return instance;
}