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
	kCharging,	// ボタン長押しを計測
	kStartup,	// 振りかぶり
	kActive,	// バットを振る・攻撃判定あり
	kRecovery,	// 後隙
	kNone,		// 攻撃してない
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
	// 通常行動初期化
	void BehaviorRootInitialize();
	void BehaviorRootUpdate();		// 通常行動更新

	// 攻撃行動初期化
	void BehaviorAttackInitialize();
	void BehaviorAttackUpdate();	// 攻撃行動更新

	// ノックバック初期化
	void BehaviorKnockBackInitialize();
	void BehaviorKnockBackUpdate(); // ノックバック更新

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

	// 接地しているか
	bool IsOnGround() const { return onGround_; }

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

	// 死亡演出を開始
	void StartDeathAnimation();

	// 死亡演出を更新
	void UpdateDeathAnimation();

	// 死亡演出が終了したか
	bool IsDeathAnimationFinished() const { return isDeathAnimationFinished_; }

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

	AttackType GetAttackType() const { return attackType_; }

	bool IsChargedAttack() const { return attackType_ == AttackType::kCharged; }

	int32_t GetHp() const { return hp_; }
	int32_t GetMaxHp() const { return kMaxHp; }

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

	//
	// 移動減衰の基本の値
	static inline const float kAttenuation = 0.1f;
	//

	// 地上で入力中に速度を変化させる量
	static constexpr float kGroundAcceleration = 0.09f;

	// 地上で入力を離したときの減速量
	static constexpr float kGroundDeceleration = 0.04f;

	// 空中での左右制御量
	static constexpr float kAirAcceleration = 0.055f;

	// 最大左右移動速度
	static constexpr float kLimitRunSpeed = 0.50f;

	// 着地時の減衰の基本の値
	static inline const float kAttenuationLanding = 0.1f;

	// 左右の向き
	LRDirection lrDirection_ = LRDirection::kRight;

	// 接地フラグ
	bool onGround_ = true;

	// 重力加速度
	static inline const float kGravityAcceleration = 0.07f;

	// 最大落下速度
	static inline const float kLimitFallSpeed_ = 0.75f;

	// ジャンプ初速
	static inline const float kJumpAcceleration_ = 1.1f;

	// 壁にぶつかった時の減速率
	static inline const float kAttenuationWall = 0.75f;

	// キャラクターの当たり判定サイズ
	static constexpr float kWidth = 1.6f;
	static constexpr float kHeight = 1.6f;

	// ブロックとの間にとる余白
	static inline const float kMargin = 0.05f;

	// 旋回開始の角度
	float turnFirstRotationY_ = 0.0f;

	// 旋回タイマー
	float turnTimer_ = 0.0f;

	// 旋回時間(秒)
	static inline const float kTimeTurn = 0.3f;

	// 空中追加ジャンプが残っているか
	bool canDoubleJump_ = true;

	// 空中追加ジャンプの初速
	static constexpr float kDoubleJumpAcceleration = 1.00f;

	/*--------------- HP管理 ---------------*/
	// 最大HP
	static constexpr int32_t kMaxHp = 5;

	// 現在HP
	int32_t hp_ = kMaxHp;

	// 敵との接触ダメージ
	static constexpr int32_t kEnemyContactDamage = 1;

	// 死亡フラグ
	bool isDead_ = false;

	/*--------------- 溜め入力管理 ---------------*/
	// 現在の攻撃種類
	AttackType attackType_ = AttackType::kNormal;

	// ボタンを押している時間
	float chargeTimer_ = 0.0f;

	// 溜め攻撃になるまでの時間
	static constexpr float kChargeRequiredTime = 0.45f;

	// 最大溜め時間
	static constexpr float kChargeMaxTime = 1.0f;

	// 最大まで溜まったか
	bool isChargeReady_ = false;

	// 溜め攻撃時の奥行き角度
	// 90度を超えることで、バットの先端が斜め後ろまで移動する
	static constexpr float kChargeBatAngle = -145.0f;

	// 通常攻撃開始時の先端の上がり具合
	static constexpr float kNormalBatTipLift = 10.0f;

	// 最大溜め時の先端の上がり具合
	static constexpr float kChargeBatTipLift = 28.0f;

	// 振り抜いた後の先端の上がり具合
	static constexpr float kBatFollowThroughLift = 18.0f;

	/*--------------- 通常攻撃用 ---------------*/
	// 各フェーズのタイマー
	float attackTimer_ = 0.0f;

	// 振りかぶり時間
	static constexpr float kAttackStartupTime = 0.05f;

	// 攻撃判定が出る時間
	static constexpr float kAttackActiveTime = 0.09f;

	// 後隙
	static constexpr float kAttackRecoveryTime = 0.10f;

	// 踏み込み速度
	static constexpr float kAttackStepSpeed = 0.80f;

	// バットの振り始めと振り終わり
	static constexpr float kBatAngleStart = -100.0f;
	static constexpr float kBatAngleEnd = 100.0f;

	// 攻撃判定の大きさ
	static constexpr float kAttackWidth = 7.0f;
	static constexpr float kAttackHeight = 2.6f;

	// プレイヤー中心から攻撃判定までの距離
	static constexpr float kAttackOffsetX = 2.0f;

	// 1回の攻撃で同じ敵へ複数回当てないためのフラグ
	bool hasHitEnemy_ = false;

	// 現在の攻撃フェーズ
	AttackPhase attackPhase_ = AttackPhase::kStartup;

	// 空中で行える通常攻撃の最大回数
	static constexpr uint32_t kMaxAirAttackCount = 3;

	// 現在の空中通常攻撃回数
	uint32_t airAttackCount_ = 0;

	// 攻撃を開始するたびに増える識別番号
	uint32_t attackSerial_ = 0;

	// 現在のバットの奥行き角度
	float currentBatSwingAngleDegree_ = 0.0f;

	// バットが正面を横切ったフレームか
	bool isAttackImpactFrame_ = false;

	// ↓ 必要なくなった
	// バット用モデル
	KamataEngine::Model* modelBat_ = nullptr;
	KamataEngine::WorldTransform worldTransformBat_;
	bool isBatVisible_ = false;
	// 

	/*--------------- 空中攻撃制御 ---------------*/
	// 空中攻撃中に縦速度へ掛ける減衰率
	static constexpr float kAirAttackVerticalDamping = 0.72f;

	// 縦速度を完全停止させる基準
	static constexpr float kAirAttackStopSpeed = 0.02f;

	// 空中攻撃終了後に滞空する時間
	static constexpr float kAirAttackHangTime = 0.08f;

	// 現在の滞空時間
	float airAttackHangTimer_ = 0.0f;

	/*--------------- プレイヤー被ノックバック ---------------*/
	// ノックバック方向
	float knockBackDirection_ = 0.0f;

	// ノックバック開始時の上向き速度
	static constexpr float kKnockBackJumpSpeed = 0.28f;

	// ノックバック経過時間
	float knockBackTimer_ = 0.0f;

	// ノックバック時間
	static constexpr float kKnockBackTime = 0.20f;

	// 目標とするノックバック距離
	static constexpr float kKnockBackDistance = kWidth * 4.0f;

	// 目標距離から計算した1フレームの移動速度
	static constexpr float kKnockBackSpeed = kKnockBackDistance / (kKnockBackTime * 60.0f);

	/*--------------- 被ダメージ後の無敵 ---------------*/
	// 無敵中か
	bool isInvincible_ = false;

	// 無敵時間の残り
	float invincibleTimer_ = 0.0f;

	// ノックバック終了後の無敵時間
	static constexpr float kInvincibleTime = 3.0f;

	// 明滅が一周する時間
	static constexpr float kInvincibleBlinkCycle = 0.35f;

	// 明滅中の最低透明度
	static constexpr float kInvincibleMinAlpha = 0.55f;

	/*--------------- 死亡演出 ---------------*/
	// 死亡演出中か
	bool isDeathAnimationPlaying_ = false;

	// 死亡演出が終了したか
	bool isDeathAnimationFinished_ = false;

	// 死亡演出の経過時間
	float deathAnimationTimer_ = 0.0f;

	// 死亡演出開始時の大きさ
	KamataEngine::Vector3 deathAnimationStartScale_ = {2.0f, 2.0f, 2.0f};

	// 回転・縮小にかける時間
	static constexpr float kDeathAnimationTime = 0.9f;

	// 演出中に回転する回数
	static constexpr float kDeathRotationCount = 3.0f;

	// 死亡演出時にプレイヤーを持ち上げる距離
	static constexpr float kDeathPositionOffsetY = kHeight * 1.5f;

	/*--------------- 武器モデル ---------------*/
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