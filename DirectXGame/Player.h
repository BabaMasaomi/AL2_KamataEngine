#pragma once
#include "KamataEngine.h"
#include "BasicCharacterComposition.h"
#include "Transform.h"
#include "temporaryAABB.h"

// 前方宣言
class MapChipField;

class Enemy;
class ShieldEnemy;

// マップとの当たり判定情報
struct CollisionMapInfo {
	bool isCeilingCollide = false;             // 天井衝突フラグ
	bool isLanding = false;                    // 着地フラグ
	bool isWallCollide = false;                // 壁衝突フラグ
	KamataEngine::Vector3 MovementAmount = {}; // 移動量
};

// 角
enum Corner {
	kRightBottom, // 右下
	kLeftBottom,  // 左下
	kRightTop,    // 右上
	kLeftTop,     // 左上

	kNumCorner // 要素数
};

// 振る舞い
enum class Behavior {
	kRoot,		// 通常状態
	kAttack,	// 攻撃中
	kKnockBack,	// ノックバック
	kUnKnown,	// 変更リクエスト無し
};

// 攻撃の種類
enum class AttackType {
	kNormal,  // バットの通常スイング
	kCharged, // 行動不能の敵を吹き飛ばす強打
};

// 通常攻撃のフェーズ(型)
enum class AttackPhase {
	kStartup,  // 振りかぶり
	kActive,   // バットを振る・攻撃判定あり
	kRecovery, // 後隙
	kNone,   // 攻撃してない
};

class Player {
public:
	// コンストラクタ&デストラクタ
	Player();
	~Player();

	/// <summary>
	/// 自機の初期化
	/// </summary>
	/// <param name="model">3Dモデル</param>
	/// <param name="camera">カメラ</param>
	void Initialize(KamataEngine::Model* model, KamataEngine::Model* modelAttack, KamataEngine::Camera* camera, const KamataEngine::Vector3 pos);

	/// <summary>
	/// 自機の更新
	/// </summary>
	void Update();

	// ルートビヘイビア用更新
	void BehaviorRootUpdate();		// 通常行動更新
	void BehaviorAttackUpdate();	// 攻撃行動更新
	void BehaviorKnockBackUpdate(); // ノックバック更新

	// 通常行動初期化
	void BehaviorRootInitialize();
	// 攻撃行動初期化
	void BehaviorAttackInitialize();
	// ノックバック初期化
	void BehaviorKnockBackInitialize();

	/// <summary>
	/// 自機の描画
	/// </summary>
	void Draw();

	/// <summary>
	/// マップ衝突判定
	/// </summary>
	/// <param name="info">マップとの当たり判定情報</param>
	void MapCollisionCheck(CollisionMapInfo& info);

	/// <summary>
	/// 指定した角の座標計算
	/// </summary>
	/// <param name="center">指定したい角の矩形の中心座標</param>
	/// <param name="corner">指定した角</param>
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner);

	// 方向別のマップ衝突判定
	void MapCollisionCheckTop(CollisionMapInfo& info);
	void MapCollisionCheckBottom(CollisionMapInfo& info);
	void MapCollisionCheckRight(CollisionMapInfo& info);
	void MapCollisionCheckLeft(CollisionMapInfo& info);

	/// <summary>
	/// 判定結果を反映させて移動させる
	/// </summary>
	/// <param name="info">マップとの当たり判定情報</param>
	void MoveReflectingResult(const CollisionMapInfo& info);

	/// <summary>
	/// 天井に接触している時の処理
	/// </summary>
	/// <param name="info">マップとの当たり判定情報</param>
	void ContactWithCeiling(const CollisionMapInfo& info);

	/// <summary>
	/// 壁に接触している時の処理
	/// </summary>
	/// <param name="info">マップとの当たり判定情報</param>
	void ContactWithWall(const CollisionMapInfo& info);

	/// <summary>
	/// 接地状態の切り替え
	/// </summary>
	/// <param name="info">マップとの当たり判定情報</param>
	void SwitchGroundingState(const CollisionMapInfo& info);

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
	/// 攻撃用のAABBを取得
	/// </summary>
	/// <returns></returns>
	AABB GetAttackAABB() const;

	/// <summary>
	/// 自機の衝突判定処理
	/// </summary>
	/// <param name="enemy">敵の情報</param>
	void OnCollisionEnemy(Enemy* enemy);

	/// <summary>
	/// 自機の衝突判定処理
	/// </summary>
	/// <param name="enemy">敵の情報</param>
	void OnCollisionShieldEnemy(ShieldEnemy* enemy);

	// 攻撃中かどうかを判定する
	bool IsAttack();

	// 敵を攻撃できるか
	bool CanAttackEnemy() const;
	// ダメージを受けるか
	bool CanReceiveDamage() const;

	// ノックバック要求を受け取る
	void RequestKnockBack(float direction);

	// 攻撃を終わらせる
	void EndAttack();

	// アクセッサ
	// ゲッター
	bool GetIsDead() { return isDead_; }
	
	// 平行移動した位置
	KamataEngine::WorldTransform& GetWorldTransform() { return worldTransform_; }

	// 速度
	const KamataEngine::Vector3& GetVeloctiy() const { return velocity_; }

	// 向き
	LRDirection GetLRDirection() const { return lrDirection_; }

	// 現在の攻撃を識別する番号
	uint32_t GetAttackSerial() const { return attackSerial_; }

	// セッター
	// 自機のワールド座標
	void SetWorldPos(const KamataEngine::Vector3& pos) { worldTransform_.translation_ = pos; }

	// マップチップ情報
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

private:
	// Translateクラス内の関数を使える様にする
	Transform transform_;

	// マップチップによるフィールド
	MapChipField* mapChipField_ = nullptr;

	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;

	// モデル
	KamataEngine::Model* model_ = nullptr;

	// 以下、移動などに使う変数をまとめる
	KamataEngine::Vector3 velocity_ = {};

	// 左右移動の加速度
	static inline const float kAcceleration = 0.025f;

	// 移動減衰の基本の値
	static inline const float kAttenuation = 0.1f;

	// 着地時の減衰の基本の値
	static inline const float kAttenuationLanding = 0.1f;

	// 制限速度
	static inline const float kLimitRunSpeed = 0.75f;

	// 左右の向き
	LRDirection lrDirection_ = LRDirection::kRight;

	// 接地フラグ
	bool onGround_ = true;

	// 重力加速度
	static inline const float kGravityAcceleration = 0.09f;

	// 最大落下速度
	static inline const float kLimitFallSpeed_ = 0.75f;

	// ジャンプ初速
	static inline const float kJumpAcceleration_ = 1.0f;

	// 壁にぶつかった時の減速率
	static inline const float kAttenuationWall = 0.75f;

	// 死亡フラグ
	bool isDead_ = false;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 1.6f;
	static inline const float kHeight = 1.6f;

	// ブロックとの間にとる余白
	static inline const float kMargin = 0.05f;

	// 旋回開始の角度
	float turnFirstRotationY_ = 0.0f;

	// 旋回タイマー
	float turnTimer_ = 0.0f;

	// 旋回時間(秒)
	static inline const float kTimeTurn = 0.3f;

	/*--------------- 通常攻撃用 ---------------*/
	// 各フェーズのタイマー
	float attackTimer_ = 0.0f;

	// 振りかぶり時間
	static constexpr float kAttackStartupTime = 0.08f;

	// 攻撃判定が出る時間
	static constexpr float kAttackActiveTime = 0.12f;

	// 後隙
	static constexpr float kAttackRecoveryTime = 0.15f;

	// 踏み込み速度
	static constexpr float kAttackStepSpeed = 0.22f;

	// バットの振り始めと振り終わり
	static constexpr float kBatAngleStart = -70.0f;
	static constexpr float kBatAngleEnd = 70.0f;

	// 攻撃判定の大きさ
	static constexpr float kAttackWidth = 3.6f;
	static constexpr float kAttackHeight = 2.6f;

	// プレイヤー中心から攻撃判定までの距離
	static constexpr float kAttackOffsetX = 1.8f;

	// 1回の攻撃で同じ敵へ複数回当てないためのフラグ
	bool hasHitEnemy_ = false;

	// 現在の攻撃フェーズ
	AttackPhase attackPhase_ = AttackPhase::kStartup;

	// 空中で攻撃可能か
	bool canAirAttack_ = true;

	// 攻撃を開始するたびに増える識別番号
	uint32_t attackSerial_ = 0;

	// バット用モデル
	KamataEngine::Model* modelBat_ = nullptr;
	KamataEngine::WorldTransform worldTransformBat_;
	bool isBatVisible_ = false;

	/*--------------- ノックバック用 ---------------*/
	// ノックバック方向
	float knockBackDirection_ = 0.0f;

	// タイマー
	float knockBackTimer_ = 0.0f;

	static constexpr float kKnockBackTime = 0.2f;
	static constexpr float kKnockBackSpeed = 1.2f;

	// 攻撃エフェクトモデル
	KamataEngine::Model* modelAttack_ = nullptr;
	// エフェクト用ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransformAttack_;
	// エフェクト表示フラグ
	bool isAttackEffect_ = false;

	/*--------------- ビヘイビア管理用 ---------------*/
	// 振る舞い
	Behavior behaivior_ = Behavior::kRoot;

	// 振る舞いのリクエスト
	Behavior behaiviorRequest_ = Behavior::kUnKnown;
};