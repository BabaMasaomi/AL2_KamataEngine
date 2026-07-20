#include "HitEffect.h"

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
}

/// <summary>
/// 更新処理
/// </summary>
void HitEffect::UpDate() {
	// 行列を定数バッファに転送
	transform_.worldMatrixUpdate(circleWorldTransform_);
}

/// <summary>
/// 描画処理
/// </summary>
void HitEffect::Draw() {
	// モデルの描画
	model_->Draw(circleWorldTransform_, *camera_);
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