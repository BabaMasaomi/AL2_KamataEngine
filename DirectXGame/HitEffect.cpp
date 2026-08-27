#include "HitEffect.h"
#include "Math.h"
#include <random>

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

// 静的メンバ変数の実体
Model* HitEffect::model_ = nullptr;
Model* HitEffect::hitModel_ = nullptr;
Model* HitEffect::guardModel_ = nullptr;
Camera* HitEffect::camera_ = nullptr;

/// <summary>
/// 初期化
/// </summary>
/// <param name="pos">エフェクトの発生座標</param>
void HitEffect::Initialise(Vector3 pos, HitEffectType type) {
	// エフェクト種類
	effectType_ = type;

	// 初期状態
	expandTimer_ = 0.0f;
	fadeTimer_ = 0.0f;

	// エフェクトの種類によって透明度を変える
	switch (effectType_) {
	case HitEffectType::kNormalHit:
		startAlpha_ = kNormalHitAlpha;
		break;

	case HitEffectType::kChargedHit:
		startAlpha_ = kChargedHitAlpha;
		break;

	default:
		startAlpha_ = 1.0f;
		break;
	}

	alpha_ = startAlpha_;
	isDead_ = false;

	circleWorldTransform_.Initialize();
	circleWorldTransform_.translation_ = pos;

	// スケール分岐
	if (effectType_ == HitEffectType::kChargedHit) {

		circleWorldTransform_.scale_ = {
		    kChargedHitInitialScale,
		    kChargedHitInitialScale,
		    kChargedHitInitialScale,
		};

		// ヒットストップ前に最大表示させるため、
		// 拡大を省略してフェードから開始
		behavior_ = HitEffectBehavior::kFadeOut;
		behaviorRequest_ = HitEffectBehavior::kFadeOut;

	} else if (effectType_ == HitEffectType::kNormalHit) {

		circleWorldTransform_.scale_ = {
		    kNormalHitInitialScale,
		    kNormalHitInitialScale,
		    kNormalHitInitialScale,
		};

		behavior_ = HitEffectBehavior::kExpand;
		behaviorRequest_ = HitEffectBehavior::kExpand;

	} else if (effectType_ == HitEffectType::kBounceWall) {

		circleWorldTransform_.scale_ = {
		    kBounceWallStartScaleX,
		    kBounceWallStartScaleY,
		    1.0f,
		};

		behavior_ = HitEffectBehavior::kExpand;
		behaviorRequest_ = HitEffectBehavior::kExpand;

	} else if (effectType_ == HitEffectType::kBounceHorizontal) {

		circleWorldTransform_.scale_ = {
		    kBounceHorizontalStartScaleX,
		    kBounceHorizontalStartScaleY,
		    1.0f,
		};

		behavior_ = HitEffectBehavior::kExpand;
		behaviorRequest_ = HitEffectBehavior::kExpand;

	} else {

		// 既存の汎用ヒット・ガード
		circleWorldTransform_.scale_ = {
		    2.0f,
		    2.0f,
		    2.0f,
		};

		behavior_ = HitEffectBehavior::kExpand;
		behaviorRequest_ = HitEffectBehavior::kExpand;
	}

	std::uniform_real_distribution<float> rotationDistribution(0.0f, 100.0f);

	for (WorldTransform& worldTransform : ellipseWorldTransform_) {

		// Initializeを先に行う
		worldTransform.Initialize();

		if (effectType_ == HitEffectType::kChargedHit) {

			worldTransform.scale_ = {
			    kChargedHitLineLength,
			    kChargedHitLineWidth,
			    2.0f,
			};

		} else if (effectType_ == HitEffectType::kNormalHit) {

			worldTransform.scale_ = {
			    kNormalHitLineLength,
			    kNormalHitLineWidth,
			    1.0f,
			};

		} else {

			worldTransform.scale_ = {
			    4.0f,
			    0.3f,
			    2.0f,
			};
		}

		float ellipseRotate = rotationDistribution(randomEngine);

		worldTransform.rotation_ = {0.0f, 0.0f, ellipseRotate};

		worldTransform.translation_ = pos;
	}

	/*
	 * 生成直後にヒットストップへ入っても正しく描画できるよう、
	 * この時点で行列を更新する。
	 */
	transform_.worldMatrixUpdate(circleWorldTransform_);

	for (WorldTransform& worldTransform : ellipseWorldTransform_) {

		transform_.worldMatrixUpdate(worldTransform);
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

	float expandTime = 0.1f;

	if (effectType_ == HitEffectType::kNormalHit) {

		expandTime = kNormalHitExpandTime;

	} else if (effectType_ == HitEffectType::kGuard) {

		expandTime = 0.04f;

	} else if (effectType_ == HitEffectType::kBounceWall || effectType_ == HitEffectType::kBounceHorizontal) {

		expandTime = kBounceExpandTime;
	}

	expandTimer_ += 1.0f / 60.0f;

	float t = std::clamp(expandTimer_ / expandTime, 0.0f, 1.0f);

	if (effectType_ == HitEffectType::kNormalHit) {

		float scale = EaseOut(kNormalHitInitialScale, kNormalHitEndScale, t);

		circleWorldTransform_.scale_ = {
		    scale,
		    scale,
		    scale,
		};

	} else if (effectType_ == HitEffectType::kHit) {

		float scale = EaseOut(2.0f, 3.5f, t);

		circleWorldTransform_.scale_ = {
		    scale,
		    scale,
		    scale,
		};

	} else if (effectType_ == HitEffectType::kBounceWall) {

		float scaleX = EaseOut(kBounceWallStartScaleX, kBounceWallEndScaleX, t);
		float scaleY = EaseOut(kBounceWallStartScaleY, kBounceWallEndScaleY, t);

		circleWorldTransform_.scale_ = {scaleX, scaleY, 1.0f};

	} else if (effectType_ == HitEffectType::kBounceHorizontal) {

		float scaleX = EaseOut(kBounceHorizontalStartScaleX, kBounceHorizontalEndScaleX, t);
		float scaleY = EaseOut(kBounceHorizontalStartScaleY, kBounceHorizontalEndScaleY, t);

		circleWorldTransform_.scale_ = {scaleX, scaleY, 1.0f};

	} else {

		float scale = EaseOut(0.5f, 5.0f, t);
		circleWorldTransform_.scale_ = {scale, scale, scale};
	}

	if (t >= 1.0f) {
		behaviorRequest_ = HitEffectBehavior::kFadeOut;
	}
}

// エフェクトフェードアウトの初期化
void HitEffect::BehaviorFadeOutInitialize() {
	fadeTimer_ = 0.0f;
	alpha_ = startAlpha_;
}

// エフェクトフェードアウトの更新
void HitEffect::BehaviorFadeOutUpdate() {
	float fadeTime = 0.0f;

	if (effectType_ == HitEffectType::kChargedHit) {

		fadeTime = kChargedHitFadeTime;

	} else if (effectType_ == HitEffectType::kNormalHit) {

		fadeTime = kNormalHitFadeTime;

	} else if (effectType_ == HitEffectType::kHit) {

		fadeTime = 0.3f;

	} else if (effectType_ == HitEffectType::kBounceWall || effectType_ == HitEffectType::kBounceHorizontal) {

		fadeTime = kBounceFadeTime;

	} else {

		fadeTime = 0.12f;
	}

	fadeTimer_ += 1.0f / 60.0f;

	float t = std::clamp(fadeTimer_ / fadeTime, 0.0f, 1.0f);

	// 徐々に透明にする
	alpha_ = EaseIn(startAlpha_, 0.0f, t);

	// 溜め攻撃は消えながら少しだけ広がる
	if (effectType_ == HitEffectType::kChargedHit) {

		float scale = EaseOut(kChargedHitInitialScale, kChargedHitEndScale, t);

		circleWorldTransform_.scale_ = {scale, scale, scale};
	}

	if (effectType_ == HitEffectType::kBounceWall) {

		float scaleX = EaseOut(kBounceWallEndScaleX, kBounceWallEndScaleX * 1.25f, t);

		float scaleY = EaseOut(kBounceWallEndScaleY, kBounceWallEndScaleY * 1.25f, t);

		circleWorldTransform_.scale_ = {scaleX, scaleY, 1.0f};

	} else if (effectType_ == HitEffectType::kBounceHorizontal) {

		float scaleX = EaseOut(kBounceHorizontalEndScaleX, kBounceHorizontalEndScaleX * 1.25f, t);

		float scaleY = EaseOut(kBounceHorizontalEndScaleY, kBounceHorizontalEndScaleY * 1.25f, t);

		circleWorldTransform_.scale_ = {scaleX, scaleY, 1.0f};
	}

	if (t >= 1.0f) {
		isDead_ = true;
	}
}

/// <summary>
/// 描画処理
/// </summary>
void HitEffect::Draw() {
	// 溜め攻撃も通常ヒットと同じモデルを使う
	if (effectType_ == HitEffectType::kHit || effectType_ == HitEffectType::kNormalHit || effectType_ == HitEffectType::kChargedHit) {

		model_ = hitModel_;

	} else {

		model_ = guardModel_;
	}

	// このエフェクトの透明度を描画直前に反映
	model_->SetAlpha(alpha_);

	model_->Draw(circleWorldTransform_, *camera_);

	if (effectType_ == HitEffectType::kNormalHit) {

		// 通常攻撃は控えめに1本だけ
		model_->Draw(ellipseWorldTransform_[0], *camera_);

	} else if (effectType_ == HitEffectType::kHit || effectType_ == HitEffectType::kChargedHit) {

		for (WorldTransform& worldTransform : ellipseWorldTransform_) {

			model_->Draw(worldTransform, *camera_);
		}
	}
}

/// <summary>
/// インスタンスの生成と初期化
/// </summary>
/// <param name="pos">エフェクトの発生座標</param>
/// <returns></returns>
HitEffect* HitEffect::Create(Vector3 pos, HitEffectType type) {
	// インスタンス生成
	HitEffect* instance = new HitEffect();
	// newの失敗を検出
	assert(instance);
	// インスタンスの初期化
	instance->Initialise(pos, type);
	// 初期化したインスタンスを返す
	return instance;
}