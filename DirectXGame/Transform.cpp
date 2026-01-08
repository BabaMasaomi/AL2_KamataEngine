#include "Transform.h"

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/// <summary>
/// ワールド行列更新関数
/// </summary>
/// <param name="worldTrandform">編集したいワールドトランスフォーム</param>
void Transform::worldMatrixUpdate(KamataEngine::WorldTransform& worldTrandform) {
	// スケール、回転、平行移動を合成して行列を計算する
	worldTrandform.matWorld_ = MakeAffineMatrix(worldTrandform.scale_, worldTrandform.rotation_, worldTrandform.translation_);

	// 定数バッファへの書き込み
	worldTrandform.TransferMatrix();
}

/*-------------------- 機能の為に必要な関数 --------------------*/
// 拡大縮小行列
Matrix4x4 Transform::MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result{};

	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;
	result.m[3][3] = 1.0f;

	return result;
}

// 平行移動行列
Matrix4x4 Transform::MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result{};

	result.m[0][0] = 1.0f;
	result.m[1][1] = 1.0f;
	result.m[2][2] = 1.0f;
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;
	result.m[3][2] = translate.z;
	result.m[3][3] = 1.0f;

	return result;
}

// Z回転行列
Matrix4x4 Transform::MakeRotateZMatrix(float radian) {
	Matrix4x4 result{};

	// Z
	result.m[0][0] = cosf(radian);
	result.m[0][1] = sinf(radian);
	result.m[1][0] = -sinf(radian);
	result.m[1][1] = cosf(radian);
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;

	return result;
}

// X回転行列
Matrix4x4 Transform::MakeRotateXMatrix(float radian) {
	Matrix4x4 result{};

	// X
	result.m[0][0] = 1.0f;
	result.m[1][1] = cosf(radian);
	result.m[1][2] = sinf(radian);
	result.m[2][1] = -sinf(radian);
	result.m[2][2] = cosf(radian);
	result.m[3][3] = 1.0f;

	return result;
}

// Y回転行列
Matrix4x4 Transform::MakeRotateYMatrix(float radian) {
	Matrix4x4 result{};

	// y
	result.m[0][0] = cosf(radian);
	result.m[0][2] = -sinf(radian);
	result.m[1][1] = 1.0f;
	result.m[2][0] = sinf(radian);
	result.m[2][2] = cosf(radian);
	result.m[3][3] = 1.0f;

	return result;
}

// 4x4行列の掛け算
Matrix4x4 Transform::Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result{};

	for (int y = 0; y < 4; y++) {
		for (int x = 0; x < 4; x++) {
			result.m[y][x] = m1.m[y][0] * m2.m[0][x] + m1.m[y][1] * m2.m[1][x] + m1.m[y][2] * m2.m[2][x] + m1.m[y][3] * m2.m[3][x];
		}
	}
	return result;
}

/// <summary>
/// アフィン変換を合成する関数
/// </summary>
/// <param name="scale">引数1 描画対象のスケール</param>
/// <param name="rotate">引数2 描画対象の回転</param>
/// <param name="translate">引数3 描画対象の平行移動量</param>
/// <returns></returns>
Matrix4x4 Transform::MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	Matrix4x4 result{};

	// 各行列を作成
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);

	Matrix4x4 rotateX = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateY = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZ = MakeRotateZMatrix(rotate.z);

	// 回転行列を合成
	Matrix4x4 rotateMatrix = Multiply(rotateX, Multiply(rotateY, rotateZ));

	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);

	// アフィン変換行列を合成
	return result = Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);
}