#include "Fade.h"
#include <algorithm>

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/*-------------------- コンストラクタ&デストラクタ --------------------*/
Fade::Fade() {}
Fade::~Fade() { delete sprite_; }

/*==============================================================
* メンバ関数
==============================================================*/
/*-------------------- 初期化 --------------------*/
void Fade::Initialize() {
	// スプライトの生成
	sprite_ = Sprite::Create(0, Vector2(0, 0), Vector4(0, 0, 0, 1), Vector2(0.0f, 0.0f), false, false);
	sprite_->SetSize(Vector2(1280, 720));
}

/*-------------------- 更新 --------------------*/
void Fade::Update() {
	switch (status_) {
	case Status::None:
		// 何もしない
		break;

	case Status::FadeIn:
		// 1フレーム分の秒数をカウント
		counter_ += 1.0f / 60.0f;
		// フェード継続時間に達したら打ち止め
		if (counter_ >= duration_) {
			counter_ = duration_;
		}
		// 0.0fから1.0fの間で経過時間がフェード継続時間に近づくほどアルファ値を小さくする
		sprite_->SetColor(Vector4(0, 0, 0, 1.0f - std::clamp(counter_ / duration_, 0.0f, 1.0f)));
		break;

	case Status::FadeOut:
		// 1フレーム分の秒数をカウント
		counter_ += 1.0f / 60.0f;
		// フェード継続時間に達したら打ち止め
		if (counter_ >= duration_) {
			counter_ = duration_;
		}
		// 0.0fから1.0fの間で経過時間がフェード継続時間に近づくほどアルファ値を大きくする
		sprite_->SetColor(Vector4(0, 0, 0, std::clamp(counter_ / duration_, 0.0f, 1.0f)));
		break;

	default:
		break;
	}
}

/*-------------------- 描画 --------------------*/
void Fade::Draw() {
	if (status_ == Status::None) {
		return;
	}
	Sprite::PreDraw();
	// スプライトを描画
	sprite_->Draw();

	Sprite::PostDraw();
}

/*-------------------- フェード開始 --------------------*/
void Fade::Start(Status status, float duration) {
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;
}

/*-------------------- フェード停止 --------------------*/
void Fade::Stop() { status_ = Status::None; }

/*-------------------- フェードが終了したか判定を取る --------------------*/
bool Fade::IsFinished() const {
	switch (status_) {
	case Fade::Status::None:
		break;

	case Fade::Status::FadeIn :
		if (counter_ >= duration_) {
			return true;
		}
		else {
			return false;
		}
		break;

	case Fade::Status::FadeOut:
		if (counter_ >= duration_) {
			return true;
		} else {
			return false;
		}
		break;

	default:
		break;
	}

	return true;
}