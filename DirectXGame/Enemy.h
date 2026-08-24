#pragma once
#include "KamataEngine.h"
#include "Transform.h"
#include "temporaryAABB.h"

// 前方宣言
class MapChipField;
class GameScene;
class Player;

// 左右の向き
// enum class LRDirection {
//	kRight,
//	kLeft,
//};

// 振る舞い
enum class BehaviorEnemy {
	kRoot,
	kStunned,
	kBlownAway,
	kDeath,
	kUnknown,
};

// マップとの当たり判定情報
struct EnemyMapCollisionInfo {
	bool hitLeft = false;
	bool hitRight = false;
	bool hitTop = false;
	bool hitBottom = false;

	KamataEngine::Vector3 movement = {};
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

	/// <summary>
	/// 敵の衝突判定処理
	/// </summary>
	/// <param name="player">自機の情報</param>
	bool OnCollisionPlayer(Player* player);

	// 当たり判定が無効化されているか
	bool IsCollisionDisEnabled() const;

	// プレイヤーに接触ダメージを与えられるか
	bool CanDamagePlayer() const;

	// 吹き飛び中で、他の敵へ攻撃できるか
	bool CanHitOtherEnemy() const;

	// 吹き飛び敵の攻撃を受けられるか
	bool CanReceiveBlownAwayHit() const;

	// 現在の飛行区間の攻撃権を消費
	void ConsumeBlownAwayHit();

	// 吹き飛んできた敵との衝突処理
	void OnCollisionBlownAwayEnemy(float attackDirection);

	// ゲッター


	bool GetIsDead() const { return isDead_; }

	int32_t GetBounceCount() const { return bounceCount_; }

	bool IsBlownAway() const { return behavior_ == BehaviorEnemy::kBlownAway; }

	float GetBlownAwayDirectionX() const;

	int32_t GetHp() const { return hp_; }
	int32_t GetStunValue() const { return stunHitCount_; }

	// セッター
	void SetGameScene(GameScene* gameScene);

	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

private:
	// HPダメージを受ける
	// 撃破された場合はtrue
	bool ApplyHpDamage(int32_t damage);

	// 通常状態での地形に沿った移動
	void UpdateRootMapMovement();

	// 横移動に地形判定を適用する
	// 壁に衝突した場合はtrue
	bool MoveHorizontalWithMap(float movementX);

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

	/*--------------- 通常状態の地形移動 ---------------*/
	// 接地しているか
	bool isOnGround_ = false;

	// 重力加速度
	static constexpr float kGravityAcceleration = 0.09f;

	// 最大落下速度
	static constexpr float kMaxFallSpeed = 0.75f;

	// 地形とのめり込み防止用余白
	static constexpr float kRootMapMargin = 0.05f;

	/*--------------- HP管理 ---------------*/
	// 最大HP
	static constexpr int32_t kMaxHp = 6;

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
	static constexpr float kStunnedTime = 3.0f;

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
	static constexpr float kHitKnockBackSpeed = 1.18f;

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

	//// 横方向の初速
	//static constexpr float kBlownAwaySpeedX = 0.75f;

	//// 上方向の初速
	//static constexpr float kBlownAwaySpeedY = 0.55f;

	//// 吹き飛び中の重力
	//static constexpr float kBlownAwayGravity = 0.04f;

	//// 最大落下速度
	//static constexpr float kBlownAwayMaxFallSpeed = 0.8f;

	//// 仮の吹き飛び継続時間
	//static constexpr float kBlownAwayTime = 1.2f;

	// 初速
	static constexpr float kBlownAwaySpeed = 0.75f;

	// この時間までは速度を維持する
	static constexpr float kBlownAwayFlyingTime = 2.0f;

	// 終了時の速度減衰率
	static constexpr float kBlownAwayStopAttenuation = 0.88f;

	// この速度を下回ったら停止
	static constexpr float kBlownAwayStopSpeed = 0.03f;

	// 反射時に加えるランダム角度
	static constexpr float kRandomBounceAngle = 40.0f;

	// 回転速度
	static constexpr float kBlownAwayRotateSpeed = 0.3f;


	/*--------------- 吹っ飛び攻撃 ---------------*/
	// 現在の飛行区間で敵へ命中できるか
	bool canHitEnemyInCurrentBounce_ = false;

	// 吹き飛び敵が与えるスタン値
	static constexpr int32_t kBlownAwayHitStunDamage = 1;

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
	//float deathTimer_ = 0.0f;
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