#include "DeathParticles.h"
#include <algorithm>
#include <cassert>

// を毎回入力しなくてもいい様にする
using namespace KamataEngine;

void DeathParticles::Initialize(Model* model, Camera* camera, const Vector3 pos) {
	// ぬるぽチェック
	assert(model);

	// 引き数の内容をメンバ変数に記録
	camera_ = camera;

	// ワールド変換の初期化
	for (WorldTransform& worldTransform : worldTransforms_) {
		worldTransform.Initialize();
		worldTransform.scale_ = {2, 2, 2};
		worldTransform.translation_ = pos;
	}

	// 色操作用の変数
	objectColor_.Initialize();
	color_ = {1.0f, 1.0f, 1.0f, 1.0f};

	// 3Dモデルの生成
	model_ = model;
}

void DeathParticles::Update() {
	// 終了
	if (isFinished_) {
		return;
	}

	//  経過時間を加算
	counter_ += 1.0f / 60.0f;

	// 存続時間を越えたら終了
	if (counter_ >= kParticleLifeTime) {
		isFinished_ = true;
		return;
	}

	// 色の変更 (徐々に透明にする)
	color_.w = std::clamp(1.0f - (counter_ / kParticleLifeTime), 0.0f, 1.0f);
	objectColor_.SetColor(color_);

	// パーティクルの移動
	for (uint32_t i = 0; i < kNumParticles; ++i) {
		// 基本となる速度ベクトル
		Vector3 velocity = {kParticleMoveSpeed, 0.0f, 0.0f};

		// 回転角を計算する
		float angle = kParticleAngleDivide * static_cast<float>(i);

		// Z回転行列を作成する
		Matrix4x4 rotZMatrix = transform_.MakeRotateZMatrix(angle);

		// 基本ベクトルを回転させて速度ベクトルを求める (要修正)
		velocity = transform_.TransformNormal(velocity, rotZMatrix);

		// 移動処理
		worldTransforms_[i].translation_.x += velocity.x;
		worldTransforms_[i].translation_.y += velocity.y;
		worldTransforms_[i].translation_.z += velocity.z;
	}

	// ワールド変換の更新(範囲for文で行うゾ)
	for (WorldTransform& worldTransform : worldTransforms_) {
		// 行列を定数バッファに転送
		transform_.worldMatrixUpdate(worldTransform);
	}
}

void DeathParticles::Draw() {
	// 終了
	if (isFinished_) {
		return;
	}

	// パーティクルを描画
	for (WorldTransform& worldTransform : worldTransforms_) {
		model_->Draw(worldTransform, *camera_, &objectColor_);
	}
}