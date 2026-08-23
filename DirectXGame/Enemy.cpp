#define NOMINMAX
#include "Enemy.h"
#include "GameScene.h"
#include "MapChipField.h"
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
void Enemy::Initialize(Model* model, Camera* camera, const Vector3 pos) {
	// ぬるぽチェック
	assert(model);

	// 引き数の内容をメンバ変数に記録
	camera_ = camera;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// メンバ変数への代入処理
	// 敵の拡縮,回転,平行移動情報
	worldTransform_.scale_ = {2, 2, 2};
	worldTransform_.translation_ = pos;
	worldTransform_.rotation_.y = -std::numbers::pi_v<float> / 2.0f;

	// 3Dモデルの生成
	modelEnemy_ = model;

	// 移動速度の初期化
	velocity_ = {-kMoveSpeed, 0.0f, 0.0f};

	// アニメーションタイマーの初期化
	walkTimer_ = 0.0f;
}

/// <summary>
/// 敵の更新
/// </summary>
void Enemy::Update() {
	// Behavior変更
	if (behaviorRequest_ != BehaviorEnemy::kUnknown) {

		behavior_ = behaviorRequest_;

		switch (behavior_) {
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

	// 行列を定数バッファに転送
	transform_.worldMatrixUpdate(worldTransform_);
}

// 通常行動初期化
void Enemy::BehaviorRootInitialize() {
	isCollisionDisenabled_ = false;

	worldTransform_.rotation_.x = 0.0f;
	worldTransform_.rotation_.z = 0.0f;

	velocity_ = {-kMoveSpeed, 0.0f, 0.0f};
}

// 通常行動更新
void Enemy::BehaviorRootUpdate() {
	// ノックバック中は通常移動しない
	if (!isHitKnockBack_) {
		//worldTransform_.translation_.x += velocity_.x;
	}

	// 歩行アニメーション
	walkTimer_ += 1.0f / 60.0f;

	float param = std::sin((walkTimer_ / kWalkMotionTime) * 2.0f * std::numbers::pi_v<float>);

	float degree = kWalkMotionAngleStart + kWalkMotionAngleEnd * ((param + 1.0f) / 2.0f);

	worldTransform_.rotation_.x = degree * std::numbers::pi_v<float> / 180.0f;
}

// 死亡アクション初期化
void Enemy::BehaviorDeathInitialize() {
	deathTimer_ = 0.0f;
	velocity_ = {};
	isCollisionDisenabled_ = true;
}

// 死亡アクション更新
void Enemy::BehaviorDeathUpdate() {
	deathTimer_ += 1.0f / 60.0f;

	float t = std::clamp(deathTimer_ / kDeathTime, 0.0f, 1.0f);

	// 敵を回転させる
	worldTransform_.rotation_.x = 3.f;
	worldTransform_.rotation_.y += 1.0f;

	// 敵のサイズを縮小
	worldTransform_.scale_.x = EaseOut(2.0f, 0.0f, t);
	worldTransform_.scale_.y = EaseOut(2.0f, 0.0f, t);
	worldTransform_.scale_.z = EaseOut(2.0f, 0.0f, t);

	// 死亡を確定
	if (deathTimer_ >= kDeathTime) {
		isDead_ = true;
	}
}

// スタン初期化
void Enemy::BehaviorStunnedInitialize() {
	stunnedTimer_ = 0.0f;
	stunnedMotionTimer_ = 0.0f;

	velocity_ = {};

	// 攻撃を受けるAABBは残す
	isCollisionDisenabled_ = false;

	// 通常移動は停止
	velocity_ = {};

	isCollisionDisenabled_ = false;

	// 上を向く
	worldTransform_.rotation_.x = kStunnedLookUpAngle * std::numbers::pi_v<float> / 180.0f;

	// 震えを中央から開始
	worldTransform_.rotation_.z = 0.0f;
}

// スタン更新
void Enemy::BehaviorStunnedUpdate() {
	const float deltaTime = 1.0f / 60.0f;

	stunnedTimer_ += deltaTime;
	stunnedMotionTimer_ += deltaTime;

	// 上向き角度を維持
	worldTransform_.rotation_.x = kStunnedLookUpAngle * std::numbers::pi_v<float> / 180.0f;

	// 上を向いたまま左右へ小刻みに震える
	float shakeDegree = std::sin(stunnedMotionTimer_ * kStunnedShakeSpeed) * kStunnedShakeAngle;

	worldTransform_.rotation_.z = shakeDegree * std::numbers::pi_v<float> / 180.0f;

	if (stunnedTimer_ >= kStunnedTime) {
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
	float initialAngleDegree = RandomFloat(kInitialBlownAwayAngleMin, kInitialBlownAwayAngleMax);

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
}

// 吹っ飛び更新
void Enemy::BehaviorBlownAwayUpdate() {
	const float deltaTime = 1.0f / 60.0f;

	blownAwayTimer_ += deltaTime;

	// 新しい飛行区間に入ったので、
	// 再び敵1体へ命中可能
	canHitEnemyInCurrentBounce_ = true;

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

			bouncedThisFrame = true;
		}
	}

	// めり込みを解消したY座標を反映
	worldTransform_.translation_.y = nextY;

	/*========== 反射回数 ==========*/
	// 角へ衝突してXとYが同時に反射しても、
	// 1フレームにつき1回だけ加算
	if (bouncedThisFrame) {
		++bounceCount_;

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
	float speed = kHitKnockBackSpeed * (1.0f - t);

	worldTransform_.translation_.x += hitKnockBackDirection_ * speed;

	if (t >= 1.0f) {
		isHitKnockBack_ = false;
		hitKnockBackTimer_ = 0.0f;
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

	// 通常攻撃は通常状態だけ
	if (attackType == AttackType::kNormal && behavior_ != BehaviorEnemy::kRoot) {
		return false;
	}

	// 溜め攻撃は通常・スタン状態で受け付ける
	if (attackType == AttackType::kCharged && behavior_ != BehaviorEnemy::kRoot && behavior_ != BehaviorEnemy::kStunned) {
		return false;
	}

	uint32_t attackSerial = player->GetAttackSerial();

	if (lastReceivedAttackSerial_ == attackSerial) {
		return false;
	}

	lastReceivedAttackSerial_ = attackSerial;

	float differenceX = worldTransform_.translation_.x - player->GetWorldTransform().translation_.x;

	float hitDirection = differenceX >= 0.0f ? 1.0f : -1.0f;

	// スタン中への溜め攻撃
	if (behavior_ == BehaviorEnemy::kStunned && attackType == AttackType::kCharged) {

		blownAwayDirection_ = hitDirection;

		isHitKnockBack_ = false;
		hitKnockBackTimer_ = 0.0f;

		behaviorRequest_ = BehaviorEnemy::kBlownAway;

		Vector3 effectPos = {
		    (worldTransform_.translation_.x + player->GetWorldTransform().translation_.x) / 2.0f,

		    (worldTransform_.translation_.y + player->GetWorldTransform().translation_.y) / 2.0f,

		    0.0f,
		};

		if (gameScene_) {
			gameScene_->CreateHitEffect(effectPos, HitEffectType::kHit);
		}

		return true;
	}

	// 通常状態への攻撃は小ノックバック
	hitKnockBackDirection_ = hitDirection;
	isHitKnockBack_ = true;
	hitKnockBackTimer_ = 0.0f;

	// 通常攻撃だけスタンを蓄積
	if (attackType == AttackType::kNormal && behavior_ == BehaviorEnemy::kRoot) {

		++stunHitCount_;

		if (stunHitCount_ >= kStunHitCount) {
			behaviorRequest_ = BehaviorEnemy::kStunned;
		}
	}

	Vector3 effectPos = {
	    (worldTransform_.translation_.x + player->GetWorldTransform().translation_.x) / 2.0f,

	    (worldTransform_.translation_.y + player->GetWorldTransform().translation_.y) / 2.0f,

	    0.0f,
	};

	if (gameScene_) {
		gameScene_->CreateHitEffect(effectPos, HitEffectType::kHit);
	}

	return true;
}

// 当たり判定が無効化されているか
bool Enemy::IsCollisionDisEnabled() const { return isCollisionDisenabled_; }

bool Enemy::CanDamagePlayer() const {
	// 通常行動中かつ、
	// 攻撃による小ノックバック中でなければ危険
	return behavior_ == BehaviorEnemy::kRoot && !isHitKnockBack_;
}

// 吹き飛び中で、他の敵へ攻撃できるか
bool Enemy::CanHitOtherEnemy() const { return behavior_ == BehaviorEnemy::kBlownAway && !isBlownAwayStopped_ && canHitEnemyInCurrentBounce_; }

// 吹き飛び敵の攻撃を受けられるか
bool Enemy::CanReceiveBlownAwayHit() const { return behavior_ == BehaviorEnemy::kRoot && !isHitKnockBack_ && !isDead_; }

// 現在の飛行区間の攻撃権を消費
void Enemy::ConsumeBlownAwayHit() { canHitEnemyInCurrentBounce_ = false; }

// 吹き飛んできた敵との衝突処理
void Enemy::OnCollisionBlownAwayEnemy(float attackDirection) {

	// 行動可能な状態でなければ受け付けない
	if (!CanReceiveBlownAwayHit()) {
		return;
	}

	// 吹き飛んできた方向へ押し出す
	hitKnockBackDirection_ = attackDirection;

	isHitKnockBack_ = true;
	hitKnockBackTimer_ = 0.0f;

	// 通常攻撃より大きいスタンダメージ
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

void Enemy::SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }