#define NOMINMAX
#include "Player.h"
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
	// 速度を初期化
	velocity_.x = 0.0f;
	velocity_.y = 0.0f;
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

	/*========== ①移動入力 ==========*/
	// 接地している時
	if (onGround_) {
		// 移動入力
		if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {
			// 加速
			Vector3 acceleration = {};

			// 左右移動
			if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
				// 速度と逆方向の時は急ブレーキ
				if (velocity_.x < 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}

				// 右移動
				acceleration.x += kAcceleration;

				// 体を右に
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;
					// 旋回開始時の角度を記録
					turnFirstRotationY_ = worldTransform_.rotation_.y;

					// 旋回タイマーをリセット
					turnTimer_ = kTimeTurn;
				}

			} else if (Input::GetInstance()->PushKey(DIK_LEFT)) {
				// 速度と逆方向の時は急ブレーキ
				if (velocity_.x > 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}

				// 左移動
				acceleration.x -= kAcceleration;

				// 体を左に
				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;
					// 旋回開始時の角度を記録
					turnFirstRotationY_ = worldTransform_.rotation_.y;

					// 旋回タイマーをリセット
					turnTimer_ = kTimeTurn;
				}
			}

			// 加速/減速
			velocity_.x += acceleration.x;

			// 最大速度制限
			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);

		} else {
			// 入力していない時は減速
			velocity_.x *= (1.0f - kAttenuation);
		}

		// ジャンプ入力
		if (Input::GetInstance()->TriggerKey(DIK_UP)) {
			// ジャンプ初速
			velocity_.y += kJumpAcceleration_;
		}

	} else { // 空中
		// 落下速度
		velocity_.y -= kGravityAcceleration;

		// 落下速度制限
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
	// 1回の攻撃開始ごとに番号を更新
	++attackSerial_;

	attackTimer_ = 0.0f;
	attackPhase_ = AttackPhase::kStartup;
	hasHitEnemy_ = false;

	// 攻撃開始時はいったん横移動を止める
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

	switch (attackPhase_) {
	case AttackPhase::kStartup: {
		float t = std::clamp(attackTimer_ / kAttackStartupTime, 0.0f, 1.0f);

		// 少し後ろへ振りかぶる
		float angleDegree = EaseOut(0.0f, kBatAngleStart, t);

		worldTransformAttack_.rotation_.z = angleDegree * std::numbers::pi_v<float> / 180.0f * direction;

		if (attackTimer_ >= kAttackStartupTime) {
			attackTimer_ = 0.0f;
			attackPhase_ = AttackPhase::kActive;
		}
		break;
	}

	case AttackPhase::kActive: {
		float t = std::clamp(attackTimer_ / kAttackActiveTime, 0.0f, 1.0f);

		// バットを前方へ振る
		float angleDegree = EaseOut(kBatAngleStart, kBatAngleEnd, t);

		worldTransformAttack_.rotation_.z = angleDegree * std::numbers::pi_v<float> / 180.0f * direction;

		// 突進ではなく、短い踏み込み
		velocity_.x = direction * kAttackStepSpeed * (1.0f - t);

		if (attackTimer_ >= kAttackActiveTime) {
			velocity_.x = 0.0f;
			attackTimer_ = 0.0f;
			attackPhase_ = AttackPhase::kRecovery;
		}
		break;
	}

	case AttackPhase::kRecovery: {
		float t = std::clamp(attackTimer_ / kAttackRecoveryTime, 0.0f, 1.0f);

		// 振り終わったバットを元へ戻す
		float angleDegree = EaseOut(kBatAngleEnd, 0.0f, t);

		worldTransformAttack_.rotation_.z = angleDegree * std::numbers::pi_v<float> / 180.0f * direction;

		velocity_.x = 0.0f;

		if (attackTimer_ >= kAttackRecoveryTime) {
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
	// 右移動しているか
	if (info.MovementAmount.x <= 0.0f) {
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

	// 右側の当たり判定を取る
	bool hit = false;

	// 右上点の判定
	MapChipField::IndexSet indexSet = {};
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// 右下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// ブロックに当たっているか
	if (hit) {
		// めり込みを解消する方向に移動量を設定(GetMapChipIndexSetByPositionには自キャラの右端座標を入れる)
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);

		// 現在座標が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition({worldTransform_.translation_.x + kWidth / 2.0f, worldTransform_.translation_.y, worldTransform_.translation_.z});

		if (indexSetNow.xIndex != indexSet.xIndex) {
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.MovementAmount.x = std::min(0.0f, (rect.left - worldTransform_.translation_.x) - (kWidth / 2.0f + kMargin));
			// 壁に当たった
			info.isWallCollide = true;
		}
	}
}

// 左方向
void Player::MapCollisionCheckLeft(CollisionMapInfo& info) {
	// 左移動しているか

	if (info.MovementAmount.x >= 0.0f) {
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

	// 左側の当たり判定を取る
	bool hit = false;

	// 左上点の判定
	MapChipField::IndexSet indexSet = {};
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// 左下点の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		hit = true;
	}

	// ブロックに当たっているか
	if (hit) {
		// めり込みを解消する方向に移動量を設定(GetMapChipIndexSetByPositionには自キャラの左端座標を入れる)
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);

		// 現在座標が壁の外か判定
		MapChipField::IndexSet indexSetNow;
		indexSetNow = mapChipField_->GetMapChipIndexSetByPosition({worldTransform_.translation_.x - kWidth / 2.0f, worldTransform_.translation_.y, worldTransform_.translation_.z});

		if (indexSetNow.xIndex != indexSet.xIndex) {
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
			info.MovementAmount.x = std::max(0.0f, (rect.right - worldTransform_.translation_.x) + (kWidth / 2.0f + kMargin));
			// 壁に当たった
			info.isWallCollide = true;
		}
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

			// 着地時にX方向速度を減衰させる(無くていいよ)
			// velocity_.x *= (1.0f - kAttenuationLanding);

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
	// 攻撃中なら敵に触れても死なない
	if (IsAttack()) {
		return;
	}
	// 敵と接触したら死亡
	isDead_ = true;

	(void)enemy;
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
bool Player::CanReceiveDamage() const {
	// 食らいモーション中じゃない
	return behaivior_ != Behavior::kKnockBack;
}

// ノックバック要求を受け取る
void Player::RequestKnockBack(float direction) {
	knockBackDirection_ = direction;
	behaiviorRequest_ = Behavior::kKnockBack;
}

// 攻撃を終了する
void Player::EndAttack() {
	// エフェクトを消す
	isAttackEffect_ = false;
	// スケールを元に戻す
	worldTransform_.scale_ = {2.0f, 2.0f, 2.0f};
	// 攻撃フェーズをリセット
	attackPhase_ = AttackPhase::kNone;
	// タイマーをリセット
	attackTimer_ = 0.0f;
	// 命中したかのフラグを取り消す
	hasHitEnemy_ = false;
}