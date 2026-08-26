#define NOMINMAX
#include "CameraController.h"
#include "Player.h"
#include <algorithm>

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

void CameraController::Initialize(Camera* camera) {
	// 引き数の内容をメンバ変数に記録
	camera_ = camera;
}

void CameraController::Update() {
	// 安全対策
	if (!target_) {
		return;
	}

	switch (mode_) {

	case Mode::kFollow:
		UpdateFollow();
		break;

	case Mode::kForcedScroll:
		UpdateForcedScroll();
		break;
	}

	// 移動範囲制限
	camera_->translation_.x = std::max(camera_->translation_.x, movableArea_.left);
	camera_->translation_.x = std::min(camera_->translation_.x, movableArea_.right);
	camera_->translation_.y = std::max(camera_->translation_.y, movableArea_.bottom);
	camera_->translation_.y = std::min(camera_->translation_.y, movableArea_.top);

	// 行列を更新する
	camera_->UpdateMatrix();
}

// 追従スクロールの更新
void CameraController::UpdateFollow() {
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	const Vector3& targetVelocity = target_->GetVeloctiy();

	// 追従対象とオフセットからカメラの座標を計算
	targetCoordinate.x = targetWorldTransform.translation_.x + targetOffset_.x + targetVelocity.x * kVelocityBiasX;
	targetCoordinate.y = targetWorldTransform.translation_.y + targetOffset_.y + targetVelocity.y * kVelocityBiasY;
	targetCoordinate.z = targetWorldTransform.translation_.z + targetOffset_.z;

	// 座標補間でゆったり追従
	camera_->translation_.x = std::lerp(camera_->translation_.x, targetCoordinate.x, kInterpolationRate);
	camera_->translation_.y = std::lerp(camera_->translation_.y, targetCoordinate.y, kInterpolationRate);

	// 追従対象が画面外に出ない様に補正
	camera_->translation_.x = std::max(camera_->translation_.x, targetWorldTransform.translation_.x + cameraMovementMargin.left);
	camera_->translation_.x = std::min(camera_->translation_.x, targetWorldTransform.translation_.x + cameraMovementMargin.right);
	camera_->translation_.y = std::max(camera_->translation_.y, targetWorldTransform.translation_.y + cameraMovementMargin.bottom);
	camera_->translation_.y = std::min(camera_->translation_.y, targetWorldTransform.translation_.y + cameraMovementMargin.top);
}

// 強制スクロールの更新
void CameraController::UpdateForcedScroll() {
	camera_->translation_.x += forceScrollSpeed_;
}

void CameraController::Reset() {
	if (!target_ || !camera_) {
		return;
	}

	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();

	camera_->translation_.x = targetWorldTransform.translation_.x + targetOffset_.x;
	camera_->translation_.y = targetWorldTransform.translation_.y + targetOffset_.y;
	camera_->translation_.z = targetWorldTransform.translation_.z + targetOffset_.z;

	// 初期位置にもマップ範囲を適用
	camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);
	camera_->UpdateMatrix();
}