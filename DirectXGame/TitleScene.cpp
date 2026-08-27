#include "TitleScene.h"
#include <numbers>

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

/*-------------------- コンストラクタ&デストラクタ --------------------*/
TitleScene::TitleScene() {}
TitleScene::~TitleScene() {
	delete modelTitle_;  // タイトルフォントの3Dモデルの解放
	delete modelPlayer_; // プレイヤーの3Dモデルの解放
	for (Sprite*& sprite : menuSprites_) {
		delete sprite;
		sprite = nullptr;
	}
	delete creditBackdropSprite_;
	creditBackdropSprite_ = nullptr;
	delete creditPlaceholderSprite_;
	creditPlaceholderSprite_ = nullptr;
	delete fade_;        // フェードの解放
}

/*-------------------- 初期化 --------------------*/
void TitleScene::Initialize() {
	finished_ = false;
	action_ = Action::kNone;
	selectedMenuIndex_ = 0;
	isCreditVisible_ = false;
	phase_ = Phase::kFadeIn;

	// カメラ初期化
	camera_.farZ = 550.0f;
	camera_.Initialize();

	// タイトル用フォントのモデル
	modelTitle_ = Model::CreateFromOBJ("titleFont", true); // タイトルフォントの3Dモデルを生成
	worldTransformTitle_.Initialize();                     // タイトルのワールドトランスフォームの初期化
	worldTransformTitle_.translation_ = {0.0f, 5.0f, 0.0f};
	worldTransformTitle_.scale_ = {2.0f, 2.0f, 2.0f};

	// プレイヤーのモデル
	modelPlayer_ = Model::CreateFromOBJ("player", true); // プレイヤーの3Dモデルを生成
	worldTransformPlayer_.Initialize();                  // プレイヤーのワールドトランスフォームの初期化
	worldTransformPlayer_.translation_ = {0.0f, -10.0f, 0.0f};
	worldTransformPlayer_.scale_ = {6.0f, 6.0f, 6.0f};

	InitializeMenu();

	// フェード用
	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, 2.0f);
}

/*-------------------- 更新 --------------------*/
void TitleScene::Update() {
	switch (phase_) {
	case TitleScene::Phase::kFadeIn:
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case TitleScene::Phase::kMain:
		UpdateMenu();
		break;

	case TitleScene::Phase::kFadeOut:
		// フェードアウトが終わったらゲームシーンに移行
		if (fade_->IsFinished()) {
			finished_ = true;
		}
		worldTransformPlayer_.rotation_.y += 1.0f;
		break;

	default:
		break;
	}

	// タイトルを揺らす
	static float t = 0.0f;
	t += 0.03f;
	worldTransformTitle_.translation_.y = 5.0f + sinf(t) * 0.5f;

	// プレイヤーを回す
	worldTransformPlayer_.rotation_.y += 0.02f;

	// 更新を反映
	transform_.worldMatrixUpdate(worldTransformTitle_);
	transform_.worldMatrixUpdate(worldTransformPlayer_);
	camera_.UpdateMatrix();

	// フェードを更新
	fade_->Update();
}

/*-------------------- 描画 --------------------*/
void TitleScene::Draw() {
	Model::PreDraw();

	// モデルを描画
	modelTitle_->Draw(worldTransformTitle_, camera_);
	modelPlayer_->Draw(worldTransformPlayer_, camera_);

	Model::PostDraw();

	Sprite::PreDraw();
	DrawMenu();
	Sprite::PostDraw();

	// UIより手前へフェードを描画
	fade_->Draw();
}

void TitleScene::InitializeMenu() {
	const std::array<const char*, 3> textureNames = {
	    "PlayGameUI_bat.png",
	    "ShowCreditUI_bat.png",
	    "QuitGameUI_bat.png",
	};

	for (size_t i = 0; i < menuSprites_.size(); ++i) {
		uint32_t texture = TextureManager::Load(textureNames[i]);
		menuSprites_[i] = Sprite::Create(texture, {kMenuCenterX, kMenuStartY + static_cast<float>(i) * kMenuSpacingY});
		menuSprites_[i]->SetAnchorPoint({0.5f, 0.5f});
		menuSprites_[i]->SetSize({kMenuSize, kMenuSize});
	}

	uint32_t whiteTexture = TextureManager::Load("white1x1.png");
	creditBackdropSprite_ = Sprite::Create(whiteTexture, {0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 0.85f});
	creditBackdropSprite_->SetSize({1280.0f, 720.0f});

	uint32_t creditTexture = TextureManager::Load("ShowCreditUI_bat.png");
	creditPlaceholderSprite_ = Sprite::Create(creditTexture, {640.0f, 330.0f});
	creditPlaceholderSprite_->SetAnchorPoint({0.5f, 0.5f});
	creditPlaceholderSprite_->SetSize({220.0f, 220.0f});

	UpdateMenuAppearance();
}

void TitleScene::UpdateMenuAppearance() {
	for (size_t i = 0; i < menuSprites_.size(); ++i) {
		if (!menuSprites_[i]) {
			continue;
		}
		float alpha = (i == selectedMenuIndex_) ? kSelectedAlpha : kUnselectedAlpha;
		menuSprites_[i]->SetColor({1.0f, 1.0f, 1.0f, alpha});
	}
}

void TitleScene::UpdateMenu() {
	Input* input = Input::GetInstance();

	if (isCreditVisible_) {
		if (input->TriggerKey(DIK_ESCAPE) || input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN)) {
			isCreditVisible_ = false;
		}
		return;
	}

	if (input->TriggerKey(DIK_UP)) {
		selectedMenuIndex_ = (selectedMenuIndex_ + menuSprites_.size() - 1) % menuSprites_.size();
		UpdateMenuAppearance();
	}
	if (input->TriggerKey(DIK_DOWN)) {
		selectedMenuIndex_ = (selectedMenuIndex_ + 1) % menuSprites_.size();
		UpdateMenuAppearance();
	}

	if (!input->TriggerKey(DIK_SPACE) && !input->TriggerKey(DIK_RETURN)) {
		return;
	}

	switch (static_cast<MenuItem>(selectedMenuIndex_)) {
	case MenuItem::kPlay:
		action_ = Action::kPlay;
		fade_->Start(Fade::Status::FadeOut, 1.0f);
		phase_ = Phase::kFadeOut;
		break;
	case MenuItem::kCredit:
		isCreditVisible_ = true;
		break;
	case MenuItem::kQuit:
		action_ = Action::kQuit;
		fade_->Start(Fade::Status::FadeOut, 1.0f);
		phase_ = Phase::kFadeOut;
		break;
	case MenuItem::kCount:
		break;
	}
}

void TitleScene::DrawMenu() {
	if (isCreditVisible_) {
		if (creditBackdropSprite_) {
			creditBackdropSprite_->Draw();
		}
		if (creditPlaceholderSprite_) {
			creditPlaceholderSprite_->Draw();
		}
		return;
	}

	for (Sprite* sprite : menuSprites_) {
		if (sprite) {
			sprite->Draw();
		}
	}
}
