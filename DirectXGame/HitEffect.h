#pragma once
#include "KamataEngine.h"
#include "Transform.h"

// 死亡演出の管理
enum class HitEffectBehavior {
	kExpand,  // 拡大
	kFadeOut, // フェードアウト
};

class HitEffect {
public:
	void Initialise(KamataEngine::Vector3 pos);
	void UpDate();

	// エフェクトBehavior初期化
	void BehaviorExpandInitialize();	// エフェクト発生
	void BehaviorFadeOutInitialize();	// エフェクトフェードアウト
	// エフェクトBehavior更新
	void BehaviorExpandUpdate();	// エフェクト発生
	void BehaviorFadeOutUpdate();	// エフェクトフェードアウト

	void Draw();

	// インスタンスの生成と初期化
	static HitEffect* Create(KamataEngine::Vector3 pos);

	// ゲッター
	bool GetIsDead() const { return isDead_; }

	// セッター
	static void SetModel(KamataEngine::Model* model) { model_ = model; }
	static void SetCamera(KamataEngine::Camera* camera) { camera_ = camera; }

private:
	// Translateクラス内の関数を使える様にする
	Transform transform_;

	// モデル(借りてくる用)
	static KamataEngine::Model* model_;
	// カメラ(借りてくる用
	static KamataEngine::Camera* camera_;

	// 円のワールドトランスフォーム
	KamataEngine::WorldTransform circleWorldTransform_;
	// 楕円のワールドトランスフォーム
	std::array<KamataEngine::WorldTransform, 2> ellipseWorldTransform_;

	// 発生用タイマー
	float expandTimer_;
	static constexpr float kExpandTime_ = 0.1f;
	// フェードアウト用タイマー
	float fadeTimer_;
	static constexpr float kFadeTime_ = 0.3f;
	// 透明度
	float alpha_ = 1.0f;

	// 消失フラグ
	bool isDead_ = false;

	/*--------------- ビヘイビア管理用 ---------------*/
	// 現在の振る舞い
	HitEffectBehavior behavior_ = HitEffectBehavior::kExpand;
	// リクエスト
	HitEffectBehavior behaviorRequest_ = HitEffectBehavior::kExpand;
};