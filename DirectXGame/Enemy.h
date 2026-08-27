#pragma once
#include "KamataEngine.h"
#include "Transform.h"
#include "temporaryAABB.h"
#include <array>

// 前方宣言
class MapChipField;
class GameScene;
class Player;

// 左右の向き
enum class EnemyLRDirection {
	kRight,
	kLeft,
};

// 振る舞い
enum class BehaviorEnemy {
	kSpawn, // 出現演出
	kRoot,
	kStunned,
	kBlownAway,
	kDeath,
	kUnknown,
};

// 敵の用途
enum class EnemyPurpose {
	kNormal,           // 通常の敵
	kTutorialLauncher, // プレイヤーに吹き飛ばされる敵
	kTutorialTarget,   // 飛んできた敵を当てる標的
};

// 追跡ジャンプの状態
enum class ChaseJumpState {
	kDirectChase,    // 通常追跡
	kMoveToTakeoff,  // 上方向：踏切位置へ移動
	kTakeoffPause,   // 上方向：踏切位置で停止
	kJumping,        // 上方向：ジャンプ中
	kMoveToDropEdge, // 下方向：足場の端へ移動
	kDropping,       // 下方向：落下中
	kLandingWait,    // 着地後の待機
};

// マップとの当たり判定情報
struct EnemyMapCollisionInfo {
	bool hitLeft = false;
	bool hitRight = false;
	bool hitTop = false;
	bool hitBottom = false;

	KamataEngine::Vector3 movement = {};
};

// 吹き飛び中の簡易残像
struct EnemyTrailPoint {
	KamataEngine::WorldTransform worldTransform;
	float lifeTimer = 0.0f;
	bool isActive = false;
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
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3 pos, bool useSpawnAnimation = false);

	/// <summary>
	/// 敵の更新
	/// </summary>
	void Update();

	// ルートビヘイビア用更新
	// 出現演出
	void BehaviorSpawnInitialize();
	void BehaviorSpawnUpdate();

	// 通常行動
	void BehaviorRootInitialize();
	void BehaviorRootUpdate();

	// スタン
	void BehaviorStunnedInitialize();
	void BehaviorStunnedUpdate();

	// 吹き飛び
	void BehaviorBlownAwayInitialize();
	void BehaviorBlownAwayUpdate();

	// 死亡アクション
	void BehaviorDeathInitialize();
	void BehaviorDeathUpdate();

	// 通常攻撃を受けた際の小ノックバック更新
	void UpdateHitKnockBack();

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

	// 吹き飛び中に他の敵へ攻撃するためのAABB
	AABB GetBlownAwayAttackAABB();

	/// <summary>
	/// 敵の衝突判定処理
	/// </summary>
	/// <param name="player">自機の情報</param>
	bool OnCollisionPlayer(Player* player);

	// 当たり判定が無効化されているか
	bool IsCollisionDisEnabled() const;

	// プレイヤーに接触ダメージを与えられるか
	bool CanDamagePlayer() const;

	// スタン衝撃による弱いノックバックを受けられるか
	bool CanReceiveStunShockwave() const { return behavior_ == BehaviorEnemy::kRoot && !isHitKnockBack_ && !isHitRecovery_ && !isDead_; }

	// スタン衝撃による弱いノックバック開始
	void StartStunShockwaveKnockBack(float direction);

	// ノックバック後の硬直中か
	bool IsHitRecovery() const { return isHitRecovery_; }

	// プレイヤーの溜め攻撃による吹き飛びを開始
	void StartBlownAway(float direction);

	// 吹き飛び中で、他の敵へ攻撃できるか
	bool CanHitOtherEnemy() const;

	// 吹き飛び敵の攻撃を受けられるか
	bool CanReceiveBlownAwayHit() const;

	// 現在の飛行区間の攻撃権を消費
	void ConsumeBlownAwayHit();

	// 吹き飛んできた敵との衝突処理
	void OnCollisionBlownAwayEnemy(float attackDirection);

	// 敵同士の重なり補正対象か
	bool CanResolveEnemyOverlap() const {
		/*
		 * ノックバック後の硬直中は押さない。
		 * ノックバック中は後続の敵を押すため、
		 * 補正対象に含める。
		 */
		if (isHitRecovery_) {
			return false;
		}

		return behavior_ == BehaviorEnemy::kRoot || behavior_ == BehaviorEnemy::kStunned;
	}

	// スタン中か
	bool IsStunned() const { return behavior_ == BehaviorEnemy::kStunned; }

	// 敵同士の重なりを解消するために横移動する
	// 実際に移動できた量を返す
	float MoveForEnemySeparation(float movementX);

	// 足場間を移動するための特殊経路中か
	bool IsUsingPlatformRoute() const {
		return chaseJumpState_ == ChaseJumpState::kMoveToTakeoff || chaseJumpState_ == ChaseJumpState::kTakeoffPause || chaseJumpState_ == ChaseJumpState::kJumping ||
		       chaseJumpState_ == ChaseJumpState::kMoveToDropEdge || chaseJumpState_ == ChaseJumpState::kDropping;
	}

	bool IsTutorialEnemy() const { return purpose_ != EnemyPurpose::kNormal; }

	// 通常攻撃などによる小ノックバック中か
	bool IsHitKnockBack() const { return isHitKnockBack_; }

	// ゲッター
	bool GetIsDead() const { return isDead_; }

	int32_t GetBounceCount() const { return bounceCount_; }

	bool IsBlownAway() const { return behavior_ == BehaviorEnemy::kBlownAway; }

	float GetBlownAwayDirectionX() const;

	int32_t GetHp() const { return hp_; }
	int32_t GetStunValue() const { return stunHitCount_; }

	EnemyPurpose GetPurpose() const { return purpose_; }

	// 敵の高さを取得
	static float GetHeight() { return kHeight; }

	// 小ノックバックの方向
	float GetHitKnockBackDirection() const { return hitKnockBackDirection_; }

	// セッター
	void SetGameScene(GameScene* gameScene);

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	// 追跡対象を設定
	void SetTarget(Player* target) { target_ = target; }

	void SetPurpose(EnemyPurpose purpose) { purpose_ = purpose; }

	/*--------------------  残像モデル  --------------------*/
	// circle.pngを使用する残像モデルを設定
	static void SetTrailModel(KamataEngine::Model* model) { trailModel_ = model; }

private:
	// HPダメージを受ける
	// 撃破された場合はtrue
	bool ApplyHpDamage(int32_t damage);

	// 横移動方向に応じた向きの更新
	void UpdateFacingDirection();

	// 通常状態での地形に沿った移動
	void UpdateRootMapMovement();

	// ノックバック終了後の硬直更新
	void UpdateHitRecovery();

	// スタン中の重力・床・天井判定
	void UpdateStunnedMapMovement();

	// プレイヤーの位置から移動方向を決める
	void UpdateChaseDirection();

	// 追跡中のジャンプ判断
	void TryChaseJump();

	// 横移動に地形判定を適用する
	// 壁に衝突した場合はtrue
	bool MoveHorizontalWithMap(float movementX);

	// 上方の足場から踏切位置を取得
	// 見つかった場合はtrue
	bool FindOverheadPlatformTakeoff(float& takeoffX, float& jumpDirection, float& platformTopY) const;

	// 現在いる足場から降りる位置を取得
	bool FindCurrentPlatformDropEdge(float& dropTargetX, float& dropDirection) const;

	// 指定したX位置から落下した場合の着地面を探す
	bool FindLandingPlatformBelow(float dropX, float& landingTopY) const;

	// 指定方向の少し先に床があるか
	bool HasFloorAhead(float direction) const;

	// 指定したX座標まで現在の足場上を歩いて到達できるか
	bool IsTakeoffReachableOnCurrentPlatform(float takeoffX) const;

	// 吹き飛び残像の更新
	void UpdateBlownAwayTrail();

	// 現在位置へ残像を1個追加
	void AddBlownAwayTrail();

	// 吹き飛び残像の描画
	void DrawBlownAwayTrail();

	// Translateクラス内の関数を使える様にする
	Transform transform_;
	GameScene* gameScene_ = nullptr;

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
	static inline const float kMoveSpeed = 0.035f;

	// 敵の当たり判定サイズ
	static constexpr float kWidth = 1.6f;
	static constexpr float kHeight = 1.6f;

	// アニメーション用変数
	// 最初の角度(度)
	static inline const float kWalkMotionAngleStart = -30.f;

	// 最後の角度(度)
	static inline const float kWalkMotionAngleEnd = 30.f;

	// アニメーション周期となる時間（秒）
	static inline const float kWalkMotionTime = 0.5f;

	// 経過時間
	float walkTimer_ = 0.0f;

	// この敵の用途
	EnemyPurpose purpose_ = EnemyPurpose::kNormal;

	/*--------------- 出現演出 ---------------*/
	// 出現演出の経過時間
	float spawnTimer_ = 0.0f;

	// 出現時の基準位置
	KamataEngine::Vector3 spawnBasePosition_{};

	// 最初の大きさ
	static constexpr float kSpawnStartScale = 0.05f;

	// 膨らみ切った瞬間の大きさ
	static constexpr float kSpawnExpandScale = 2.25f;

	// 通常時の大きさ
	static constexpr float kNormalScale = 2.0f;

	// 膨らむ時間
	static constexpr float kSpawnExpandTime = 0.45f;

	// 膨らんだ後に滞空する時間
	static constexpr float kSpawnHoverTime = 0.25f;

	// 足場より上に出現する高さ
	static constexpr float kSpawnHeight = 2.5f;

	/*--------------- 通常状態の地形移動 ---------------*/
	// 接地しているか
	bool isOnGround_ = false;

	// 重力加速度
	static constexpr float kGravityAcceleration = 0.07f;

	// 最大落下速度
	static constexpr float kMaxFallSpeed = 0.75f;

	// 地形とのめり込み防止用余白
	static constexpr float kRootMapMargin = 0.05f;

	/*--------------- 左右方向・旋回管理 ---------------*/
	// 現在向いている方向
	EnemyLRDirection lrDirection_ = EnemyLRDirection::kLeft;

	// 旋回開始時のY回転角
	float turnStartRotationY_ = 0.0f;

	// 旋回の残り時間
	float turnTimer_ = 0.0f;

	// 敵の旋回時間
	static constexpr float kTurnTime = 0.10f;

	/*--------------- プレイヤー追跡 ---------------*/
	// 追跡対象
	Player* target_ = nullptr;

	// プレイヤーとのX座標差がこの値以下なら停止
	static constexpr float kChaseStopDistance = 0.15f;

	// 空中でプレイヤーを追う横移動速度
	static constexpr float kAirChaseMoveSpeed = 0.08f;

	// 現在の追跡ジャンプ状態
	ChaseJumpState chaseJumpState_ = ChaseJumpState::kDirectChase;

	// ジャンプ中に固定する横方向
	float chaseJumpDirection_ = 0.0f;

	// 着地後の待機時間
	float chaseLandingWaitTimer_ = 0.0f;

	// 着地後に再判断するまでの時間
	static constexpr float kChaseLandingWaitTime = 0.35f;

	// 追跡経路を決めた時点のプレイヤーのY座標
	float plannedTargetY_ = 0.0f;

	// 1マスが2.0なので、その半分
	static constexpr float kChaseReplanHeightThreshold = 1.0f;

	// 乗り移る足場上の着地目標X座標
	float targetPlatformLandingX_ = 0.0f;

	// 着地目標Xへ到着したとみなす距離
	static constexpr float kPlatformLandingArrivalDistance = 0.05f;

	// 目的足場へ着地できたとみなす高さの誤差
	static constexpr float kPlatformLandingHeightTolerance = 0.25f;

	/*--------------- 追跡ジャンプ ---------------*/
	// ジャンプ初速
	static constexpr float kChaseJumpSpeed = 1.25f;

	// プレイヤーがこの高さ以上にいる場合にジャンプを検討
	static constexpr float kJumpHeightThreshold = 1.0f;

	// 足場へ乗り移る際に予定しているジャンプ方向
	float plannedJumpDirection_ = 0.0f;

	// 次のジャンプまでの待ち時間
	float chaseJumpCooldownTimer_ = 0.0f;

	// ジャンプ後の待ち時間
	static constexpr float kChaseJumpCooldownTime = 0.75f;

	// 上方の足場を調べるマップチップ数
	static constexpr uint32_t kOverheadSearchRows = 5;

	// 固定した踏切位置
	float takeoffTargetX_ = 0.0f;

	// 踏切位置で停止する時間
	float takeoffPauseTimer_ = 0.0f;

	// ジャンプ前の停止時間
	static constexpr float kTakeoffPauseTime = 0.20f;

	// 踏切位置へ到着したとみなす距離
	static constexpr float kTakeoffArrivalDistance = 0.06f;

	// 乗り移る足場の上面Y座標
	float targetPlatformTopY_ = 0.0f;

	// 敵の下端が足場上面を越えたか
	bool hasClearedTargetPlatformTop_ = false;

	// 足場へ乗り移る際の横速度
	static constexpr float kPlatformTransferSpeed = 0.20f;

	// 足場上面を越えたと判断する余白
	static constexpr float kPlatformTopClearance = 0.08f;

	/*--------------- 追跡落下 ---------------*/
	// プレイヤーがこれ以上下にいる場合、足場から降りる
	static constexpr float kDropHeightThreshold = 1.0f;

	// 足場から確実に外れるための余白
	static constexpr float kDropEdgeExtraMargin = 0.15f;

	// 足場から降りる目標X座標
	float dropTargetX_ = 0.0f;

	// 足場から降りる方向
	float dropDirection_ = 0.0f;

	/*--------------- HP管理 ---------------*/
	// 最大HP
	static constexpr int32_t kMaxHp = 5;

	// 現在のHP
	int32_t hp_ = kMaxHp;

	// プレイヤーの通常攻撃によるHPダメージ
	static constexpr int32_t kNormalAttackHpDamage = 1;

	// プレイヤーの溜め攻撃によるHPダメージ
	static constexpr int32_t kChargedAttackHpDamage = 2;

	// 吹き飛んだ敵が与えるHPダメージ
	static constexpr int32_t kBlownAwayHitHpDamage = 4;

	/*--------------- スタン管理 ---------------*/
	// 通常攻撃を受けた回数
	int32_t stunHitCount_ = 0;

	// 行動不能になるまでの攻撃回数
	static constexpr int32_t kStunHitCount = 3;

	// 行動不能時間
	float stunnedTimer_ = 0.0f;
	static constexpr float kStunnedTime = 5.0f;

	// 最後に受けた攻撃の識別番号
	uint32_t lastReceivedAttackSerial_ = 0;

	// スタン演出用
	float stunnedMotionTimer_ = 0.0f;

	// スタン中に上を向く角度
	static constexpr float kStunnedLookUpAngle = -70.0f;

	// スタン中の震える角度
	static constexpr float kStunnedShakeAngle = 7.0f;

	// 震える速さ
	static constexpr float kStunnedShakeSpeed = 35.0f;

	/*--------------- 通常攻撃ノックバック ---------------*/
	// ノックバック中か
	bool isHitKnockBack_ = false;

	// ノックバック方向
	float hitKnockBackDirection_ = 0.0f;

	// 経過時間
	float hitKnockBackTimer_ = 0.0f;

	// ノックバック時間
	static constexpr float kHitKnockBackTime = 0.12f;

	// ノックバック速度
	static constexpr float kHitKnockBackSpeed = 1.05f;

	// 今回のノックバックに使用する速度
	float currentHitKnockBackSpeed_ = kHitKnockBackSpeed;

	// ノックバック終了後の硬直中か
	bool isHitRecovery_ = false;

	// 硬直時間
	float hitRecoveryTimer_ = 0.0f;
	static constexpr float kHitRecoveryTime = 0.18f;

	// スタン発生時に周囲へ与える弱いノックバック速度
	static constexpr float kStunShockwaveKnockBackSpeed = 4.20f;

	/*--------------- 直線吹き飛び ---------------*/
	// 最初の吹っ飛び角度の最大最小
	static constexpr float kInitialBlownAwayAngleMin = 5.0f;
	static constexpr float kInitialBlownAwayAngleMax = 45.0f;

	// 吹き飛ばす方向
	float blownAwayDirection_ = 0.0f;

	// 吹き飛び中の速度
	KamataEngine::Vector3 blownAwayVelocity_ = {};

	// 吹き飛び経過時間
	float blownAwayTimer_ = 0.0f;

	// 初速
	static constexpr float kBlownAwaySpeed = 1.75f;

	// この時間までは速度を維持する
	static constexpr float kBlownAwayFlyingTime = 2.5f;

	// 終了時の速度減衰率
	static constexpr float kBlownAwayStopAttenuation = 0.88f;

	// この速度を下回ったら停止
	static constexpr float kBlownAwayStopSpeed = 0.03f;

	// 反射時に加えるランダム角度
	static constexpr float kRandomBounceAngle = 40.0f;

	// 回転速度
	static constexpr float kBlownAwayRotateSpeed = 0.3f;

	// カメラ中心から画面左右端までの距離
	static constexpr float kScreenHalfWidth = 21.0f;

	// 16:9の画面なので、横幅から縦幅を計算
	static constexpr float kScreenHalfHeight = kScreenHalfWidth * 9.0f / 16.0f;

	// 画面端との間に取る余白
	static constexpr float kScreenBounceMargin = 0.05f;

	// 吹き飛び・死亡演出を地形より手前に表示する
	// 敵半分～1体分の中間
	static constexpr float kBlownAwayFrontOffset = kWidth * 0.75f;

	/*--------------- 吹き飛び残像 ---------------*/
	// circle.pngを使用しているモデル
	static KamataEngine::Model* trailModel_;

	// 残像の最大数
	static constexpr size_t kTrailPointCount = 5;

	// 残像データ
	std::array<EnemyTrailPoint, kTrailPointCount> trailPoints_{};

	// 次に上書きする場所
	size_t trailWriteIndex_ = 0;

	// 残像生成間隔のタイマー
	float trailSpawnTimer_ = 0.0f;

	// 残像を配置する間隔
	static constexpr float kTrailSpawnInterval = 0.05f;

	// 残像1個が消えるまでの時間
	static constexpr float kTrailLifeTime = 0.20f;

	// 最初の透明度
	static constexpr float kTrailStartAlpha = 0.16f;

	// 最初の大きさ
	static constexpr float kTrailStartScale = 0.65f;

	// 消える直前の大きさ
	static constexpr float kTrailEndScale = 0.20f;

	// 敵本体より少し奥へ配置
	static constexpr float kTrailBackOffsetZ = 0.08f;

	/*--------------- 吹っ飛び攻撃 ---------------*/
	// 現在の飛行区間で敵へ命中できるか
	bool canHitEnemyInCurrentBounce_ = false;

	// 吹き飛び敵が与えるスタン値
	static constexpr int32_t kBlownAwayHitStunDamage = 1;

	// 吹き飛び攻撃判定の拡大量
	static constexpr float kBlownAwayAttackMarginX = 0.8f;
	static constexpr float kBlownAwayAttackMarginY = 0.6f;

	/*--------------- 地形反射 ---------------*/
	// 反射回数
	int32_t bounceCount_ = 0;

	// ブロックとの余白
	static constexpr float kBlownAwayMargin = 0.05f;

	// 反射が弱くなって停止したか
	bool isBlownAwayStopped_ = false;

	/*--------------- 死亡演出管理 ---------------*/
	// 死んだか
	bool isDead_ = false;

	// 死亡アクション時間管理
	// float deathTimer_ = 0.0f;
	static constexpr float kDeathTime = 1.0f;

	bool isCollisionDisenabled_ = false;

	/*--------------- 破裂死亡演出 ---------------*/
	// 死亡演出の経過時間
	float deathTimer_ = 0.0f;

	// 最初に膨らむ時間
	static constexpr float kDeathExpandTime = 0.10f;

	// 膨らんだ後に消える時間
	static constexpr float kDeathShrinkTime = 0.12f;

	// 膨張倍率
	static constexpr float kDeathExpandScaleRate = 1.4f;

	// 死亡開始時の大きさ
	KamataEngine::Vector3 deathStartScale_ = {};

	// ヒットエフェクトを生成済みか
	bool hasCreatedDeathBurstEffect_ = false;

	/*--------------- ビヘイビア管理用 ---------------*/
	// 現在の振る舞い
	BehaviorEnemy behavior_ = BehaviorEnemy::kRoot;

	// リクエスト
	BehaviorEnemy behaviorRequest_ = BehaviorEnemy::kUnknown;
};