#pragma once
#include "KamataEngine.h"

class Transform {
public:
	/// <summary>
	/// ワールド行列更新関数
	/// </summary>
	/// <param name="worldTrandform">編集したいワールドトランスフォーム</param>
	void worldMatrixUpdate(KamataEngine::WorldTransform& worldTrandform);

	/*-------------------- 機能の為に必要な関数 --------------------*/
	// 拡大縮小行列
	KamataEngine::Matrix4x4 MakeScaleMatrix(const KamataEngine::Vector3& scale);

	// 平行移動行列
	KamataEngine::Matrix4x4 MakeTranslateMatrix(const KamataEngine::Vector3& translate);

	// Z回転行列
	KamataEngine::Matrix4x4 MakeRotateZMatrix(float radian);

	// X回転行列
	KamataEngine::Matrix4x4 MakeRotateXMatrix(float radian);

	// Y回転行列
	KamataEngine::Matrix4x4 MakeRotateYMatrix(float radian);

	// 4x4行列の掛け算
	KamataEngine::Matrix4x4 Multiply(const KamataEngine::Matrix4x4& m1, const KamataEngine::Matrix4x4& m2);

	// アフィン変換を合成する関数
	KamataEngine::Matrix4x4 MakeAffineMatrix(const KamataEngine::Vector3& scale, const KamataEngine::Vector3& rotate, const KamataEngine::Vector3& translate);
};