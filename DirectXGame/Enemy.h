#pragma once
#include "KamataEngine.h"
#include "Transform.h"
#include "temporaryAABB.h"

// 前方宣言
class MapChipField;

class Player;

// 左右の向き
// enum class LRDirection {
//	kRight,
//	kLeft,
//};

// 振る舞い
enum class BehaviorEnemy {
	kRoot,
	kDeath,
	kUnknown,
};

class Enemy {
public:
	// コンストラクタ&デストラクタ
	Enemy();
	~Enemy();

	/// <summary>
	/// 敵の初期化
	/// </summary>
	/// <param name="model"></param>
	/// <param name="pos"></param>
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3 pos);

	/// <summary>
	/// 敵の更新
	/// </summary>
	void Update();

	// ルートビヘイビア用更新
	void BehaviorRootUpdate();  // 通常行動更新
	void BehaviorDeathUpdate(); // 死亡アクション更新

	// 通常行動初期化
	void BehaviorRootInitialize();
	// 死亡アクション初期化
	void BehaviorDeathInitialize();

	/// <summary>
	/// 敵の描画
	/// </summary>
	void Draw();

	/// <summary>
	/// 自機のworld座標を取得
	/// </summary>
	/// <returns></returns>
	KamataEngine::Vector3 GetWorldPos();

	/// <summary>
	/// AABBを取得
	/// </summary>
	/// <returns></returns>
	AABB GetAABB();

	/// <summary>
	/// 敵の衝突判定処理
	/// </summary>
	/// <param name="player">自機の情報</param>
	void OnCollisionPlayer(Player* player);

	// 当たり判定が無効化されているか
	bool IsCollisionDisEnabled() const;

	// ゲッター
	bool GetIsDead() const { return isDead_; }

private:
	// Translateクラス内の関数を使える様にする
	Transform transform_;

	// マップチップによるフィールド
	MapChipField* mapChipField_ = nullptr;

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;

	// モデル
	KamataEngine::Model* modelEnemy_ = nullptr;

	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

	// 以下、移動などに使う変数をまとめる
	KamataEngine::Vector3 velocity_ = {};

	// 基礎移動速度
	static inline const float kMoveSpeed = 0.05f;

	// 敵の当たり判定サイズ
	static inline const float kWidth = 1.6f;
	static inline const float kHeight = 1.6f;

	// アニメーション用変数
	// 最初の角度(度)
	static inline const float kWalkMotionAngleStart = -30.f;

	// 最後の角度(度)
	static inline const float kWalkMotionAngleEnd = 30.f;

	// アニメーション周期となる時間（秒）
	static inline const float kWalkMotionTime = 0.5f;

	// 経過時間
	float walkTimer_ = 0.0f;

	// 死んだか
	bool isDead_ = false;

	// 死亡アクション時間管理
	float deathTimer_ = 0.0f;
	static constexpr float kDeathTime = 1.0f;

	bool isCollisionDisenabled_ = false;

	/*--------------- ビヘイビア管理用 ---------------*/
	// 現在の振る舞い
	BehaviorEnemy behavior_ = BehaviorEnemy::kRoot;

	// リクエスト
	BehaviorEnemy behaviorRequest_ = BehaviorEnemy::kUnknown;
};