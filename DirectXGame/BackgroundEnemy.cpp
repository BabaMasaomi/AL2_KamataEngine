#include "BackgroundEnemy.h"
#include <cassert>
#include <cmath>
#include <random>

using namespace KamataEngine;

float BackgroundEnemy::RandomFloat(float minValue, float maxValue) {
	static std::random_device seedGenerator;
	static std::mt19937 randomEngine(seedGenerator());

	std::uniform_real_distribution<float> distribution(minValue, maxValue);
	return distribution(randomEngine);
}

void BackgroundEnemy::Initialize(Model* model, Camera* camera, const Vector3& position) {

	assert(model);
	assert(camera);

	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();

	float depthRate = (position.z - kMinDepth) / (kMaxDepth - kMinDepth);

	float alpha = 0.50f + (0.20f - 0.50f) * depthRate;

	objectColor_.Initialize();
	objectColor_.SetColor({
	    1.0f,
	    1.0f,
	    1.0f,
	    alpha,
	});

	// 背景敵の色を初期化
	objectColor_.Initialize();

	// RGBはそのまま、アルファだけ下げる
	objectColor_.SetColor({
	    1.0f,
	    1.0f,
	    1.0f,
	    kAlpha,
	});

	worldTransform_.translation_ = position;

	// 通常敵より少し小さめ
	float scale = RandomFloat(0.7f, 1.5f);
	worldTransform_.scale_ = {scale, scale, scale};

	worldTransform_.rotation_ = {
	    RandomFloat(0.0f, 6.28f),
	    RandomFloat(0.0f, 6.28f),
	    RandomFloat(0.0f, 6.28f),
	};

	ResetMovement();
	transform_.worldMatrixUpdate(worldTransform_);
}

void BackgroundEnemy::ResetMovement() {
	// ランダムな2次元方向
	float directionX = RandomFloat(-1.0f, 1.0f);
	float directionY = RandomFloat(-1.0f, 1.0f);

	float length = std::sqrt(directionX * directionX + directionY * directionY);

	if (length < 0.001f) {
		directionX = 1.0f;
		directionY = 0.0f;
		length = 1.0f;
	}

	directionX /= length;
	directionY /= length;

	float speed = RandomFloat(kMinMoveSpeed, kMaxMoveSpeed);

	velocity_.x = directionX * speed;
	velocity_.y = directionY * speed;

	// Z方向は大きく動かさない
	velocity_.z = RandomFloat(-0.003f, 0.003f);

	angularVelocity_ = {
	    RandomFloat(-kMaxRotateSpeed, kMaxRotateSpeed),
	    RandomFloat(-kMaxRotateSpeed, kMaxRotateSpeed),
	    RandomFloat(-kMaxRotateSpeed, kMaxRotateSpeed),
	};

	// 回転がすべてほぼ停止しないようにする
	if (std::abs(angularVelocity_.x) < kMinRotateSpeed) {
		angularVelocity_.x = angularVelocity_.x < 0.0f ? -kMinRotateSpeed : kMinRotateSpeed;
	}

	movementTimer_ = 0.0f;
	movementChangeTime_ = RandomFloat(2.0f, 5.0f);
}

void BackgroundEnemy::Update() {
	const float deltaTime = 1.0f / 60.0f;

	movementTimer_ += deltaTime;

	// 数秒ごとに少し違う動きへ変更
	if (movementTimer_ >= movementChangeTime_) {
		ResetMovement();
	}

	worldTransform_.translation_.x += velocity_.x;
	worldTransform_.translation_.y += velocity_.y;
	worldTransform_.translation_.z += velocity_.z;

	worldTransform_.rotation_.x += angularVelocity_.x;
	worldTransform_.rotation_.y += angularVelocity_.y;
	worldTransform_.rotation_.z += angularVelocity_.z;

	// 奥行き範囲から出ないように反射
	if (worldTransform_.translation_.z < kMinDepth) {
		worldTransform_.translation_.z = kMinDepth;
		velocity_.z = std::abs(velocity_.z);
	}

	if (worldTransform_.translation_.z > kMaxDepth) {
		worldTransform_.translation_.z = kMaxDepth;
		velocity_.z = -std::abs(velocity_.z);
	}

	WrapAroundScreen();
	transform_.worldMatrixUpdate(worldTransform_);
}

void BackgroundEnemy::WrapAroundScreen() {
	if (!camera_) {
		return;
	}

	float left = camera_->translation_.x - kAreaHalfWidth;
	float right = camera_->translation_.x + kAreaHalfWidth;
	float bottom = camera_->translation_.y - kAreaHalfHeight;
	float top = camera_->translation_.y + kAreaHalfHeight;

	if (worldTransform_.translation_.x < left) {
		worldTransform_.translation_.x = right;
		worldTransform_.translation_.y = RandomFloat(bottom, top);
	} else if (worldTransform_.translation_.x > right) {
		worldTransform_.translation_.x = left;
		worldTransform_.translation_.y = RandomFloat(bottom, top);
	}

	if (worldTransform_.translation_.y < bottom) {
		worldTransform_.translation_.y = top;
		worldTransform_.translation_.x = RandomFloat(left, right);
	} else if (worldTransform_.translation_.y > top) {
		worldTransform_.translation_.y = bottom;
		worldTransform_.translation_.x = RandomFloat(left, right);
	}
}

void BackgroundEnemy::Draw() {
	if (!model_ || !camera_) {
		return;
	}

	model_->Draw(worldTransform_, *camera_, &objectColor_);
}