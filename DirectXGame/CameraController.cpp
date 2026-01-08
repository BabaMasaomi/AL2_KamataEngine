#define NOMINMAX
#include "CameraController.h"
#include "Player.h"
#include <algorithm>

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

void CameraController::Intialize(Camera* camera) {
	// 引き数の内容をメンバ変数に記録
	camera_ = camera;
}

void CameraController::Update() {
	// 安全対策
	if (!target_) {
		return;
	}

	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	const Vector3& targetVelocity = target_->GetVeloctiy();

	// 追従対象とオフセットからカメラの座標を計算
	targetCoordinate.x = targetWorldTransform.translation_.x + targetOffset_.x + targetVelocity.x * kVelocityBias;
	targetCoordinate.y = targetWorldTransform.translation_.y + targetOffset_.y + targetVelocity.y * kVelocityBias;
	targetCoordinate.z = targetWorldTransform.translation_.z + targetOffset_.z + targetVelocity.z * kVelocityBias;

	// 座標補間でゆったり追従
	camera_->translation_.x = std::lerp(camera_->translation_.x, targetCoordinate.x, kInterpolationRate);
	camera_->translation_.y = std::lerp(camera_->translation_.y, targetCoordinate.y, kInterpolationRate);

	// 追従対象が画面外に出ない様に補正
	camera_->translation_.x = std::max(camera_->translation_.x, targetWorldTransform.translation_.x + cameraMovementMargin.left);
	camera_->translation_.x = std::min(camera_->translation_.x, targetWorldTransform.translation_.x + cameraMovementMargin.right);
	camera_->translation_.y = std::max(camera_->translation_.y, targetWorldTransform.translation_.y + cameraMovementMargin.bottom);
	camera_->translation_.y = std::min(camera_->translation_.y, targetWorldTransform.translation_.y + cameraMovementMargin.top);

	// 移動範囲制限
	camera_->translation_.x = std::max(camera_->translation_.x, movableArea_.left);
	camera_->translation_.x = std::min(camera_->translation_.x, movableArea_.right);
	camera_->translation_.y = std::max(camera_->translation_.y, movableArea_.bottom);
	camera_->translation_.y = std::min(camera_->translation_.y, movableArea_.top);

	// 行列を更新する
	camera_->UpdateMatrix();
}

void CameraController::Reset() {
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	// 追従対象とオフセットからカメラの座標を計算
	camera_->translation_.x = targetWorldTransform.translation_.x + targetOffset_.x;
	camera_->translation_.y = targetWorldTransform.translation_.y + targetOffset_.y;
	camera_->translation_.z = targetWorldTransform.translation_.z + targetOffset_.z;
}