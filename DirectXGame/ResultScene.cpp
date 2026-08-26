#include "ResultScene.h"

using namespace KamataEngine;

ResultScene::ResultScene() {}

ResultScene::~ResultScene() {
	delete backgroundSprite_;
	delete fade_;
}

void ResultScene::Initialize(GameResult result) {
	result_ = result;
	finished_ = false;
	phase_ = Phase::kFadeIn;

	uint32_t textureHandle = TextureManager::Load("white1x1.png");

	Vector4 backgroundColor;

	if (result_ == GameResult::kClear) {
		// クリア画面
		backgroundColor = {0.10f, 0.25f, 0.55f, 1.0f};

	} else {
		// ゲームオーバー画面
		backgroundColor = {0.45f, 0.08f, 0.08f, 1.0f};
	}

	backgroundSprite_ = Sprite::Create(textureHandle, {0.0f, 0.0f}, backgroundColor);

	backgroundSprite_->SetSize({1280.0f, 720.0f});

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 0.75f);
}

void ResultScene::Update() {
	switch (phase_) {
	case Phase::kFadeIn:
		if (fade_->IsFinished()) {
			fade_->Stop();
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		// 仮操作：Spaceでタイトルへ戻る
		if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {

			fade_->Start(Fade::Status::FadeOut, 0.75f);

			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		break;
	}

	fade_->Update();
}

void ResultScene::Draw() {
	Sprite::PreDraw();
	backgroundSprite_->Draw();
	Sprite::PostDraw();

	fade_->Draw();
}