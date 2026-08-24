#define NOMINMAX
#include "Player.h"
#include "Enemy.h"
#include "MapChipField.h"
#include <algorithm>
#include <cassert>
#include <numbers>

// コンストラクタ&デストラクタ
Player::Player() {}
Player::~Player() {}

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/// <summary>
/// 自機の初期化関数
/// </summary>
/// <param name="model">3Dモデル</param>
/// <param name="camera">カメラ</param>
void Player::Initialize(Model* model, Model* modelAttack, Camera* camera, const Vector3 pos) {
	// ぬるぽチェック
	assert(model);

	// 引き数の内容をメンバ変数に記録
	camera_ = camera;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();
	worldTransformAttack_.Initialize();

	// メンバ変数への代入処理
	// プレイヤーの拡縮,回転,平行移動情報
	worldTransform_.scale_ = {2, 2, 2};
	worldTransform_.translation_ = pos;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;

	worldTransformAttack_.scale_ = {2.0f, 2.0f, 2.0f};

	// 3Dモデルの生成
	model_ = model;
	modelAttack_ = modelAttack;

	canDoubleJump_ = true;

	hp_ = kMaxHp;
	isDead_ = false;
}

/// <summary>
/// 自機の更新
/// </summary>
void Player::Update() {
	// 振る舞いを変更する
	if (behaiviorRequest_ != Behavior::kUnKnown) {
		behaivior_ = behaiviorRequest_;
		// 各振る舞いごとの初期化を実行
		switch (behaivior_) {
		case Behavior::kRoot:
			// 通常行動初期化
			BehaviorRootInitialize();
			break;

		case Behavior::kAttack:
			// 攻撃行動初期化
			BehaviorAttackInitialize();
			break;

		case Behavior::kKnockBack:
			// ノックバック初期化
			BehaviorKnockBackInitialize();
			break;

		default:
			break;
		}
		// 振る舞いリクエストをリセット
		behaiviorRequest_ = Behavior::kUnKnown;
	}

	// Behaiviorの実行
	switch (behaivior_) {
	case Behavior::kRoot:
		// 通常行動更新
		BehaviorRootUpdate();
		break;

	case Behavior::kAttack:
		// 攻撃行動更新
		BehaviorAttackUpdate();
		break;

	case Behavior::kKnockBack:
		// ノックバック更新
		BehaviorKnockBackUpdate();
		break;

	default:
		break;
	}

	/*========== ②移動量を加味して衝突判定する ==========*/
	// 衝突情報を初期化
	CollisionMapInfo collisionMapInfo;
	// 移動量に速度の値をコピー
	collisionMapInfo.MovementAmount = velocity_;

	// 画面外に出ない様に
	bool isPushedByCamera = false;

	float left = camera_->translation_.x - 21.0f;
	float pushX = 0.0f;
	// 画面漏れを対処
	if (worldTransform_.translation_.x < left) {
		pushX = left - worldTransform_.translation_.x;
		isPushedByCamera = true;
	}

	// 圧死判定用に数値を管理
	collisionMapInfo.MovementAmount.x += pushX;

	// マップ衝突チェック
	MapCollisionCheck(collisionMapInfo);

	/*========== ③判定結果を反映して移動 ==========*/
	MoveReflectingResult(collisionMapInfo);

	/*========== ④天井に接触している時の処理 ==========*/
	ContactWithCeiling(collisionMapInfo);

	/*========== ⑤壁に接触している時の処理 ==========*/
	ContactWithWall(collisionMapInfo);

	/*========== ⑥接地状態の切り替え ==========*/
	SwitchGroundingState(collisionMapInfo);

	if (isPushedByCamera && collisionMapInfo.isWallCollide) {
		isDead_ = true;
	}

	// 攻撃エフェクトの位置更新
	if (isAttackEffect_) {
		float direction = lrDirection_ == LRDirection::kRight ? 1.0f : -1.0f;

		worldTransformAttack_.translation_ = worldTransform_.translation_;

		worldTransformAttack_.translation_.x += direction * 0.8f;

		worldTransformAttack_.translation_.y += 0.3f;

		transform_.worldMatrixUpdate(worldTransformAttack_);
	}

	/*========== ⑧行列計算 ==========*/
	// 行列を定数バッファに転送
	transform_.worldMatrixUpdate(worldTransform_);
}

// 通常行動初期化
void Player::BehaviorRootInitialize() {
	// 回転角を初期化
	turnTimer_ = 0.0f;
	turnFirstRotationY_ = worldTransform_.rotation_.y;
}

/// <summary>
/// 通常行動更新
/// </summary>
void Player::BehaviorRootUpdate() {
	// 攻撃キーを押したら
	if (Input::GetInstance()->TriggerKey(DIK_Z)) {
		// 地上なら何度でも攻撃可能
		if (onGround_) {
			// 攻撃ビヘイビアをリクエスト
			behaiviorRequest_ = Behavior::kAttack;

		} else if (canAirAttack_) {
			// 空中なら1回だけ
			// 攻撃ビヘイビアをリクエスト
			behaiviorRequest_ = Behavior::kAttack;
			canAirAttack_ = false;
		}
	}

	/*========== 左右移動 ==========*/
	float inputDirection = 0.0f;

	if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
		inputDirection += 1.0f;
	}

	if (Input::GetInstance()->PushKey(DIK_LEFT)) {
		inputDirection -= 1.0f;
	}

	if (inputDirection != 0.0f) {
		float targetSpeed = inputDirection * kLimitRunSpeed;

		float acceleration = onGround_ ? kGroundAcceleration : kAirAcceleration;

		// 現在速度を目標速度へ近づける
		float difference = targetSpeed - velocity_.x;

		velocity_.x += std::clamp(difference, -acceleration, acceleration);

		// 向きを更新
		LRDirection newDirection = inputDirection > 0.0f ? LRDirection::kRight : LRDirection::kLeft;

		if (lrDirection_ != newDirection) {
			lrDirection_ = newDirection;

			turnFirstRotationY_ = worldTransform_.rotation_.y;

			turnTimer_ = kTimeTurn;
		}

	} else if (onGround_) {
		// 入力がないときは素早く停止
		float difference = -velocity_.x;

		velocity_.x += std::clamp(difference, -kGroundDeceleration, kGroundDeceleration);
	}

	// 微小な速度を完全停止
	if (onGround_ && std::abs(velocity_.x) < 0.001f) {
		velocity_.x = 0.0f;
	}

	/*========== ジャンプ ==========*/
	// このフレームにジャンプしたか
	bool jumpedThisFrame = false;

	if (Input::GetInstance()->TriggerKey(DIK_UP)) {
		if (onGround_) {
			// 通常ジャンプ
			velocity_.y = kJumpAcceleration_;

			canDoubleJump_ = true;
			jumpedThisFrame = true;

		} else if (canDoubleJump_) {
			// 二段ジャンプ
			velocity_.y = kDoubleJumpAcceleration;

			canDoubleJump_ = false;
			jumpedThisFrame = true;
		}
	}

	// 空中かつ、ジャンプした直後でなければ重力を適用
	if (!onGround_ && !jumpedThisFrame) {
		velocity_.y -= kGravityAcceleration;

		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed_);
	}

	/*========== ⑦旋回制御 ==========*/
	if (turnTimer_ > 0.0f) {
		// 旋回タイマーを1/60秒だけカウントダウン
		turnTimer_ -= 1.0f / 60.0f;
		turnTimer_ = std::max(turnTimer_, 0.0f);

		// 補間係数t
		float t = 1.0f - (turnTimer_ / kTimeTurn);
		t = std::clamp(t, 0.0f, 1.0f);

		// EaseInOutの形にする
		float easeT = t * t * (3.0f - 2.0f * t);

		// 左右の自キャラ角度テーブル
		float destinationRotationYTable[] = {
		    std::numbers::pi_v<float> / 2.0f,
		    std::numbers::pi_v<float> * 3.0f / 2.0f,
		};

		// 状態に応じた角度を取得する
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		//	自キャラの角度を調整する
		// 旋回タイマーを使って角度を線形補間する
		worldTransform_.rotation_.y = turnFirstRotationY_ + (destinationRotationY - turnFirstRotationY_) * easeT;
	}
}

// 攻撃行動初期化
void Player::BehaviorAttackInitialize() {
	attackTimer_ = 0.0f;
	chargeTimer_ = 0.0f;

	attackType_ = AttackType::kNormal;
	attackPhase_ = AttackPhase::kCharging;

	isChargeReady_ = false;
	hasHitEnemy_ = false;

	// 溜め中は横移動を停止
	velocity_.x = 0.0f;

	// バットを表示
	isAttackEffect_ = true;
}

/// <summary>
/// 攻撃行動更新
/// </summary>
void Player::BehaviorAttackUpdate() {
	const float deltaTime = 1.0f / 60.0f;
	const float direction = lrDirection_ == LRDirection::kRight ? 1.0f : -1.0f;

	attackTimer_ += deltaTime;

	// 空中攻撃中は上昇・落下を滑らかに停止
	if (!onGround_) {
		velocity_.y *= kAirAttackVerticalDamping;

		if (std::abs(velocity_.y) < kAirAttackStopSpeed) {
			velocity_.y = 0.0f;
		}
	}

	switch (attackPhase_) {
	case AttackPhase::kCharging: {
		// Zキーを押している間
		if (Input::GetInstance()->PushKey(DIK_Z)) {
			chargeTimer_ += deltaTime;

			chargeTimer_ = std::min(chargeTimer_, kChargeMaxTime);

			float t = std::clamp(chargeTimer_ / kChargeRequiredTime, 0.0f, 1.0f);

			// 溜めるほど大きく振りかぶる
			float angleDegree = EaseOut(0.0f, kChargeBatAngle, t);

			worldTransformAttack_.rotation_.z = angleDegree * std::numbers::pi_v<float> / 180.0f * direction;

			// 規定時間に達した
			if (chargeTimer_ >= kChargeRequiredTime) {
				isChargeReady_ = true;
			}

			break;
		}

		// Zキーを離した瞬間
		attackType_ = isChargeReady_ ? AttackType::kCharged : AttackType::kNormal;

		// ここから1回の攻撃として扱う
		++attackSerial_;

		attackTimer_ = 0.0f;
		attackPhase_ = AttackPhase::kStartup;
		break;
	}

	case AttackPhase::kStartup: {
		float startupTime = attackType_ == AttackType::kCharged ? 0.12f : kAttackStartupTime;
		float t = std::clamp(attackTimer_ / startupTime, 0.0f, 1.0f);
		float startAngle = attackType_ == AttackType::kCharged ? kChargeBatAngle : kBatAngleStart;

		// 溜め中の角度から、攻撃開始位置へ合わせる
		float angleDegree = EaseOut(startAngle, kBatAngleStart, t);
		worldTransformAttack_.rotation_.z = angleDegree * std::numbers::pi_v<float> / 180.0f * direction;

		if (attackTimer_ >= startupTime) {
			attackTimer_ = 0.0f;
			attackPhase_ = AttackPhase::kActive;
		}

		break;
	}

	case AttackPhase::kActive: {
		float activeTime = attackType_ == AttackType::kCharged ? 0.16f : kAttackActiveTime;

		float stepSpeed = attackType_ == AttackType::kCharged ? 0.32f : kAttackStepSpeed;

		float swingStart = attackType_ == AttackType::kCharged ? kChargeBatAngle : kBatAngleStart;

		float t = std::clamp(attackTimer_ / activeTime, 0.0f, 1.0f);

		float angleDegree = EaseOut(swingStart, kBatAngleEnd, t);

		worldTransformAttack_.rotation_.z = angleDegree * std::numbers::pi_v<float> / 180.0f * direction;

		// 溜め攻撃は少し強く踏み込む
		velocity_.x = direction * stepSpeed * (1.0f - t);

		if (attackTimer_ >= activeTime) {
			velocity_.x = 0.0f;
			attackTimer_ = 0.0f;
			attackPhase_ = AttackPhase::kRecovery;
		}

		break;
	}

	case AttackPhase::kRecovery: {
		float recoveryTime = attackType_ == AttackType::kCharged ? 0.25f : kAttackRecoveryTime;

		float t = std::clamp(attackTimer_ / recoveryTime, 0.0f, 1.0f);

		float angleDegree = EaseOut(kBatAngleEnd, 0.0f, t);

		worldTransformAttack_.rotation_.z = angleDegree * std::numbers::pi_v<float> / 180.0f * direction;

		velocity_.x = 0.0f;

		if (attackTimer_ >= recoveryTime) {
			EndAttack();
			behaiviorRequest_ = Behavior::kRoot;
		}

		break;
	}

	case AttackPhase::kNone:
	default:
		break;
	}
}

/// <summary>
/// ノックバック初期化
/// </summary>
void Player::BehaviorKnockBackInitialize() {
	// タイマー初期化
	knockBackTimer_ = 0.0f;
	// 速度初期化
	velocity_.x = 0.0f;
	velocity_.y = 0.0f;
}

/// <summary>
/// ノックバック更新
/// </summary>
void Player::BehaviorKnockBackUpdate() {
	// 攻撃を終了させる
	EndAttack();
	// ノックバック時間を計測
	knockBackTimer_ += 1.0f / 60.0f;
	// ノックバック方向に移動
	velocity_.x = knockBackDirection_ * kKnockBackSpeed;
	// 少し浮かせる
	velocity_.y = 0.15f;
	// ノックバック時間が経過したら通常行動に戻す
	if (knockBackTimer_ >= kKnockBackTime) {
		behaiviorRequest_ = Behavior::kRoot;
	}
}

/// <summary>
/// 自機の描画
/// </summary>
void Player::Draw() {
	// 自機を描画
	model_->Draw(worldTransform_, *camera_);

	// 攻撃エフェクトを描画
	if (isAttackEffect_) {
		modelAttack_->Draw(worldTransformAttack_, *camera_);
	}
}

/// <summary>
/// マップ衝突判定
/// </summary>
/// <param name="info">マップとの当たり判定情報</param>
void Player::MapCollisionCheck(CollisionMapInfo& info) {
	// 方向別のマップ衝突判定
	MapCollisionCheckTop(info);    // 上方向
	MapCollisionCheckBottom(info); // 下方向
	MapCollisionCheckRight(info);  // 右方向
	MapCollisionCheckLeft(info);   // 左方向
}

/// <summary>
/// 指定した角の座標計算
/// </summary>
/// <param name="center">指定したい角の矩形の座標</param>
/// <param name="corner">指定した角</param>
Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 result = {};

	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, // 右下
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, // 左下
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0.0f}, // 右上
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0.0f}  // 左上
	};

	result.x = center.x + offsetTable[static_cast<uint32_t>(corner)].x;
	result.y = center.y + offsetTable[static_cast<uint32_t>(corner)].y;
	result.z = center.z + offsetTable[static_cast<uint32_t>(corner)].z;

	return result;
}

// 方向別のマップ衝突判定
// 上方向
void Player::MapCollisionCheckTop(CollisionMapInfo& info) {
	// 上昇しているか
	if (info.MovementAmount.y <= 0.0f) {
		return;
	}

	// 移動後の4つの角の座標
	std::array<Vector3, kNumCorner> positionsNew = {};

	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(
		    {worldTransform_.translation_.x + info.MovementAmount.x, worldTransform_.translation_.y + info.MovementAmount.y, worldTransform_.translation_.z + info.MovementAmount.z},
		    static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;

	// 真上の当たり判定を取る
	bool hit = false;

	// 左上点の判定
	MapChipField::IndexSet indexSet = {};
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// 右上点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// ブロックに当たっているか
	if (hit) {
		// めり込みを解消する方向に移動量を設定(GetMapChipIndexSetByPositionには自キャラの上端座標を入れる)
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);

		// 現在座標が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition({worldTransform_.translation_.x, worldTransform_.translation_.y + kHeight / 2.0f, worldTransform_.translation_.z});

		if (indexSetNow.yIndex != indexSet.yIndex) {
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.MovementAmount.y = std::max(0.0f, (rect.bottom - worldTransform_.translation_.y) - (kHeight / 2.0f + kMargin));
			// 天井に当たった
			info.isCeilingCollide = true;
		}
	}
}

// 下方向
void Player::MapCollisionCheckBottom(CollisionMapInfo& info) {
	// 降下しているか
	if (info.MovementAmount.y >= 0.0f) {
		return;
	}

	// 移動後の4つの角の座標
	std::array<Vector3, kNumCorner> positionsNew = {};

	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(
		    {worldTransform_.translation_.x + info.MovementAmount.x, worldTransform_.translation_.y + info.MovementAmount.y, worldTransform_.translation_.z + info.MovementAmount.z},
		    static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;

	// 真下の当たり判定を取る
	bool hit = false;

	// 左下点の判定
	MapChipField::IndexSet indexSet = {};
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// 右下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// ブロックに当たっているか
	if (hit) {
		// めり込みを解消する方向に移動量を設定(GetMapChipIndexSetByPositionには自キャラの下端座標を入れる)
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);

		// 現在座標が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition({worldTransform_.translation_.x, worldTransform_.translation_.y - kHeight / 2.0f, worldTransform_.translation_.z});

		if (indexSetNow.yIndex != indexSet.yIndex) {
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.MovementAmount.y = std::min(0.0f, (rect.top - worldTransform_.translation_.y) + (kHeight / 2.0f + kMargin));

			// 地面に当たった
			info.isLanding = true;
		}
	}
}

// 右方向
void Player::MapCollisionCheckRight(CollisionMapInfo& info) {

	// 右へ移動していなければ判定しない
	if (info.MovementAmount.x <= 0.0f) {
		return;
	}

	const float halfWidth = kWidth / 2.0f;
	const float halfHeight = kHeight / 2.0f;

	// 移動後の中心座標
	Vector3 nextCenter = {
	    worldTransform_.translation_.x + info.MovementAmount.x,

	    worldTransform_.translation_.y + info.MovementAmount.y,

	    worldTransform_.translation_.z,
	};

	// 右端の上下2点
	// 上下を少し内側にして、床や天井を壁と誤認しないようにする
	Vector3 checkPositions[] = {
	    {
         nextCenter.x + halfWidth,
         nextCenter.y + halfHeight - kMargin,
         nextCenter.z,
	     },
	    {
         nextCenter.x + halfWidth,
         nextCenter.y - halfHeight + kMargin,
         nextCenter.z,
	     },
	};

	bool hitWall = false;

	// 元の移動量を補正していく
	float resolvedMovementX = info.MovementAmount.x;

	for (const Vector3& position : checkPositions) {
		MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

		if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
			continue;
		}

		MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

		// プレイヤーの右端を壁の左端へ合わせる
		float candidateMovementX = (rect.left - worldTransform_.translation_.x) - (halfWidth + kMargin);

		resolvedMovementX = std::min(resolvedMovementX, candidateMovementX);

		hitWall = true;
	}

	if (hitWall) {
		info.MovementAmount.x = resolvedMovementX;
		info.isWallCollide = true;
	}
}

// 左方向
void Player::MapCollisionCheckLeft(CollisionMapInfo& info) {

	// 左へ移動していなければ判定しない
	if (info.MovementAmount.x >= 0.0f) {
		return;
	}

	const float halfWidth = kWidth / 2.0f;
	const float halfHeight = kHeight / 2.0f;

	// 移動後の中心座標
	Vector3 nextCenter = {
	    worldTransform_.translation_.x + info.MovementAmount.x,

	    worldTransform_.translation_.y + info.MovementAmount.y,

	    worldTransform_.translation_.z,
	};

	// 左端の上下2点
	Vector3 checkPositions[] = {
	    {
         nextCenter.x - halfWidth,
         nextCenter.y + halfHeight - kMargin,
         nextCenter.z,
	     },
	    {
         nextCenter.x - halfWidth,
         nextCenter.y - halfHeight + kMargin,
         nextCenter.z,
	     },
	};

	bool hitWall = false;

	float resolvedMovementX = info.MovementAmount.x;

	for (const Vector3& position : checkPositions) {
		MapChipField::IndexSet index = mapChipField_->GetMapChipIndexSetByPosition(position);

		if (mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) != MapChipType::kBlock) {
			continue;
		}

		MapChipField::Rect rect = mapChipField_->GetRectByIndex(index.xIndex, index.yIndex);

		// プレイヤーの左端を壁の右端へ合わせる
		float candidateMovementX = (rect.right - worldTransform_.translation_.x) + (halfWidth + kMargin);

		resolvedMovementX = std::max(resolvedMovementX, candidateMovementX);

		hitWall = true;
	}

	if (hitWall) {
		info.MovementAmount.x = resolvedMovementX;
		info.isWallCollide = true;
	}
}

// 判定結果を反映させて移動させる
void Player::MoveReflectingResult(const CollisionMapInfo& info) {
	// 移動
	worldTransform_.translation_.x += info.MovementAmount.x;
	worldTransform_.translation_.y += info.MovementAmount.y;
}

// 天井に接触している時の処理
void Player::ContactWithCeiling(const CollisionMapInfo& info) {
	// 天井に当たったか
	if (info.isCeilingCollide) {
		// DebugText::GetInstance()->ConsolePrintf("hit ceiking\n");
		velocity_.y = 0.0f;
	}
}

// 壁に接触している時の処理
void Player::ContactWithWall(const CollisionMapInfo& info) {
	// 壁に当たったか
	if (info.isWallCollide) {
		// DebugText::GetInstance()->ConsolePrintf("hit wall\n");
		velocity_.x = velocity_.x * (1.0f - kAttenuationWall);
	}
}

// 接地状態の切り替え
void Player::SwitchGroundingState(const CollisionMapInfo& info) {
	// 自キャラは接地状態か
	// 接地状態の処理
	if (onGround_) {
		// ジャンプ開始
		if (velocity_.y > 0.0f) {
			// 空中状態に切り替える
			onGround_ = false;

		} else {
			// 落下判定
			// 移動後の4つの角の座標
			std::array<Vector3, kNumCorner> positionsNew = {};

			// AL2_05_08スライド22の余白の追加をここに移動中
			for (uint32_t i = 0; i < positionsNew.size(); ++i) {
				positionsNew[i] = CornerPosition(
				    {worldTransform_.translation_.x + info.MovementAmount.x, worldTransform_.translation_.y + info.MovementAmount.y, worldTransform_.translation_.z + info.MovementAmount.z},
				    static_cast<Corner>(i));
			}

			MapChipType mapChipType;

			// 真下の当たり判定を取る
			bool hit = false;

			Vector3 pos;
			pos.x = positionsNew[kLeftBottom].x + 0.0f;
			pos.y = positionsNew[kLeftBottom].y - kMargin;
			pos.z = positionsNew[kLeftBottom].z + 0.0f;

			// 左下点の判定
			MapChipField::IndexSet indexSet = {};
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(pos);
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) {
				hit = true;
			}

			pos.x = positionsNew[kRightBottom].x + 0.0f;
			pos.y = positionsNew[kRightBottom].y - kMargin;
			pos.z = positionsNew[kRightBottom].z + 0.0f;

			// 右下点の判定
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(pos);
			mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (mapChipType == MapChipType::kBlock) {
				hit = true;
			}

			// 落下なら空中状態に切り替え
			if (!hit) {
				onGround_ = false;
			}
		}

	} else {
		// 空中状態の処理
		if (info.isLanding) {
			// 着地状態に切り替える
			onGround_ = true;

			canAirAttack_ = true;
			canDoubleJump_ = true;

			// Y方向速度を0にする
			velocity_.y = 0.0f;
		}
	}
}

// ワールド座標取得
Vector3 Player::GetWorldPos() {
	Vector3 worldPos;

	// ワールド行列の平行移動成分を取得
	worldPos.x = worldTransform_.translation_.x;
	worldPos.y = worldTransform_.translation_.y;
	worldPos.z = worldTransform_.translation_.z;

	return worldPos;
}

// 本体のAABBを取得
AABB Player::GetAABB() {
	Vector3 worldPos = GetWorldPos();

	AABB aabb;

	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

// 攻撃用のAABBを取得
AABB Player::GetAttackAABB() const {
	float direction = lrDirection_ == LRDirection::kRight ? 1.0f : -1.0f;

	Vector3 center = worldTransform_.translation_;

	// プレイヤーの前方へ配置
	center.x += direction * kAttackOffsetX;

	AABB attackAABB;
	attackAABB.min = {
	    center.x - kAttackWidth / 2.0f,
	    center.y - kAttackHeight / 2.0f,
	    center.z - kWidth / 2.0f,
	};

	attackAABB.max = {
	    center.x + kAttackWidth / 2.0f,
	    center.y + kAttackHeight / 2.0f,
	    center.z + kWidth / 2.0f,
	};

	return attackAABB;
}

// 敵との当たり判定
void Player::OnCollisionEnemy(Enemy* enemy) {
	// 攻撃判定が出ている間は接触ダメージを受けない
	if (IsAttack()) {
		return;
	}

	// HPを減らす
	hp_ -= kEnemyContactDamage;
	hp_ = std::max(hp_, 0);

	// HPが0なら死亡
	if (hp_ <= 0) {
		isDead_ = true;
		EndAttack();
		return;
	}

	// 敵との位置関係からノックバック方向を決める
	float differenceX = worldTransform_.translation_.x - enemy->GetWorldPos().x;

	float direction = differenceX >= 0.0f ? 1.0f : -1.0f;

	RequestKnockBack(direction);
}

// 盾敵との当たり判定
void Player::OnCollisionShieldEnemy(ShieldEnemy* shieldEnemy) {
	// 攻撃中なら敵に触れても死なない
	if (IsAttack()) {
		return;
	}
	// 敵と接触したら死亡
	isDead_ = true;

	(void)shieldEnemy;
}

// 攻撃中かどうかを判定する
bool Player::IsAttack() {
	// スイングモーション中を攻撃中と判断
	return behaivior_ == Behavior::kAttack && attackPhase_ == AttackPhase::kActive;
}

// 敵を攻撃できるか
bool Player::CanAttackEnemy() const {
	// スイングモーション中のみ攻撃可能
	return behaivior_ == Behavior::kAttack && attackPhase_ == AttackPhase::kActive;
}

// ダメージを受けるか
bool Player::CanReceiveDamage() const { return !isDead_ && behaivior_ != Behavior::kKnockBack; }

// ノックバック要求を受け取る
void Player::RequestKnockBack(float direction) {
	knockBackDirection_ = direction;
	behaiviorRequest_ = Behavior::kKnockBack;
}

// 攻撃を終了する
void Player::EndAttack() {
	isAttackEffect_ = false;

	worldTransform_.scale_ = {2.0f, 2.0f, 2.0f};

	worldTransformAttack_.rotation_.z = 0.0f;

	attackPhase_ = AttackPhase::kNone;
	attackType_ = AttackType::kNormal;

	attackTimer_ = 0.0f;
	chargeTimer_ = 0.0f;

	isChargeReady_ = false;
	hasHitEnemy_ = false;
}