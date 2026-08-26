#define NOMINMAX
#include "Enemy.h"
#include "GameScene.h"
#include "MapChipField.h"
#include "Player.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>
#include <random>

// コンストラクタ&デストラクタ
Enemy::Enemy() {}
Enemy::~Enemy() {}

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

namespace {

float RandomFloat(float minValue, float maxValue) {
	static std::random_device seedGenerator;
	static std::mt19937 randomEngine(seedGenerator());

	std::uniform_real_distribution<float> distribution(minValue, maxValue);

	return distribution(randomEngine);
}

} // namespace

/// <summary>
/// 敵の初期化
/// </summary>
/// <param name="model"></param>
/// <param name="pos"></param>
void Enemy::Initialize(Model* model, Camera* camera, const Vector3 pos, bool useSpawnAnimation) {
	// ぬるぽチェック
	assert(model);

	// 引き数の内容をメンバ変数に記録
	camera_ = camera;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// メンバ変数への代入処理
	// 敵の拡縮,回転,平行移動情報
	worldTransform_.translation_ = pos;

	if (useSpawnAnimation) {
		// 着地予定位置より上に出現
		worldTransform_.translation_.y += kSpawnHeight;

		worldTransform_.scale_ = {
		    kSpawnStartScale,
		    kSpawnStartScale,
		    kSpawnStartScale,
		};

		behavior_ = BehaviorEnemy::kSpawn;
		isCollisionDisenabled_ = true;

	} else {
		worldTransform_.scale_ = {
		    kNormalScale,
		    kNormalScale,
		    kNormalScale,
		};

		behavior_ = BehaviorEnemy::kRoot;
		isCollisionDisenabled_ = false;
	}
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;

	// 旋回制御用の変数初期化
	lrDirection_ = EnemyLRDirection::kLeft;
	turnStartRotationY_ = worldTransform_.rotation_.y;
	turnTimer_ = 0.0f;

	// 3Dモデルの生成
	modelEnemy_ = model;

	// 移動速度の初期化
	velocity_ = {-kMoveSpeed, 0.0f, 0.0f};

	// アニメーションタイマーの初期化
	walkTimer_ = 0.0f;

	// HPとスタン値の初期化
	hp_ = kMaxHp;
	stunHitCount_ = 0;

	isDead_ = false;
	behaviorRequest_ = BehaviorEnemy::kUnknown;

	spawnTimer_ = 0.0f;
	spawnBasePosition_ = worldTransform_.translation_;

	isOnGround_ = false;

	// 追跡対象の位置の初期化
	plannedTargetY_ = worldTransform_.translation_.y;

	// 追跡ジャンプ
	chaseJumpCooldownTimer_ = 0.0f;

	chaseJumpState_ = ChaseJumpState::kDirectChase;

	chaseJumpDirection_ = 0.0f;
	chaseLandingWaitTimer_ = 0.0f;
	chaseJumpCooldownTimer_ = 0.0f;

	// 追跡ジャンプの踏切位置の初期化
	takeoffTargetX_ = worldTransform_.translation_.x;

	// 追跡ジャンプの踏切位置での停止時間の初期化
	takeoffPauseTimer_ = 0.0f;
	chaseJumpDirection_ = 0.0f;
}

/// <summary>
/// 敵の更新
/// </summary>
void Enemy::Update() {
	// Behavior変更
	if (behaviorRequest_ != BehaviorEnemy::kUnknown) {

		behavior_ = behaviorRequest_;

		switch (behavior_) {
			// 出現演出初期化
		case BehaviorEnemy::kSpawn:
			BehaviorSpawnInitialize();
			break;
			// 通常行動初期化
		case BehaviorEnemy::kRoot:
			BehaviorRootInitialize();
			break;
			// スタン初期化
		case BehaviorEnemy::kStunned:
			BehaviorStunnedInitialize();
			break;
			// 吹っ飛び初期化
		case BehaviorEnemy::kBlownAway:
			BehaviorBlownAwayInitialize();
			break;
			// 死亡アクション初期化
		case BehaviorEnemy::kDeath:
			BehaviorDeathInitialize();
			break;

		default:
			break;
		}
		// 振る舞いリクエストをリセット
		behaviorRequest_ = BehaviorEnemy::kUnknown;
	}

	// Behaviorの実行
	switch (behavior_) {
		// 出現演出更新
	case BehaviorEnemy::kSpawn:
		BehaviorSpawnUpdate();
		break;
		// 通常行動更新
	case BehaviorEnemy::kRoot:
		BehaviorRootUpdate();
		break;
		// スタン更新
	case BehaviorEnemy::kStunned:
		BehaviorStunnedUpdate();
		break;
	// 吹っ飛び更新
	case BehaviorEnemy::kBlownAway:
		BehaviorBlownAwayUpdate();
		break;
		// 死亡アクション更新
	case BehaviorEnemy::kDeath:
		BehaviorDeathUpdate();
		break;

	default:
		break;
	}

	// 通常攻撃による小ノックバック
	UpdateHitKnockBack();

	// ノックバック後の硬直
	UpdateHitRecovery();

	// 行列を定数バッファに転送
	transform_.worldMatrixUpdate(worldTransform_);
}

// 横移動方向に応じた向きの更新
void Enemy::UpdateFacingDirection() {
	/*========== 移動方向の変化を確認 ==========*/
	EnemyLRDirection newDirection = lrDirection_;

	// ジャンプ前の停止中とジャンプ中は、
	// 足場へ乗り移る方向を向く
	if (chaseJumpState_ == ChaseJumpState::kTakeoffPause || chaseJumpState_ == ChaseJumpState::kJumping) {

		if (chaseJumpDirection_ > 0.0f || plannedJumpDirection_ > 0.0f) {
			newDirection = EnemyLRDirection::kRight;

		} else if (chaseJumpDirection_ < 0.0f || plannedJumpDirection_ < 0.0f) {
			newDirection = EnemyLRDirection::kLeft;
		}

	} else {
		// 通常時は移動方向を向く
		if (velocity_.x > 0.0f) {
			newDirection = EnemyLRDirection::kRight;

		} else if (velocity_.x < 0.0f) {
			newDirection = EnemyLRDirection::kLeft;
		}
	}

	// 向きが変化した瞬間に旋回開始
	if (newDirection != lrDirection_) {
		lrDirection_ = newDirection;

		turnStartRotationY_ = worldTransform_.rotation_.y;

		turnTimer_ = kTurnTime;
	}

	/*========== 旋回補間 ==========*/
	if (turnTimer_ <= 0.0f) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f;

	turnTimer_ -= deltaTime;
	turnTimer_ = std::max(turnTimer_, 0.0f);

	// 線形補間
	float t = 1.0f - turnTimer_ / kTurnTime;
	t = std::clamp(t, 0.0f, 1.0f);

	// 滑らかに開始・終了する補間
	float easeT = t * t * (3.0f - 2.0f * t);

	float destinationRotationY = 0.0f;

	switch (lrDirection_) {
	case EnemyLRDirection::kRight:
		destinationRotationY = std::numbers::pi_v<float> / 2.0f;
		break;

	case EnemyLRDirection::kLeft:
		destinationRotationY = -std::numbers::pi_v<float> / 2.0f;
		break;
	}

	worldTransform_.rotation_.y = turnStartRotationY_ + (destinationRotationY - turnStartRotationY_) * easeT;
}

// 通常状態での地形に沿った移動
void Enemy::UpdateRootMapMovement() {
	// マップが設定されていない場合
	if (!mapChipField_) {
		if (!isHitKnockBack_) {
			worldTransform_.translation_.x += velocity_.x;
		}

		velocity_.y -= kGravityAcceleration;
		velocity_.y = std::max(velocity_.y, -kMaxFallSpeed);

		worldTransform_.translation_.y += velocity_.y;
		return;
	}

	const float halfWidth = kWidth / 2.0f;
	const float halfHeight = kHeight / 2.0f;

	/*========== 重力 ==========*/
	velocity_.y -= kGravityAcceleration;
	velocity_.y = std::max(velocity_.y, -kMaxFallSpeed);

	/*========== 追跡ジャンプ ==========*/
	// 通常敵だけ追跡ジャンプを行う
	if (purpose_ == EnemyPurpose::kNormal) {
		TryChaseJump();
	}

	/*========== 横方向の移動と壁判定 ==========*/
	float movementX = 0.0f;

	if (!isHitKnockBack_ && !isHitRecovery_) {

		movementX = velocity_.x;
	}

	bool hitWall = MoveHorizontalWithMap(movementX);

	if (hitWall) {
		velocity_.x = 0.0f;
	}

	/*========== 縦方向の移動と床・天井判定 ==========*/
	float nextY = worldTransform_.translation_.y + velocity_.y;

	isOnGround_ = false;

	if (velocity_.y > 0.0f) {
		/*========== 天井判定 ==========*/

		Vector3 checkPositions[] = {
		    {
             worldTransform_.translation_.x - halfWidth + kRootMapMargin,
             nextY + halfHeight,
             worldTransform_.translation_.z,
		     },
		    {
             worldTransform_.translation_.x + halfWidth - kRootMapMargin,
             nextY + halfHeight,
             worldTransform_.translation_.z,
		     },
		};

		bool hitCeiling = false;
		float resolvedY = nextY;

		for (const Vector3& position : checkPositions) {

			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

			// 敵の上端を天井の下面へ合わせる
			float candidateY = rect.bottom - halfHeight - kRootMapMargin;

			resolvedY = std::min(resolvedY, candidateY);

			hitCeiling = true;
		}

		if (hitCeiling) {
			nextY = resolvedY;
			velocity_.y = 0.0f;
		}

	} else {
		/*========== 床判定 ==========*/

		Vector3 checkPositions[] = {
		    {
             worldTransform_.translation_.x - halfWidth + kRootMapMargin,
             nextY - halfHeight,
             worldTransform_.translation_.z,
		     },
		    {
             worldTransform_.translation_.x + halfWidth - kRootMapMargin,
             nextY - halfHeight,
             worldTransform_.translation_.z,
		     },
		};

		bool hitFloor = false;
		float resolvedY = nextY;

		for (const Vector3& position : checkPositions) {

			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

			// 敵の下端を床の上面へ合わせる
			float candidateY = rect.top + halfHeight + kRootMapMargin;

			resolvedY = std::max(resolvedY, candidateY);

			hitFloor = true;
		}

		if (hitFloor) {
			nextY = resolvedY;
			velocity_.y = 0.0f;
			isOnGround_ = true;
		}
	}

	worldTransform_.translation_.y = nextY;
}

// ノックバック終了後の硬直更新
void Enemy::UpdateHitRecovery() {
	if (!isHitRecovery_) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f;

	hitRecoveryTimer_ -= deltaTime;

	// 硬直中は横移動を停止
	velocity_.x = 0.0f;

	if (hitRecoveryTimer_ <= 0.0f) {
		hitRecoveryTimer_ = 0.0f;
		isHitRecovery_ = false;
	}
}

// スタン中の重力・床・天井判定
void Enemy::UpdateStunnedMapMovement() {
	/*========== マップがない場合 ==========*/

	if (!mapChipField_) {
		velocity_.y -= kGravityAcceleration;

		velocity_.y = std::max(velocity_.y, -kMaxFallSpeed);

		worldTransform_.translation_.y += velocity_.y;

		isOnGround_ = false;
		return;
	}

	const float halfWidth = kWidth / 2.0f;

	const float halfHeight = kHeight / 2.0f;

	/*========== 重力 ==========*/

	velocity_.y -= kGravityAcceleration;

	velocity_.y = std::max(velocity_.y, -kMaxFallSpeed);

	float nextY = worldTransform_.translation_.y + velocity_.y;

	isOnGround_ = false;

	/*========== 上昇中の天井判定 ==========*/

	if (velocity_.y > 0.0f) {
		Vector3 checkPositions[] = {
		    {
             worldTransform_.translation_.x - halfWidth + kRootMapMargin,

             nextY + halfHeight,

             worldTransform_.translation_.z,
		     },
		    {
             worldTransform_.translation_.x + halfWidth - kRootMapMargin,

             nextY + halfHeight,

             worldTransform_.translation_.z,
		     },
		};

		bool hitCeiling = false;
		float resolvedY = nextY;

		for (const Vector3& position : checkPositions) {

			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

			float candidateY = rect.bottom - halfHeight - kRootMapMargin;

			resolvedY = std::min(resolvedY, candidateY);

			hitCeiling = true;
		}

		if (hitCeiling) {
			nextY = resolvedY;
			velocity_.y = 0.0f;
		}
	}

	/*========== 落下中の床判定 ==========*/

	else {
		Vector3 checkPositions[] = {
		    {
             worldTransform_.translation_.x - halfWidth + kRootMapMargin,

             nextY - halfHeight,

             worldTransform_.translation_.z,
		     },
		    {
             worldTransform_.translation_.x + halfWidth - kRootMapMargin,

             nextY - halfHeight,

             worldTransform_.translation_.z,
		     },
		};

		bool hitFloor = false;
		float resolvedY = nextY;

		for (const Vector3& position : checkPositions) {

			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

			// 敵の下端を床上面へ合わせる
			float candidateY = rect.top + halfHeight + kRootMapMargin;

			resolvedY = std::max(resolvedY, candidateY);

			hitFloor = true;
		}

		if (hitFloor) {
			nextY = resolvedY;
			velocity_.y = 0.0f;
			isOnGround_ = true;
		}
	}

	worldTransform_.translation_.y = nextY;
}

// プレイヤーの位置から移動方向を決める
void Enemy::UpdateChaseDirection() {
	if (!target_) {
		velocity_.x = 0.0f;
		return;
	}

	Vector3 playerPos = target_->GetWorldPos();

	float differenceX = playerPos.x - worldTransform_.translation_.x;
	float differenceY = playerPos.y - worldTransform_.translation_.y;

	/*========== プレイヤーの移動に応じた経路再計算 ==========*/
	bool isMovingToPlannedPosition = chaseJumpState_ == ChaseJumpState::kMoveToTakeoff || chaseJumpState_ == ChaseJumpState::kTakeoffPause || chaseJumpState_ == ChaseJumpState::kMoveToDropEdge ||
	                                 chaseJumpState_ == ChaseJumpState::kLandingWait;

	/*
	 * プレイヤーが接地した状態で別の高さへ移動した場合、
	 * 古い踏み切り位置・降り口を破棄する。
	 *
	 * kJumpingとkDroppingは空中なので、
	 * 着地するまで現在の動きを継続する。
	 */
	if (isMovingToPlannedPosition && target_->IsOnGround() && std::abs(playerPos.y - plannedTargetY_) > kChaseReplanHeightThreshold) {

		chaseJumpState_ = ChaseJumpState::kDirectChase;

		takeoffPauseTimer_ = 0.0f;
		chaseLandingWaitTimer_ = 0.0f;

		plannedJumpDirection_ = 0.0f;
		chaseJumpDirection_ = 0.0f;
		dropDirection_ = 0.0f;
	}

	/*========== 下方向：足場の端へ移動 ==========*/
	if (chaseJumpState_ == ChaseJumpState::kMoveToDropEdge) {

		// すでに足場から離れた場合
		if (!isOnGround_) {
			chaseJumpState_ = ChaseJumpState::kDropping;

			velocity_.x = dropDirection_ * kAirChaseMoveSpeed;

			return;
		}

		float differenceToDrop = dropTargetX_ - worldTransform_.translation_.x;

		if (differenceToDrop > 0.0f) {
			velocity_.x = std::min(kMoveSpeed, differenceToDrop);
		} else {
			velocity_.x = std::max(-kMoveSpeed, differenceToDrop);
		}

		/*
		 * 目標位置へ着いても、接地が切れるまでは
		 * 外側へ少し進み続ける。
		 */
		if (std::abs(differenceToDrop) <= kTakeoffArrivalDistance) {

			velocity_.x = dropDirection_ * kMoveSpeed;
		}

		return;
	}

	/*========== 下方向：落下中 ==========*/

	if (chaseJumpState_ == ChaseJumpState::kDropping) {

		// 落下中はプレイヤーのX方向へ寄せる
		if (differenceX > kChaseStopDistance) {
			velocity_.x = kAirChaseMoveSpeed;

		} else if (differenceX < -kChaseStopDistance) {
			velocity_.x = -kAirChaseMoveSpeed;

		} else {
			velocity_.x = 0.0f;
		}

		return;
	}

	/*========== 固定した踏切位置へ移動 ==========*/
	if (chaseJumpState_ == ChaseJumpState::kMoveToTakeoff) {

		float differenceToTakeoff = takeoffTargetX_ - worldTransform_.translation_.x;

		/*========== 踏切位置へ到着 ==========*/
		if (std::abs(differenceToTakeoff) <= kTakeoffArrivalDistance) {
			velocity_.x = 0.0f;
			chaseJumpState_ = ChaseJumpState::kTakeoffPause;
			takeoffPauseTimer_ = kTakeoffPauseTime;

			return;
		}

		/*
		 * プレイヤーの移動や着地位置の変化によって、
		 * 保存した踏み切り位置へ現在の足場から
		 * 行けなくなっていた場合だけ中止する。
		 */
		if (isOnGround_ && !IsTakeoffReachableOnCurrentPlatform(takeoffTargetX_)) {

			chaseJumpState_ = ChaseJumpState::kDirectChase;

			velocity_.x = 0.0f;

			plannedJumpDirection_ = 0.0f;
			chaseJumpDirection_ = 0.0f;

			return;
		}

		/*========== 踏切位置へ移動 ==========*/

		if (differenceToTakeoff > 0.0f) {

			velocity_.x = std::min(kMoveSpeed, differenceToTakeoff);

		} else {

			velocity_.x = std::max(-kMoveSpeed, differenceToTakeoff);
		}

		return;
	}

	/*========== 踏切位置で停止 ==========*/
	if (chaseJumpState_ == ChaseJumpState::kTakeoffPause) {

		velocity_.x = 0.0f;
		return;
	}

	/*========== ジャンプ中 ==========*/
	if (chaseJumpState_ == ChaseJumpState::kJumping) {

		const float halfHeight = kHeight / 2.0f;

		float enemyBottom = worldTransform_.translation_.y - halfHeight;

		/*
		 * 計画した足場ジャンプの場合、
		 * 敵の下端が足場上面を越えるまでは
		 * 横移動を行わない。
		 */
		if (!hasClearedTargetPlatformTop_) {
			if (enemyBottom >= targetPlatformTopY_ + kPlatformTopClearance) {

				hasClearedTargetPlatformTop_ = true;
			}
		}

		if (hasClearedTargetPlatformTop_) {
			/*
			 * 足場上面を越えた後は、
			 * 足場内の着地目標へ移動する。
			 */
			float differenceToLanding = targetPlatformLandingX_ - worldTransform_.translation_.x;

			if (std::abs(differenceToLanding) <= kPlatformLandingArrivalDistance) {

				// 着地位置まで来たので横移動を停止
				velocity_.x = 0.0f;

			} else if (differenceToLanding > 0.0f) {

				// 残り距離を超えないように右へ移動
				velocity_.x = std::min(kPlatformTransferSpeed, differenceToLanding);

			} else {

				// 残り距離を超えないように左へ移動
				velocity_.x = std::max(-kPlatformTransferSpeed, differenceToLanding);
			}

		} else {
			// 足場外側で垂直に上昇
			velocity_.x = 0.0f;
		}

		return;
	}

	/*========== 着地後待機 ==========*/
	if (chaseJumpState_ == ChaseJumpState::kLandingWait) {

		bool playerMovedFar = std::abs(differenceX) > MapChipField::kBlockWidth * 1.5f;
		bool playerChangedHeight = std::abs(playerPos.y - plannedTargetY_) > kChaseReplanHeightThreshold;

		if (playerMovedFar || playerChangedHeight) {

			chaseJumpState_ = ChaseJumpState::kDirectChase;
			chaseLandingWaitTimer_ = 0.0f;

			// このまま通常追跡判定へ進む
		} else {
			velocity_.x = 0.0f;
			return;
		}
	}

	/*========== 通常追跡 ==========*/
	// 接地したプレイヤーが高所にいる場合、
	// 頭上の足場の踏切位置を探す
	if (isOnGround_ && target_->IsOnGround() && differenceY > kJumpHeightThreshold) {

		float foundTakeoffX = 0.0f;
		float foundJumpDirection = 0.0f;
		float foundPlatformTopY = 0.0f;

		bool foundPlatform = FindOverheadPlatformTakeoff(foundTakeoffX, foundJumpDirection, foundPlatformTopY);

		if (foundPlatform) {
			takeoffTargetX_ = foundTakeoffX;
			plannedJumpDirection_ = foundJumpDirection;
			targetPlatformTopY_ = foundPlatformTopY;

			/*
			 * 踏み切り位置から足場側へ、
			 * 敵1体分と余白だけ進んだ位置を
			 * 着地目標にする。
			 */
			targetPlatformLandingX_ = takeoffTargetX_ + plannedJumpDirection_ * (kWidth + kRootMapMargin * 2.0f);

			plannedTargetY_ = playerPos.y;
			hasClearedTargetPlatformTop_ = false;

			chaseJumpState_ = ChaseJumpState::kMoveToTakeoff;

			float differenceToTakeoff = takeoffTargetX_ - worldTransform_.translation_.x;

			velocity_.x = differenceToTakeoff >= 0.0f ? kMoveSpeed : -kMoveSpeed;

			return;
		}
	}

	/*========== 下方向の追跡 ==========*/
	if (isOnGround_ && target_->IsOnGround() && differenceY < -kDropHeightThreshold) {

		float foundDropTargetX = 0.0f;
		float foundDropDirection = 0.0f;

		if (FindCurrentPlatformDropEdge(foundDropTargetX, foundDropDirection)) {

			dropTargetX_ = foundDropTargetX;
			dropDirection_ = foundDropDirection;

			// この時点の追跡対象の高さを保存
			plannedTargetY_ = playerPos.y;

			chaseJumpState_ = ChaseJumpState::kMoveToDropEdge;

			float differenceToDrop = dropTargetX_ - worldTransform_.translation_.x;

			velocity_.x = differenceToDrop >= 0.0f ? kMoveSpeed : -kMoveSpeed;

			return;
		}
	}

	/*========== プレイヤーのX位置を直接追跡 ==========*/
	float chaseSpeed = isOnGround_ ? kMoveSpeed : kAirChaseMoveSpeed;

	/*========== 右方向へ追跡 ==========*/
	if (differenceX > kChaseStopDistance) {

		/*
		 * プレイヤーが上にいるのに
		 * 上方向の経路を作れていない場合、
		 * 足場端から勝手に落ちないようにする。
		 */
		if (isOnGround_ && differenceY > kJumpHeightThreshold && !HasFloorAhead(1.0f)) {

			velocity_.x = 0.0f;
			return;
		}

		velocity_.x = chaseSpeed;
		return;
	}

	/*========== 左方向へ追跡 ==========*/

	if (differenceX < -kChaseStopDistance) {

		if (isOnGround_ && differenceY > kJumpHeightThreshold && !HasFloorAhead(-1.0f)) {

			velocity_.x = 0.0f;
			return;
		}

		velocity_.x = -chaseSpeed;
		return;
	}

	velocity_.x = 0.0f;
}

// 追跡中のジャンプ判断
void Enemy::TryChaseJump() {
	// プレイヤーがいない場合は何もしない
	if (target_ == nullptr) {
		return;
	}

	// 下方向の追跡中はジャンプしない
	if (chaseJumpState_ == ChaseJumpState::kMoveToDropEdge || chaseJumpState_ == ChaseJumpState::kDropping) {
		return;
	}

	// 接地していない場合はジャンプしない
	if (!isOnGround_) {
		return;
	}

	// 踏み切り位置へ移動中はジャンプしない
	if (chaseJumpState_ == ChaseJumpState::kMoveToTakeoff) {
		return;
	}

	// 着地後の待機中はジャンプしない
	if (chaseJumpState_ == ChaseJumpState::kLandingWait) {
		return;
	}

	/*
	 * 計画された踏み切り位置で
	 * 停止している場合だけジャンプする。
	 */
	if (chaseJumpState_ != ChaseJumpState::kTakeoffPause) {
		return;
	}

	// ジャンプ前の停止時間が残っている
	if (takeoffPauseTimer_ > 0.0f) {
		return;
	}

	// 保存しておいた足場方向を使用
	chaseJumpDirection_ = plannedJumpDirection_;

	// 最初は垂直に上昇
	velocity_.x = 0.0f;
	velocity_.y = kChaseJumpSpeed;

	isOnGround_ = false;
	hasClearedTargetPlatformTop_ = false;

	chaseJumpState_ = ChaseJumpState::kJumping;

	chaseJumpCooldownTimer_ = kChaseJumpCooldownTime;
}

// 横移動に地形判定を適用する
// 壁に衝突した場合はtrue
bool Enemy::MoveHorizontalWithMap(float movementX) {
	if (movementX == 0.0f) {
		return false;
	}

	// マップがない場合はそのまま移動
	if (!mapChipField_) {
		worldTransform_.translation_.x += movementX;
		return false;
	}

	const float halfWidth = kWidth / 2.0f;
	const float halfHeight = kHeight / 2.0f;

	float nextX = worldTransform_.translation_.x + movementX;

	bool hitWall = false;

	if (movementX > 0.0f) {
		// 右側の上下2点
		Vector3 checkPositions[] = {
		    {
             nextX + halfWidth,
             worldTransform_.translation_.y + halfHeight - kRootMapMargin,
             worldTransform_.translation_.z,
		     },
		    {
             nextX + halfWidth,
             worldTransform_.translation_.y - halfHeight + kRootMapMargin,
             worldTransform_.translation_.z,
		     },
		};

		for (const Vector3& position : checkPositions) {
			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

			// 右端を壁の左端へ合わせる
			nextX = std::min(nextX, rect.left - halfWidth - kRootMapMargin);

			hitWall = true;
		}

	} else {
		// 左側の上下2点
		Vector3 checkPositions[] = {
		    {
             nextX - halfWidth,
             worldTransform_.translation_.y + halfHeight - kRootMapMargin,
             worldTransform_.translation_.z,
		     },
		    {
             nextX - halfWidth,
             worldTransform_.translation_.y - halfHeight + kRootMapMargin,
             worldTransform_.translation_.z,
		     },
		};

		for (const Vector3& position : checkPositions) {
			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

			// 左端を壁の右端へ合わせる
			nextX = std::max(nextX, rect.right + halfWidth + kRootMapMargin);

			hitWall = true;
		}
	}

	worldTransform_.translation_.x = nextX;

	return hitWall;
}

// 上方の足場から踏切位置を取得
// 見つかった場合はtrue
bool Enemy::FindOverheadPlatformTakeoff(float& takeoffX, float& jumpDirection, float& platformTopY) const {

	if (!mapChipField_ || !target_ || !isOnGround_) {
		return false;
	}

	MapChipField::IndexSet enemyIndex = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_);

	const float enemyX = worldTransform_.translation_.x;

	const float playerX = target_->GetWorldPos().x;

	const float takeoffMargin = kWidth / 2.0f + kRootMapMargin;

	bool foundReachablePlatform = false;

	float bestRouteCost = std::numeric_limits<float>::max();

	float bestTakeoffX = 0.0f;
	float bestJumpDirection = 0.0f;
	float bestPlatformTopY = 0.0f;

	const float playerY = target_->GetWorldPos().y;

	/*
	 * 敵に近い高さから順に調べる。
	 * 同じ高さに複数の足場がある場合は、
	 * 最も移動コストが小さいものを選ぶ。
	 */
	for (uint32_t step = 1; step <= kOverheadSearchRows; ++step) {

		if (enemyIndex.yIndex < step) {
			break;
		}

		uint32_t checkY = enemyIndex.yIndex - step;

		/*========== この高さの足場を横方向全体から探す ==========*/
		uint32_t xIndex = 0;

		while (xIndex < MapChipField::kNumBlockHorizontal) {

			if (mapChipField_->GetMapChipTypeByIndex(xIndex, checkY) != MapChipType::kBlock) {

				++xIndex;
				continue;
			}

			/*========== 連続した足場の左右端を取得 ==========*/
			uint32_t leftIndex = xIndex;
			uint32_t rightIndex = xIndex;

			while (rightIndex + 1 < MapChipField::kNumBlockHorizontal && mapChipField_->GetMapChipTypeByIndex(rightIndex + 1, checkY) == MapChipType::kBlock) {

				++rightIndex;
			}

			/*
			 * 次の検索では、
			 * 今見つけた足場の右隣から再開する。
			 */
			xIndex = rightIndex + 1;

			/*========== 壁を足場として扱わない ==========*/
			bool hasWalkableTop = false;

			for (uint32_t surfaceX = leftIndex; surfaceX <= rightIndex; ++surfaceX) {

				if (checkY == 0 || mapChipField_->GetMapChipTypeByIndex(surfaceX, checkY - 1) == MapChipType::kBlank) {

					hasWalkableTop = true;
					break;
				}
			}

			if (!hasWalkableTop) {
				continue;
			}

			MapChipField::Rect leftRect = mapChipField_->GetRectByIndex(leftIndex, checkY);
			MapChipField::Rect rightRect = mapChipField_->GetRectByIndex(rightIndex, checkY);

			float candidatePlatformTopY = leftRect.top;

			float leftTakeoffX = leftRect.left - takeoffMargin;
			float rightTakeoffX = rightRect.right + takeoffMargin;

			/*
			 * 踏み切り位置が現在の足場上にあるか。
			 * これにより、途中で崖から落ちる経路を除外する。
			 */
			bool canUseLeft = IsTakeoffReachableOnCurrentPlatform(leftTakeoffX);
			bool canUseRight = IsTakeoffReachableOnCurrentPlatform(rightTakeoffX);

			/*========== 左側から乗る経路を評価 ==========*/
			if (canUseLeft) {
				float landingX = leftTakeoffX + (kWidth + kRootMapMargin * 2.0f);

				float predictedEnemyY = candidatePlatformTopY + kHeight / 2.0f + kRootMapMargin;

				float heightDifference = std::abs(playerY - predictedEnemyY);

				float routeCost = std::abs(enemyX - leftTakeoffX) + std::abs(playerX - landingX) + heightDifference * 4.0f;

				if (routeCost < bestRouteCost) {
					bestRouteCost = routeCost;
					bestTakeoffX = leftTakeoffX;
					bestJumpDirection = 1.0f;
					bestPlatformTopY = candidatePlatformTopY;

					foundReachablePlatform = true;
				}
			}

			/*========== 右側から乗る経路を評価 ==========*/
			if (canUseRight) {
				float landingX = rightTakeoffX - (kWidth + kRootMapMargin * 2.0f);

				float predictedEnemyY = candidatePlatformTopY + kHeight / 2.0f + kRootMapMargin;

				float heightDifference = std::abs(playerY - predictedEnemyY);

				float routeCost = std::abs(enemyX - rightTakeoffX) + std::abs(playerX - landingX) + heightDifference * 4.0f;

				if (routeCost < bestRouteCost) {
					bestRouteCost = routeCost;
					bestTakeoffX = rightTakeoffX;
					bestJumpDirection = -1.0f;
					bestPlatformTopY = candidatePlatformTopY;

					foundReachablePlatform = true;
				}
			}
		}
	}

	// 全候補の中で最も経路評価が良かった足場を使う
	if (foundReachablePlatform) {
		takeoffX = bestTakeoffX;
		jumpDirection = bestJumpDirection;
		platformTopY = bestPlatformTopY;

		return true;
	}

	return false;
}

// 現在いる足場から降りる位置を取得
bool Enemy::FindCurrentPlatformDropEdge(float& dropTargetX, float& dropDirection) const {

	if (!mapChipField_ || !target_ || !isOnGround_) {
		return false;
	}

	const float halfWidth = kWidth / 2.0f;
	const float halfHeight = kHeight / 2.0f;

	/*========== 現在いる足場を取得 ==========*/
	Vector3 floorCheckPosition = {
	    worldTransform_.translation_.x,
	    worldTransform_.translation_.y - halfHeight - kRootMapMargin,
	    worldTransform_.translation_.z,
	};

	MapChipField::IndexSet floorIndex = mapChipField_->GetMapChipIndexSetByPosition(floorCheckPosition);

	if (mapChipField_->GetMapChipTypeByIndex(floorIndex.xIndex, floorIndex.yIndex) != MapChipType::kBlock) {
		return false;
	}

	/*========== 足場の左右端を取得 ==========*/
	uint32_t leftIndex = floorIndex.xIndex;
	uint32_t rightIndex = floorIndex.xIndex;

	while (leftIndex > 0) {
		if (mapChipField_->GetMapChipTypeByIndex(leftIndex - 1, floorIndex.yIndex) != MapChipType::kBlock) {
			break;
		}

		--leftIndex;
	}

	while (rightIndex + 1 < MapChipField::kNumBlockHorizontal) {

		if (mapChipField_->GetMapChipTypeByIndex(rightIndex + 1, floorIndex.yIndex) != MapChipType::kBlock) {
			break;
		}

		++rightIndex;
	}

	MapChipField::Rect leftRect = mapChipField_->GetRectByIndex(leftIndex, floorIndex.yIndex);
	MapChipField::Rect rightRect = mapChipField_->GetRectByIndex(rightIndex, floorIndex.yIndex);

	/*========== 左右の落下位置を作る ==========*/
	float leftDropX = leftRect.left - halfWidth - kRootMapMargin - kDropEdgeExtraMargin;
	float rightDropX = rightRect.right + halfWidth + kRootMapMargin + kDropEdgeExtraMargin;

	/*========== 左右の落下先を調べる ==========*/
	float leftLandingTopY = 0.0f;
	float rightLandingTopY = 0.0f;

	bool canDropLeft = FindLandingPlatformBelow(leftDropX, leftLandingTopY);

	bool canDropRight = FindLandingPlatformBelow(rightDropX, rightLandingTopY);

	Vector3 playerPos = target_->GetWorldPos();

	float currentHeightDifference = std::abs(playerPos.y - worldTransform_.translation_.y);

	/*========== 左の落下先を評価 ==========*/

	if (canDropLeft) {
		// 左側へ落下した場合の着地後の敵中心Y
		float predictedEnemyY = leftLandingTopY + halfHeight + kRootMapMargin;

		float nextHeightDifference = std::abs(playerPos.y - predictedEnemyY);

		/*
		 * 落下してもプレイヤーとの高低差が
		 * 小さくならないなら候補から外す。
		 */
		if (nextHeightDifference >= currentHeightDifference) {
			canDropLeft = false;
		}
	}

	/*========== 右の落下先を評価 ==========*/
	if (canDropRight) {
		float predictedEnemyY = rightLandingTopY + halfHeight + kRootMapMargin;

		float nextHeightDifference = std::abs(playerPos.y - predictedEnemyY);

		if (nextHeightDifference >= currentHeightDifference) {
			canDropRight = false;
		}
	}

	/*========== 有効な降り口がない ==========*/
	if (!canDropLeft && !canDropRight) {
		return false;
	}

	/*========== 片側だけ有効 ==========*/
	if (canDropLeft && !canDropRight) {
		dropTargetX = leftDropX;
		dropDirection = -1.0f;
		return true;
	}

	if (!canDropLeft && canDropRight) {
		dropTargetX = rightDropX;
		dropDirection = 1.0f;
		return true;
	}

	/*========== 両側が有効なら総距離を比較 ==========*/
	float enemyX = worldTransform_.translation_.x;

	float leftRouteCost = std::abs(enemyX - leftDropX) + std::abs(playerPos.x - leftDropX);
	float rightRouteCost = std::abs(enemyX - rightDropX) + std::abs(playerPos.x - rightDropX);

	if (leftRouteCost <= rightRouteCost) {
		dropTargetX = leftDropX;
		dropDirection = -1.0f;
	} else {
		dropTargetX = rightDropX;
		dropDirection = 1.0f;
	}

	return true;
}

// 指定したX位置から落下した場合の着地面を探す
bool Enemy::FindLandingPlatformBelow(float dropX, float& landingTopY) const {

	if (!mapChipField_) {
		return false;
	}

	Vector3 startPosition = {
	    dropX,
	    worldTransform_.translation_.y,
	    worldTransform_.translation_.z,
	};

	MapChipField::IndexSet startIndex = mapChipField_->GetMapChipIndexSetByPosition(startPosition);

	// CSVでは下へ行くほどyIndexが大きくなる
	for (uint32_t yIndex = startIndex.yIndex + 1; yIndex < MapChipField::kNumBlockVertical; ++yIndex) {

		if (mapChipField_->GetMapChipTypeByIndex(startIndex.xIndex, yIndex) != MapChipType::kBlock) {
			continue;
		}

		MapChipField::Rect landingRect = mapChipField_->GetRectByIndex(startIndex.xIndex, yIndex);

		landingTopY = landingRect.top;
		return true;
	}

	return false;
}

// 指定方向の少し先に床があるか
// 指定方向の少し先に床があるか
bool Enemy::HasFloorAhead(float direction) const {

	if (!mapChipField_ || std::abs(direction) < 0.001f) {
		return false;
	}

	const float halfWidth = kWidth / 2.0f;
	const float halfHeight = kHeight / 2.0f;

	Vector3 checkPosition = {
	    worldTransform_.translation_.x + direction * (halfWidth + kMoveSpeed + kRootMapMargin),
	    worldTransform_.translation_.y - halfHeight - kRootMapMargin,
	    worldTransform_.translation_.z,
	};

	MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(checkPosition);

	return mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipType::kBlock;
}

// 指定したX座標まで現在の足場上を歩いて到達できるか
bool Enemy::IsTakeoffReachableOnCurrentPlatform(float takeoffX) const {

	if (!mapChipField_ || !isOnGround_) {
		return false;
	}

	const float halfWidth = kWidth / 2.0f;
	const float halfHeight = kHeight / 2.0f;

	// 敵の現在の足元を調べる
	Vector3 floorCheckPosition = {
	    worldTransform_.translation_.x,

	    worldTransform_.translation_.y - halfHeight - kRootMapMargin,

	    worldTransform_.translation_.z,
	};

	MapChipField::IndexSet floorIndex = mapChipField_->GetMapChipIndexSetByPosition(floorCheckPosition);

	if (mapChipField_->GetMapChipTypeByIndex(floorIndex.xIndex, floorIndex.yIndex) != MapChipType::kBlock) {
		return false;
	}

	// 現在いる足場の左右端を探す
	uint32_t leftIndex = floorIndex.xIndex;
	uint32_t rightIndex = floorIndex.xIndex;

	while (leftIndex > 0) {
		if (mapChipField_->GetMapChipTypeByIndex(leftIndex - 1, floorIndex.yIndex) != MapChipType::kBlock) {
			break;
		}

		--leftIndex;
	}

	while (rightIndex + 1 < MapChipField::kNumBlockHorizontal) {

		if (mapChipField_->GetMapChipTypeByIndex(rightIndex + 1, floorIndex.yIndex) != MapChipType::kBlock) {
			break;
		}

		++rightIndex;
	}

	MapChipField::Rect leftRect = mapChipField_->GetRectByIndex(leftIndex, floorIndex.yIndex);
	MapChipField::Rect rightRect = mapChipField_->GetRectByIndex(rightIndex, floorIndex.yIndex);

	/*
	 * 敵全体が現在の足場上に収まる範囲。
	 * 余白分だけ判定を緩める。
	 */
	float reachableLeft = leftRect.left + halfWidth - kRootMapMargin;
	float reachableRight = rightRect.right - halfWidth + kRootMapMargin;

	return takeoffX >= reachableLeft && takeoffX <= reachableRight;
}

// 敵同士の重なりを解消するために横移動する
// 実際に移動できた量を返す
float Enemy::MoveForEnemySeparation(float movementX) {

	if (!CanResolveEnemyOverlap() || std::abs(movementX) < 0.001f) {
		return 0.0f;
	}

	float beforeX = worldTransform_.translation_.x;

	/*
	 * 既存の地形判定を利用して移動する。
	 * これにより、押された敵が壁へ入り込まない。
	 */
	MoveHorizontalWithMap(movementX);

	float actualMovementX = worldTransform_.translation_.x - beforeX;

	/*
	 * 通常敵が進行方向と反対へ押し戻された場合、
	 * そのフレームの追跡移動を停止する。
	 */
	if (behavior_ == BehaviorEnemy::kRoot && !IsUsingPlatformRoute() && velocity_.x * movementX < 0.0f) {
		velocity_.x = 0.0f;
	}

	/*
	 * Enemy::Update()後に位置を変更するため、
	 * 描画用行列もここで更新する。
	 */
	transform_.worldMatrixUpdate(worldTransform_);

	return actualMovementX;
}

// 出現行動初期化
void Enemy::BehaviorSpawnInitialize() {
	spawnTimer_ = 0.0f;

	velocity_ = {};

	isOnGround_ = false;

	// 出現中は攻撃・接触・敵同士の押し合いを無効化
	isCollisionDisenabled_ = true;

	worldTransform_.scale_ = {
	    kSpawnStartScale,
	    kSpawnStartScale,
	    kSpawnStartScale,
	};
}

// 出現行動更新
void Enemy::BehaviorSpawnUpdate() {
	const float deltaTime = 1.0f / 60.0f;

	spawnTimer_ += deltaTime;

	/*========== 膨張 ==========*/

	if (spawnTimer_ < kSpawnExpandTime) {
		float t = std::clamp(spawnTimer_ / kSpawnExpandTime, 0.0f, 1.0f);

		// 滑らかに膨らませる
		float easeT = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);

		float scale = kSpawnStartScale + (kSpawnExpandScale - kSpawnStartScale) * easeT;

		worldTransform_.scale_ = {
		    scale,
		    scale,
		    scale,
		};

		// 膨張中はその場に滞空
		velocity_ = {};
		return;
	}

	/*========== 滞空 ==========*/

	float hoverTimer = spawnTimer_ - kSpawnExpandTime;

	if (hoverTimer < kSpawnHoverTime) {
		float t = std::clamp(hoverTimer / kSpawnHoverTime, 0.0f, 1.0f);

		/*
		 * 少し大きく膨らんだ状態から
		 * 通常サイズへ戻す。
		 */
		float smoothT = t * t * (3.0f - 2.0f * t);

		float scale = kSpawnExpandScale + (kNormalScale - kSpawnExpandScale) * smoothT;

		worldTransform_.scale_ = {
		    scale,
		    scale,
		    scale,
		};

		velocity_ = {};
		return;
	}

	/*========== 落下 ==========*/

	worldTransform_.scale_ = {
	    kNormalScale,
	    kNormalScale,
	    kNormalScale,
	};

	/*
	 * スタン状態用の処理には、
	 * 横移動を行わない重力・床・天井判定が
	 * すでにまとまっているため再利用する。
	 */
	UpdateStunnedMapMovement();

	// 着地後に通常行動へ移行
	if (isOnGround_) {
		behaviorRequest_ = BehaviorEnemy::kRoot;
	}
}

// 通常行動初期化
void Enemy::BehaviorRootInitialize() {
	isCollisionDisenabled_ = false;

	worldTransform_.rotation_.x = 0.0f;
	worldTransform_.rotation_.z = 0.0f;

	// スタン前の向きに合わせて移動を再開
	if (lrDirection_ == EnemyLRDirection::kRight) {

		velocity_.x = kMoveSpeed;

	} else {
		velocity_.x = -kMoveSpeed;
	}

	velocity_.y = 0.0f;
	velocity_.z = 0.0f;

	isOnGround_ = false;

	// 復帰時に不要な旋回を発生させない
	turnTimer_ = 0.0f;

	// 復帰時に追跡ジャンプをすぐに行わないようにする
	chaseJumpCooldownTimer_ = std::max(chaseJumpCooldownTimer_, 0.20f);

	chaseJumpState_ = ChaseJumpState::kDirectChase;

	chaseJumpDirection_ = 0.0f;
	chaseLandingWaitTimer_ = 0.0f;
}

// 通常行動更新
void Enemy::BehaviorRootUpdate() {
	const float deltaTime = 1.0f / 60.0f;

	// チュートリアル敵は追跡や自発的な横移動をしない
	if (purpose_ != EnemyPurpose::kNormal) {
		velocity_.x = 0.0f;

		// 重力、床、壁との判定は実行する
		UpdateRootMapMovement();

		return;
	}

	/*========== 上方向ジャンプの着地判定 ==========*/
	if (chaseJumpState_ == ChaseJumpState::kJumping && isOnGround_) {

		// 計画した足場ジャンプの場合、着地位置の高さが想定通りか確認する
		const float expectedEnemyY = targetPlatformTopY_ + kHeight / 2.0f + kRootMapMargin;
		const float landingHeightError = std::abs(worldTransform_.translation_.y - expectedEnemyY);
		bool landedSuccessfully = landingHeightError <= kPlatformLandingHeightTolerance;

		if (landedSuccessfully) {
			chaseJumpState_ = ChaseJumpState::kLandingWait;

			chaseLandingWaitTimer_ = kChaseLandingWaitTime;

		} else {
			chaseJumpState_ = ChaseJumpState::kDirectChase;

			chaseLandingWaitTimer_ = 0.0f;
			chaseJumpCooldownTimer_ = 0.15f;

			hasClearedTargetPlatformTop_ = false;
			chaseJumpDirection_ = 0.0f;
			plannedJumpDirection_ = 0.0f;
		}

		velocity_.x = 0.0f;
	}

	/*========== 下方向落下の着地判定 ==========*/

	if (chaseJumpState_ == ChaseJumpState::kDropping && isOnGround_) {

		chaseJumpState_ = ChaseJumpState::kLandingWait;
		chaseLandingWaitTimer_ = kChaseLandingWaitTime;

		// 現在の着地点から再計画できるよう更新
		if (target_) {
			plannedTargetY_ = target_->GetWorldPos().y;
		}

		velocity_.x = 0.0f;
	}

	// ジャンプ後の着地待機時間を減算
	if (chaseJumpState_ == ChaseJumpState::kLandingWait) {

		chaseLandingWaitTimer_ -= deltaTime;

		if (chaseLandingWaitTimer_ <= 0.0f) {
			chaseLandingWaitTimer_ = 0.0f;

			chaseJumpState_ = ChaseJumpState::kDirectChase;
		}
	}

	/*========== 踏切位置での停止時間 ==========*/
	if (chaseJumpState_ == ChaseJumpState::kTakeoffPause) {

		takeoffPauseTimer_ -= deltaTime;

		takeoffPauseTimer_ = std::max(takeoffPauseTimer_, 0.0f);
	}

	/*========== クールタイム ==========*/
	if (chaseJumpCooldownTimer_ > 0.0f) {
		chaseJumpCooldownTimer_ -= deltaTime;

		chaseJumpCooldownTimer_ = std::max(chaseJumpCooldownTimer_, 0.0f);
	}

	/*========== 既存の追跡処理 ==========*/
	if (!isHitKnockBack_ && !isHitRecovery_) {
		UpdateChaseDirection();

	} else {
		velocity_.x = 0.0f;
	}

	UpdateFacingDirection();
	UpdateRootMapMovement();

	/*========== アニメーション ==========*/
	// 歩行中だけアニメーションさせる
	if (!isHitKnockBack_ && std::abs(velocity_.x) > 0.001f) {

		walkTimer_ += 1.0f / 60.0f;

		float param = std::sin((walkTimer_ / kWalkMotionTime) * 2.0f * std::numbers::pi_v<float>);
		float degree = kWalkMotionAngleStart + kWalkMotionAngleEnd * ((param + 1.0f) / 2.0f);

		worldTransform_.rotation_.x = degree * std::numbers::pi_v<float> / 180.0f;

	} else {
		// 停止中は傾きを中央へ戻す
		worldTransform_.rotation_.x = 0.0f;
	}
}

// 死亡アクション初期化
void Enemy::BehaviorDeathInitialize() {
	deathTimer_ = 0.0f;

	velocity_ = {};
	blownAwayVelocity_ = {};

	isHitKnockBack_ = false;
	isBlownAwayStopped_ = true;
	canHitEnemyInCurrentBounce_ = false;

	// 死亡開始時の大きさを保存
	deathStartScale_ = worldTransform_.scale_;

	hasCreatedDeathBurstEffect_ = false;

	// 死亡演出中はすべての判定を無効化
	isCollisionDisenabled_ = true;

	isHitRecovery_ = false;
	hitRecoveryTimer_ = 0.0f;
}

// 死亡アクション更新
void Enemy::BehaviorDeathUpdate() {
	const float deltaTime = 1.0f / 60.0f;

	deathTimer_ += deltaTime;

	/*========== 膨張 ==========*/

	if (deathTimer_ <= kDeathExpandTime) {
		float t = std::clamp(deathTimer_ / kDeathExpandTime, 0.0f, 1.0f);

		float scaleRate = EaseOut(1.0f, kDeathExpandScaleRate, t);

		worldTransform_.scale_ = {
		    deathStartScale_.x * scaleRate,
		    deathStartScale_.y * scaleRate,
		    deathStartScale_.z * scaleRate,
		};

		return;
	}

	/*========== 破裂開始時のエフェクト ==========*/

	if (!hasCreatedDeathBurstEffect_) {
		hasCreatedDeathBurstEffect_ = true;

		if (gameScene_) {
			gameScene_->CreateHitEffect(GetWorldPos(), HitEffectType::kHit);
		}
	}

	/*========== 急収縮 ==========*/
	float shrinkTimer = deathTimer_ - kDeathExpandTime;

	float t = std::clamp(shrinkTimer / kDeathShrinkTime, 0.0f, 1.0f);

	float expandedRate = kDeathExpandScaleRate;

	// 膨らんだ状態から一気に0へ
	float scaleRate = EaseIn(expandedRate, 0.0f, t);

	worldTransform_.scale_ = {
	    deathStartScale_.x * scaleRate,
	    deathStartScale_.y * scaleRate,
	    deathStartScale_.z * scaleRate,
	};

	// 消滅
	if (t >= 1.0f) {
		isDead_ = true;
	}
}

// スタン初期化
void Enemy::BehaviorStunnedInitialize() {
	stunnedTimer_ = 0.0f;
	stunnedMotionTimer_ = 0.0f;

	velocity_ = {};

	isHitKnockBack_ = false;
	hitKnockBackTimer_ = 0.0f;

	isHitRecovery_ = false;
	hitRecoveryTimer_ = 0.0f;

	isCollisionDisenabled_ = false;

	worldTransform_.rotation_.x = kStunnedLookUpAngle * std::numbers::pi_v<float> / 180.0f;

	worldTransform_.rotation_.z = 0.0f;

	// 周囲の通常敵を軽く弾く
	if (gameScene_) {
		gameScene_->PushEnemiesAroundStunned(this);
	}
}

// スタン更新
void Enemy::BehaviorStunnedUpdate() {
	const float deltaTime = 1.0f / 60.0f;

	stunnedTimer_ += deltaTime;
	stunnedMotionTimer_ += deltaTime;

	/*========== 重力・床・足場判定 ==========*/
	UpdateStunnedMapMovement();

	/*========== スタン演出 ==========*/

	// 上向き角度を維持
	worldTransform_.rotation_.x = kStunnedLookUpAngle * std::numbers::pi_v<float> / 180.0f;

	// 上を向いたまま左右へ小刻みに震える
	float shakeDegree = std::sin(stunnedMotionTimer_ * kStunnedShakeSpeed) * kStunnedShakeAngle;

	worldTransform_.rotation_.z = shakeDegree * std::numbers::pi_v<float> / 180.0f;

	/*========== スタン終了 ==========*/
	// 通常敵だけ3秒でスタンを解除する
	const float stunDuration = purpose_ == EnemyPurpose::kTutorialLauncher ? 10.0f : kStunnedTime;

	if (stunnedTimer_ >= stunDuration) {
		stunHitCount_ = 0;
		behaviorRequest_ = BehaviorEnemy::kRoot;
	}
}

// 吹っ飛び初期化
void Enemy::BehaviorBlownAwayInitialize() {
	blownAwayTimer_ = 0.0f;

	bounceCount_ = 0;
	isBlownAwayStopped_ = false;

	// 最初の吹き飛び区間は1ヒット可能
	canHitEnemyInCurrentBounce_ = true;

	isHitKnockBack_ = false;
	hitKnockBackTimer_ = 0.0f;

	velocity_ = {};

	// 正面から斜め上までのランダム角度
	float initialAngleDegree = 0.0f;

	if (purpose_ == EnemyPurpose::kTutorialLauncher) {
		/*
		 * チュートリアル標的を右側へ配置しているため、
		 * 必ず右斜め上へ飛ばす。
		 */
		blownAwayDirection_ = 1.0f;

		// 地面との判定を避けつつ、ほぼ水平に飛ばす
		initialAngleDegree = 5.0f;

	} else {
		// 通常プレイは従来どおりランダム
		initialAngleDegree = RandomFloat(kInitialBlownAwayAngleMin, kInitialBlownAwayAngleMax);
	}

	// 度からラジアンへ変換
	float initialAngle = initialAngleDegree * std::numbers::pi_v<float> / 180.0f;

	// 速度の大きさを維持しながら
	// XとY成分に分解
	blownAwayVelocity_.x = blownAwayDirection_ * std::cos(initialAngle) * kBlownAwaySpeed;

	blownAwayVelocity_.y = std::sin(initialAngle) * kBlownAwaySpeed;

	blownAwayVelocity_.z = 0.0f;

	stunHitCount_ = 0;
	// 地形や敵とのAABB判定は残す
	isCollisionDisenabled_ = false;

	isHitRecovery_ = false;
	hitRecoveryTimer_ = 0.0f;
}

// 吹っ飛び更新
void Enemy::BehaviorBlownAwayUpdate() {
	KamataEngine::DebugText::GetInstance()->ConsolePrintf("BlownAwayUpdate: velocity=(%f, %f) stopped=%d\n", blownAwayVelocity_.x, blownAwayVelocity_.y, isBlownAwayStopped_);

	const float deltaTime = 1.0f / 60.0f;

	blownAwayTimer_ += deltaTime;

	// 反射が弱くなって停止した場合は、
	// デバッグ中なのでその場に残す
	if (isBlownAwayStopped_) {
		return;
	}

	// マップが設定されていない場合は、
	// 従来どおり移動だけ行う
	if (!mapChipField_) {
		worldTransform_.translation_.x += blownAwayVelocity_.x;
		worldTransform_.translation_.y += blownAwayVelocity_.y;

		if (blownAwayTimer_ >= kBlownAwayFlyingTime) {

			blownAwayVelocity_.x *= kBlownAwayStopAttenuation;
			blownAwayVelocity_.y *= kBlownAwayStopAttenuation;

			float speed = std::sqrt(blownAwayVelocity_.x * blownAwayVelocity_.x + blownAwayVelocity_.y * blownAwayVelocity_.y);

			if (speed <= kBlownAwayStopSpeed) {
				blownAwayVelocity_ = {};
				isBlownAwayStopped_ = true;
				canHitEnemyInCurrentBounce_ = false;

				// 破裂死亡演出へ移行
				behaviorRequest_ = BehaviorEnemy::kDeath;
			}
		}

		return;
	}

	bool bouncedThisFrame = false;
	bool hitX = false;
	bool hitY = false;

	const float halfWidth = kWidth / 2.0f;
	const float halfHeight = kHeight / 2.0f;

	/*========== X方向の移動・壁判定 ==========*/
	float nextX = worldTransform_.translation_.x + blownAwayVelocity_.x;

	if (blownAwayVelocity_.x > 0.0f) {
		// 右へ移動している場合、右上と右下を確認
		Vector3 rightTop = {
		    nextX + halfWidth,
		    worldTransform_.translation_.y + halfHeight - kBlownAwayMargin,
		    0.0f,
		};

		Vector3 rightBottom = {
		    nextX + halfWidth,
		    worldTransform_.translation_.y - halfHeight + kBlownAwayMargin,
		    0.0f,
		};

		Vector3 checkPositions[] = {
		    rightTop,
		    rightBottom,
		};

		bool hitRight = false;
		float resolvedX = nextX;

		for (const Vector3& position : checkPositions) {

			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

			// 敵の右端が壁の左端へ合う位置
			float candidateX = rect.left - halfWidth - kBlownAwayMargin;

			resolvedX = std::min(resolvedX, candidateX);

			hitRight = true;
		}

		if (hitRight) {
			nextX = resolvedX;

			blownAwayVelocity_.x = -blownAwayVelocity_.x;

			hitX = true;
			bouncedThisFrame = true;
		}
	} else if (blownAwayVelocity_.x < 0.0f) {
		// 左へ移動している場合、左上と左下を確認
		Vector3 leftTop = {
		    nextX - halfWidth,
		    worldTransform_.translation_.y + halfHeight - kBlownAwayMargin,
		    0.0f,
		};

		Vector3 leftBottom = {
		    nextX - halfWidth,
		    worldTransform_.translation_.y - halfHeight + kBlownAwayMargin,
		    0.0f,
		};

		Vector3 checkPositions[] = {
		    leftTop,
		    leftBottom,
		};

		bool hitLeft = false;
		float resolvedX = nextX;

		for (const Vector3& position : checkPositions) {

			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

			// 敵の左端が壁の右端へ合う位置
			float candidateX = rect.right + halfWidth + kBlownAwayMargin;

			resolvedX = std::max(resolvedX, candidateX);

			hitLeft = true;
		}

		if (hitLeft) {
			nextX = resolvedX;

			blownAwayVelocity_.x = -blownAwayVelocity_.x;

			hitX = true;
			bouncedThisFrame = true;
		}
	}

	/*========== 画面左右端との反射 ==========*/
	if (camera_) {
		// 敵全体が画面内に収まる範囲
		float screenLeft = camera_->translation_.x - kScreenHalfWidth + halfWidth + kScreenBounceMargin;

		float screenRight = camera_->translation_.x + kScreenHalfWidth - halfWidth - kScreenBounceMargin;

		/*========== 左端 ==========*/
		if (nextX < screenLeft) {
			nextX = screenLeft;

			// 左方向へ進んでいた場合だけ反射
			if (blownAwayVelocity_.x < 0.0f) {
				blownAwayVelocity_.x = -blownAwayVelocity_.x;

				hitX = true;
				bouncedThisFrame = true;
			}
		}

		/*========== 右端 ==========*/
		else if (nextX > screenRight) {
			nextX = screenRight;

			// 右方向へ進んでいた場合だけ反射
			if (blownAwayVelocity_.x > 0.0f) {
				blownAwayVelocity_.x = -blownAwayVelocity_.x;

				hitX = true;
				bouncedThisFrame = true;
			}
		}
	}

	// めり込みを解消したX座標を反映
	worldTransform_.translation_.x = nextX;

	/*========== Y方向の移動・床天井判定 ==========*/
	float nextY = worldTransform_.translation_.y + blownAwayVelocity_.y;

	bool hitFloor = false;

	if (blownAwayVelocity_.y > 0.0f) {
		// 上昇中：左上と右上を確認
		Vector3 leftTop = {
		    worldTransform_.translation_.x - halfWidth + kBlownAwayMargin,
		    nextY + halfHeight,
		    0.0f,
		};

		Vector3 rightTop = {
		    worldTransform_.translation_.x + halfWidth - kBlownAwayMargin,
		    nextY + halfHeight,
		    0.0f,
		};

		Vector3 checkPositions[] = {
		    leftTop,
		    rightTop,
		};

		bool hitCeiling = false;
		float resolvedY = nextY;

		for (const Vector3& position : checkPositions) {

			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

			// 敵の上端を天井の下端へ合わせる
			float candidateY = rect.bottom - halfHeight - kBlownAwayMargin;

			resolvedY = std::min(resolvedY, candidateY);

			hitCeiling = true;
		}

		if (hitCeiling) {
			nextY = resolvedY;

			blownAwayVelocity_.y = -blownAwayVelocity_.y;

			hitY = true;
			bouncedThisFrame = true;
		}

	} else if (blownAwayVelocity_.y < 0.0f) {
		// 落下中：左下と右下を確認
		Vector3 leftBottom = {
		    worldTransform_.translation_.x - halfWidth + kBlownAwayMargin,
		    nextY - halfHeight,
		    0.0f,
		};

		Vector3 rightBottom = {
		    worldTransform_.translation_.x + halfWidth - kBlownAwayMargin,
		    nextY - halfHeight,
		    0.0f,
		};

		Vector3 checkPositions[] = {
		    leftBottom,
		    rightBottom,
		};

		float resolvedY = nextY;

		for (const Vector3& position : checkPositions) {

			MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

			if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
				continue;
			}

			MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

			// 敵の下端を床の上端へ合わせる
			float candidateY = rect.top + halfHeight + kBlownAwayMargin;

			resolvedY = std::max(resolvedY, candidateY);

			hitFloor = true;
		}

		if (hitFloor) {
			nextY = resolvedY;

			blownAwayVelocity_.y = -blownAwayVelocity_.y;

			hitY = true;
			bouncedThisFrame = true;
		}
	}

	/*========== 画面上下端との反射 ==========*/
	if (camera_) {
		float screenBottom = camera_->translation_.y - kScreenHalfHeight + halfHeight + kScreenBounceMargin;

		float screenTop = camera_->translation_.y + kScreenHalfHeight - halfHeight - kScreenBounceMargin;

		/*========== 下端 ==========*/

		if (nextY < screenBottom) {
			nextY = screenBottom;

			// 下方向へ進んでいた場合だけ反射
			if (blownAwayVelocity_.y < 0.0f) {
				blownAwayVelocity_.y = -blownAwayVelocity_.y;

				hitY = true;
				bouncedThisFrame = true;
			}
		}

		/*========== 上端 ==========*/

		else if (nextY > screenTop) {
			nextY = screenTop;

			// 上方向へ進んでいた場合だけ反射
			if (blownAwayVelocity_.y > 0.0f) {
				blownAwayVelocity_.y = -blownAwayVelocity_.y;

				hitY = true;
				bouncedThisFrame = true;
			}
		}
	}

	// めり込みを解消したY座標を反映
	worldTransform_.translation_.y = nextY;

	/*========== 反射回数 ==========*/
	// 角へ衝突してXとYが同時に反射しても、
	// 1フレームにつき1回だけ加算
	if (bouncedThisFrame) {
		++bounceCount_;

		// 新しい飛行区間に入ったので、
		// 再び敵1体へ命中可能
		canHitEnemyInCurrentBounce_ = true;

		// 通常反射を行った直後の速度
		float reflectedX = blownAwayVelocity_.x;

		float reflectedY = blownAwayVelocity_.y;

		// 現在の移動速度の大きさ
		float speed = std::sqrt(reflectedX * reflectedX + reflectedY * reflectedY);

		if (speed > 0.0f) {
			// 現在の反射方向
			float angle = std::atan2(reflectedY, reflectedX);

			// ランダムな角度を生成
			float randomAngleDegree = RandomFloat(-kRandomBounceAngle, kRandomBounceAngle);

			// 度からラジアンへ変換
			float randomAngle = randomAngleDegree * std::numbers::pi_v<float> / 180.0f;

			// 反射方向にランダム角度を加える
			angle += randomAngle;

			// 速度の大きさを維持したまま
			// X・Y速度へ戻す
			float randomizedX = std::cos(angle) * speed;

			float randomizedY = std::sin(angle) * speed;

			// 壁に反射した場合、
			// ランダム化後に壁側へ戻らないようにする
			if (hitX && randomizedX * reflectedX < 0.0f) {

				randomizedX = -randomizedX;
			}

			// 床・天井に反射した場合も、
			// 衝突した地形側へ戻らないようにする
			if (hitY && randomizedY * reflectedY < 0.0f) {

				randomizedY = -randomizedY;
			}

			blownAwayVelocity_.x = randomizedX;

			blownAwayVelocity_.y = randomizedY;
		}
	}

	/*========== 飛行終了後の減速 ==========*/

	if (blownAwayTimer_ >= kBlownAwayFlyingTime) {

		blownAwayVelocity_.x *= kBlownAwayStopAttenuation;

		blownAwayVelocity_.y *= kBlownAwayStopAttenuation;

		float currentSpeed = std::sqrt(blownAwayVelocity_.x * blownAwayVelocity_.x + blownAwayVelocity_.y * blownAwayVelocity_.y);

		// 十分遅くなったら完全停止
		if (currentSpeed <= kBlownAwayStopSpeed) {

			blownAwayVelocity_ = {};
			isBlownAwayStopped_ = true;

			// 破裂死亡演出へ移行
			behaviorRequest_ = BehaviorEnemy::kDeath;
		}

		// 停止後は敵へダメージを与えない
		canHitEnemyInCurrentBounce_ = false;
	}

	/*========== 回転 ==========*/

	worldTransform_.rotation_.z += kBlownAwayRotateSpeed * blownAwayDirection_;

	/*========== デバッグ中は死亡させない ==========*/

	// 一時的に無効化
	//
	// if (blownAwayTimer_ >= kBlownAwayTime) {
	//     behaviorRequest_ =
	//         BehaviorEnemy::kDeath;
	// }
}

// 小ノックバック更新
void Enemy::UpdateHitKnockBack() {
	if (!isHitKnockBack_) {
		return;
	}

	const float deltaTime = 1.0f / 60.0f;

	hitKnockBackTimer_ += deltaTime;

	float t = std::clamp(hitKnockBackTimer_ / kHitKnockBackTime, 0.0f, 1.0f);

	// 最初は速く、徐々に減速
	float speed = currentHitKnockBackSpeed_ * (1.0f - t);

	float movementX = hitKnockBackDirection_ * speed;

	// ノックバックにも壁判定を適用
	bool hitWall = MoveHorizontalWithMap(movementX);

	// 時間終了、または壁に当たったら終了
	if (t >= 1.0f || hitWall) {
		isHitKnockBack_ = false;
		hitKnockBackTimer_ = 0.0f;

		// ノックバック後の硬直開始
		if (behavior_ == BehaviorEnemy::kRoot) {
			isHitRecovery_ = true;
			hitRecoveryTimer_ = kHitRecoveryTime;

			velocity_.x = 0.0f;
		}
	}
}

// 敵の描画
void Enemy::Draw() {
	// 敵を描画
	modelEnemy_->Draw(worldTransform_, *camera_);
}

Vector3 Enemy::GetWorldPos() {
	Vector3 worldPos;

	// ワールド行列の平行移動成分を取得(正しいやり方がワカンナイヨー)
	worldPos.x = worldTransform_.translation_.x;
	worldPos.y = worldTransform_.translation_.y;
	worldPos.z = worldTransform_.translation_.z;

	return worldPos;
}

AABB Enemy::GetAABB() {
	Vector3 worldPos = GetWorldPos();

	AABB aabb;

	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

bool Enemy::OnCollisionPlayer(Player* player) {
	if (behavior_ == BehaviorEnemy::kDeath || behavior_ == BehaviorEnemy::kBlownAway) {
		return false;
	}

	AttackType attackType = player->GetAttackType();

	// 通常攻撃は通常状態だけ受け付ける
	if (attackType == AttackType::kNormal && behavior_ != BehaviorEnemy::kRoot) {
		return false;
	}

	// 溜め攻撃は通常・スタン状態で受け付ける
	if (attackType == AttackType::kCharged && behavior_ != BehaviorEnemy::kRoot && behavior_ != BehaviorEnemy::kStunned) {
		return false;
	}

	uint32_t attackSerial = player->GetAttackSerial();

	// 同じ攻撃による複数ヒットを防止
	if (lastReceivedAttackSerial_ == attackSerial) {
		return false;
	}

	lastReceivedAttackSerial_ = attackSerial;

	float differenceX = worldTransform_.translation_.x - player->GetWorldTransform().translation_.x;

	float hitDirection = differenceX >= 0.0f ? 1.0f : -1.0f;

	// エフェクト位置
	Vector3 effectPos = worldTransform_.translation_;

	// 敵のプレイヤー側の表面に配置
	effectPos.x -= hitDirection * (kWidth * 0.2f);

	// 高さはプレイヤーと敵の位置を考慮する
	effectPos.y = std::clamp(player->GetWorldTransform().translation_.y, worldTransform_.translation_.y - kHeight * 0.35f, worldTransform_.translation_.y + kHeight * 0.35f);

	/*========== スタン中への溜め攻撃 ==========*/
	if (behavior_ == BehaviorEnemy::kStunned && attackType == AttackType::kCharged) {

		// ヒットストップ前に吹き飛び状態と初速を確定
		StartBlownAway(hitDirection);

		if (gameScene_) {
			gameScene_->CreateHitEffect(effectPos, HitEffectType::kChargedHit);

			gameScene_->StartChargedAttackHitStop();
		}

		return true;
	}

	/*========== HPダメージ ==========*/

	int32_t hpDamage = 0;

	switch (attackType) {
	case AttackType::kNormal:
		hpDamage = kNormalAttackHpDamage;
		break;

	case AttackType::kCharged:
		hpDamage = kChargedAttackHpDamage;
		break;
	}

	bool defeated = ApplyHpDamage(hpDamage);

	/*========== 生存中のリアクション ==========*/

	if (!defeated) {
		// 小ノックバック
		hitKnockBackDirection_ = hitDirection;

		currentHitKnockBackSpeed_ = kHitKnockBackSpeed;

		isHitKnockBack_ = true;
		hitKnockBackTimer_ = 0.0f;

		isHitRecovery_ = false;
		hitRecoveryTimer_ = 0.0f;

		// 通常攻撃だけスタン値を蓄積
		if (attackType == AttackType::kNormal && behavior_ == BehaviorEnemy::kRoot) {

			++stunHitCount_;

			if (stunHitCount_ >= kStunHitCount) {
				behaviorRequest_ = BehaviorEnemy::kStunned;
			}
		}
	}

	/*========== ヒットエフェクト ==========*/

	if (gameScene_) {
		gameScene_->CreateHitEffect(effectPos, HitEffectType::kHit);
	}

	return true;
}

// 当たり判定が無効化されているか
bool Enemy::IsCollisionDisEnabled() const { return isCollisionDisenabled_; }

// プレイヤーへ接触ダメージを与えられるか
bool Enemy::CanDamagePlayer() const {
	// チュートリアル敵はプレイヤーへ接触ダメージを与えない
	if (purpose_ != EnemyPurpose::kNormal) {
		return false;
	}

	return behavior_ == BehaviorEnemy::kRoot && !isHitKnockBack_;
}

// スタン衝撃による弱いノックバック開始
void Enemy::StartStunShockwaveKnockBack(float direction) {

	if (!CanReceiveStunShockwave()) {
		return;
	}

	hitKnockBackDirection_ = direction >= 0.0f ? 1.0f : -1.0f;

	currentHitKnockBackSpeed_ = kStunShockwaveKnockBackSpeed;

	isHitKnockBack_ = true;
	hitKnockBackTimer_ = 0.0f;

	isHitRecovery_ = false;
	hitRecoveryTimer_ = 0.0f;
}

// プレイヤーの溜め攻撃による吹き飛びを開始
void Enemy::StartBlownAway(float direction) {
	blownAwayDirection_ = direction >= 0.0f ? 1.0f : -1.0f;

	// 保留中の状態変更を取り消す
	behaviorRequest_ = BehaviorEnemy::kUnknown;

	// 吹き飛び状態を即座に確定
	behavior_ = BehaviorEnemy::kBlownAway;

	// 初速などを設定
	BehaviorBlownAwayInitialize();

	// ヒットストップ中にも正しい姿勢を描画できるようにする
	transform_.worldMatrixUpdate(worldTransform_);

	KamataEngine::DebugText::GetInstance()->ConsolePrintf("StartBlownAway: direction=%f velocity=(%f, %f)\n", blownAwayDirection_, blownAwayVelocity_.x, blownAwayVelocity_.y);
}

// 吹き飛び中で、他の敵へ攻撃できるか
bool Enemy::CanHitOtherEnemy() const { return behavior_ == BehaviorEnemy::kBlownAway && !isBlownAwayStopped_ && canHitEnemyInCurrentBounce_; }

// 吹き飛び敵の攻撃を受けられるか
bool Enemy::CanReceiveBlownAwayHit() const { return behavior_ == BehaviorEnemy::kRoot && !isHitKnockBack_ && !isDead_; }

// 現在の飛行区間の攻撃権を消費
void Enemy::ConsumeBlownAwayHit() { canHitEnemyInCurrentBounce_ = false; }

// 吹き飛んできた敵との衝突処理
void Enemy::OnCollisionBlownAwayEnemy(float attackDirection) {
	// 行動可能な敵だけが受け付ける
	if (!CanReceiveBlownAwayHit()) {
		return;
	}

	int32_t damage = kBlownAwayHitHpDamage;

	// チュートリアル標的は吹き飛ばし衝突1回で倒す
	if (purpose_ == EnemyPurpose::kTutorialTarget) {
		damage = hp_;
	}

	// 飛来した敵から大きなHPダメージを受ける
	bool defeated = ApplyHpDamage(damage);

	// 撃破された場合は、ノックバックやスタンで
	// 死亡リクエストを上書きしない
	if (defeated) {
		return;
	}

	// 飛来した敵との位置関係に応じてノックバック
	hitKnockBackDirection_ = attackDirection;
	isHitKnockBack_ = true;
	hitKnockBackTimer_ = 0.0f;

	// HPとは別にスタン値も蓄積
	stunHitCount_ += kBlownAwayHitStunDamage;

	if (stunHitCount_ >= kStunHitCount) {
		behaviorRequest_ = BehaviorEnemy::kStunned;
	}
}

float Enemy::GetBlownAwayDirectionX() const {
	if (blownAwayVelocity_.x > 0.0f) {
		return 1.0f;
	}

	if (blownAwayVelocity_.x < 0.0f) {
		return -1.0f;
	}

	return blownAwayDirection_;
}

// HPダメージを受ける
bool Enemy::ApplyHpDamage(int32_t damage) {
	if (behavior_ == BehaviorEnemy::kDeath || isDead_) {
		return false;
	}

	hp_ -= damage;
	hp_ = std::max(hp_, 0);

	if (hp_ <= 0) {
		// 他の状態への遷移が残らないようにする
		isHitKnockBack_ = false;
		hitKnockBackTimer_ = 0.0f;

		canHitEnemyInCurrentBounce_ = false;
		behaviorRequest_ = BehaviorEnemy::kDeath;

		return true;
	}

	return false;
}

void Enemy::SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }