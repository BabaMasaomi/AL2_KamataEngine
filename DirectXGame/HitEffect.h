#pragma once
#include "KamataEngine.h"
#include "Transform.h"

class HitEffect {
public:
	void Initialise(KamataEngine::Vector3 pos);
	void UpDate();
	void Draw();

	// インスタンスの生成と初期化
	static HitEffect* Create(KamataEngine::Vector3 pos);

	// セッター
	static void SetModel(KamataEngine::Model* model) { model_ = model; }
	static void SetCamera(KamataEngine::Camera* camera) { camera_ = camera; }

private:
	// Translateクラス内の関数を使える様にする
	Transform transform_;

	// モデル(借りてくる用)
	static KamataEngine::Model* model_;
	// カメラ(借りてくる用
	static KamataEngine::Camera* camera_;

	// 円のワールドトランスフォーム
	KamataEngine::WorldTransform circleWorldTransform_;
	// 楕円のワールドトランスフォーム
	std::array<KamataEngine::WorldTransform, 2> ellipseWorldTransform_;
};