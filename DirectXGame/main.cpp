#include "GameScene.h"
#include "KamataEngine.h"
#include <Windows.h>

// KamataEngine::を毎回入力しなくてもいい様にする
using namespace KamataEngine;

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	// エンジンの初期化
	Initialize(L"LC1A_25_ババ_マサオミ_AL2_5-3");

	// DirectXCommonインスタンスを取得(画面描画に使用)
	DirectXCommon* dxCoommon = DirectXCommon::GetInstance();

	// ImGuiのインスタンスを取得
	ImGuiManager* imGuiManager = ImGuiManager::GetInstance();

	/*-------------------- インスタンス生成 --------------------*/
	// ゲームシーン
	GameScene* gameScene = new GameScene;

	/*-------------------- 初期化処理 --------------------*/
	// ゲームシーン
	gameScene->Initialize();

	// メインループ
	while (true) {
		// エンジンの更新
		if (Update()) {
			break;
		}

		/*--------------- 更新処理 ---------------*/
		// ImGui受付開始
		imGuiManager->Begin();

		// ゲームシーン
		gameScene->Update();

		// ImGui受付終了
		imGuiManager->End();

		/*--------------- 描画処理 ---------------*/
		// 描画開始
		dxCoommon->PreDraw();

		// 描画処理を記述
		// ゲームシーン
		gameScene->Draw();

		// ImGui描画
		imGuiManager->Draw();

		// 描画終了
		dxCoommon->PostDraw();
	}

	// 解放処理
	delete gameScene;

	// nullputrの代入(解放処理とセットで)
	gameScene = nullptr;

	// エンジンの終了
	Finalize();

	return 0;
}