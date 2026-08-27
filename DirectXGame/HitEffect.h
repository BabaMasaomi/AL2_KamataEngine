#pragma once
#include "KamataEngine.h"
#include "Transform.h"

// エフェクトの種類
enum class HitEffectType {
	kHit,        // 汎用・敵同士の衝突・死亡演出
	kNormalHit,  // プレイヤーの通常攻撃
	kChargedHit, // 溜め攻撃用
	kGuard,

	kBounceWall,       // 画面の左右端
	kBounceHorizontal, // 画面の上下端

	kPlayerDamage, // プレイヤー被ダメージ
};

// 死亡演出の管理
enum class HitEffectBehavior {
	kExpand,  // 拡大
	kFadeOut, // フェードアウト
};

class HitEffect {
public:
	void Initialise(KamataEngine::Vector3 pos, HitEffectType type);
	void UpDate();

	// エフェクトBehavior初期化
	void BehaviorExpandInitialize();  // エフェクト発生
	void BehaviorFadeOutInitialize(); // エフェクトフェードアウト
	// エフェクトBehavior更新
	void BehaviorExpandUpdate();  // エフェクト発生
	void BehaviorFadeOutUpdate(); // エフェクトフェードアウト

	void Draw();

	// インスタンスの生成と初期化
	static HitEffect* Create(KamataEngine::Vector3 pos, HitEffectType type);

	// ゲッター
	bool GetIsDead() const { return isDead_; }

	// セッター
	static void SetHitModel(KamataEngine::Model* model) { hitModel_ = model; }
	static void SetGuardModel(KamataEngine::Model* model) { guardModel_ = model; }
	static void SetCamera(KamataEngine::Camera* camera) { camera_ = camera; }
	static void SetPlayerDamageModel(KamataEngine::Model* model) { playerDamageModel_ = model; }

private:
	// Translateクラス内の関数を使える様にする
	Transform transform_;

	// モデル(借りてくる用)
	// 総合用
	static KamataEngine::Model* model_;
	// ヒット用
	static KamataEngine::Model* hitModel_;
	// ガード用
	static KamataEngine::Model* guardModel_;
	// カメラ(借りてくる用
	static KamataEngine::Camera* camera_;

	// 円のワールドトランスフォーム
	KamataEngine::WorldTransform circleWorldTransform_;
	// 楕円のワールドトランスフォーム
	std::array<KamataEngine::WorldTransform, 2> ellipseWorldTransform_;

	// 発生用タイマー
	float expandTimer_ = 0.0f;
	static constexpr float kExpandTime_ = 0.1f;
	// フェードアウト用タイマー
	float fadeTimer_ = 0.0f;
	static constexpr float kFadeTime_ = 0.3f;
	// 透明度
	float alpha_ = 1.0f;
	// エフェクトごとのフェード開始透明度
	float startAlpha_ = 1.0f;

	// 消失フラグ
	bool isDead_ = false;

	/*--------------- プレイヤー被ダメージ用 ---------------*/
	static KamataEngine::Model* playerDamageModel_;

	static constexpr float kPlayerDamageStartScale = 0.8f;
	static constexpr float kPlayerDamageEndScale = 4.6f;
	static constexpr float kPlayerDamageExpandTime = 0.08f;
	static constexpr float kPlayerDamageFadeTime = 0.18f;
	static constexpr float kPlayerDamageAlpha = 0.85f;

	/*--------------- 通常攻撃命中用 ---------------*/
	// 円の初期サイズ
	static constexpr float kNormalHitInitialScale = 1.25f;

	// 円の最大サイズ
	static constexpr float kNormalHitEndScale = 2.20f;

	// 拡大時間
	static constexpr float kNormalHitExpandTime = 0.06f;

	// フェード時間
	static constexpr float kNormalHitFadeTime = 0.14f;

	// 最大透明度
	static constexpr float kNormalHitAlpha = 0.55f;

	// 放射線の長さと太さ
	static constexpr float kNormalHitLineLength = 2.40f;
	static constexpr float kNormalHitLineWidth = 0.16f;

	/*--------------- 溜め攻撃用 ---------------*/
	// ヒットストップ開始時点のサイズ
	static constexpr float kChargedHitInitialScale = 5.0f;

	// 消える直前のサイズ
	static constexpr float kChargedHitEndScale = 7.0f;

	// フェード時間
	static constexpr float kChargedHitFadeTime = 0.26f;

	// 最大透明度
	static constexpr float kChargedHitAlpha = 1.0f;

	// 放射線の長さと太さ
	static constexpr float kChargedHitLineLength = 8.0f;
	static constexpr float kChargedHitLineWidth = 0.65f;

	/*--------------- 跳ね返りエフェクト用 ---------------*/
	// 拡大時間
	static constexpr float kBounceExpandTime = 0.07f;

	// フェード時間
	static constexpr float kBounceFadeTime = 0.16f;

	// 左右端用
	static constexpr float kBounceWallStartScaleX = 0.35f;
	static constexpr float kBounceWallStartScaleY = 1.20f;
	static constexpr float kBounceWallEndScaleX = 0.75f;
	static constexpr float kBounceWallEndScaleY = 2.60f;

	// 上下端用
	static constexpr float kBounceHorizontalStartScaleX = 1.20f;
	static constexpr float kBounceHorizontalStartScaleY = 0.35f;
	static constexpr float kBounceHorizontalEndScaleX = 2.60f;
	static constexpr float kBounceHorizontalEndScaleY = 0.75f;

	/*--------------- ビヘイビア管理用 ---------------*/
	// エフェクトの種類
	HitEffectType effectType_ = HitEffectType::kHit;

	// 現在の振る舞い
	HitEffectBehavior behavior_ = HitEffectBehavior::kExpand;
	// リクエスト
	HitEffectBehavior behaviorRequest_ = HitEffectBehavior::kExpand;
};