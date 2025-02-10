#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <windows.h>
#include <process.h>
#include <mmsystem.h>

#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "winmm.lib")


SOCKET sock;
HWND hwMain;

// 送受信する座標データ
struct POS
{
	int x;
	int y;
};
POS pos1P, old_pos1P, pos2P;
RECT rect;

bool bSocket = false;

// プロトタイプ宣言
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI threadfunc(void*);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR szCmdLine, int iCmdShow) {

	MSG  msg;
	WNDCLASS wndclass;
	WSADATA wdData;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = "CWindow";

	RegisterClass(&wndclass);

	// winsock初期化
	WSAStartup(MAKEWORD(1, 1), &wdData);

	// ウィンドウ作成
	hwMain = CreateWindow("CWindow", "Client",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600, NULL, NULL, hInstance, NULL);

	// ウィンドウ表示
	ShowWindow(hwMain, iCmdShow);

	// ウィンドウ領域更新(WM_PAINTメッセージを発行)
	UpdateWindow(hwMain);

	// メッセージループ
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// winsock終了
	WSACleanup();

	return msg.wParam;
}

/* ウインドウプロシージャー */
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {

	static HDC hdc, mdc, mdc2P;
	static PAINTSTRUCT ps;
	static HBITMAP hBitmap;
	static HBITMAP hBitmap2P;
	static char str[256];
	static HANDLE hThread;
	static DWORD dwID;

	// WINDOWSから飛んで来るメッセージに対応する処理の記述
	switch (iMsg) {
	case WM_CREATE:
		// リソースからビットマップを読み込む（1P）
		hBitmap = LoadBitmap(
			((LPCREATESTRUCT)lParam)->hInstance,
			"MAJO");

		// ディスプレイと互換性のある論理デバイス（デバイスコンテキスト）を取得
		mdc = CreateCompatibleDC(NULL);

		// 論理デバイスに読み込んだビットマップを展開
		SelectObject(mdc, hBitmap);

		// リソースからビットマップを読み込む（2P）
		hBitmap2P = LoadBitmap(
			((LPCREATESTRUCT)lParam)->hInstance,
			"MAJO2P");

		// 1Pと同じ
		mdc2P = CreateCompatibleDC(NULL);
		// 1Pと同じ
		SelectObject(mdc2P, hBitmap2P);

		pos1P.x = pos1P.y = 0;
		pos2P.x = pos2P.y = 100;

		// データを送受信処理をスレッド（WinMainの流れに関係なく動作する処理の流れ）として生成。
		// データ送受信をスレッドにしないと何かデータを受信するまでRECV関数で止まってしまう。
		hThread = (HANDLE)CreateThread(NULL, 0, &threadfunc, NULL, 0, &dwID);

		return 0;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_ESCAPE:
			SendMessage(hwnd, WM_CLOSE, NULL, NULL);
			break;
		case VK_RIGHT:
			if (pos2P.x < 750) pos2P.x += 10;
			break;
		case VK_LEFT:
			if (pos2P.x > 0) pos2P.x -= 10;
			break;
		case VK_DOWN:
			if (pos2P.y < 520) pos2P.y += 10;
			break;
		case VK_UP:
			if (pos2P.y > 0) pos2P.y -= 10;
			break;
		}

		// ウィンドウを更新領域に追加
		// hWnd	 ：	ウインドウのハンドル
		// lprec ：RECTのポインタ．NULLなら全体
		// bErase：TRUEなら更新領域を背景色で初期化， FALSEなら現在の状態から上書き描画
		// 返り値：成功すればTRUE，それ以外はFALSE
		InvalidateRect(hwnd, NULL, TRUE);

		// WM_PAINTメッセージがウィンドウに送信される
		UpdateWindow(hwnd);
		break;
	case WM_PAINT:
		// 更新領域に描画する為に必要な描画ツール（デバイスコンテキスト）を取得
		hdc = BeginPaint(hwnd, &ps);

		// 転送元デバイスコンテキストから転送先デバイスコンテキストへ
		// 長方形カラーデータのビットブロックを転送
		BitBlt(hdc, pos1P.x, pos1P.y, 32, 32, mdc, 0, 0, SRCCOPY);
		BitBlt(hdc, pos2P.x, pos2P.y, 32, 32, mdc2P, 0, 0, SRCCOPY);

		wsprintf(str, "X:%d Y:%d", pos2P.x, pos2P.y);
		SetWindowText(hwMain, str);

		// 更新領域を空にする
		EndPaint(hwnd, &ps);
		return 0;

		// ウインドウ破棄時
	case WM_DESTROY:
		DeleteObject(hBitmap);
		DeleteDC(mdc);
		DeleteObject(hBitmap2P);
		DeleteDC(mdc2P);

		// ソケット作られていたら開放
		if (bSocket) {
			shutdown(sock, 2);
			closesocket(sock);
		}

		PostQuitMessage(0);

		return 0;
	}

	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

/* 通信スレッド関数 */
DWORD WINAPI threadfunc(void*) {

	int recv_cnt;
	int send_cnt;
	int fromlen;
	struct sockaddr_in send_addr, recv_addr;


	sock = socket(PF_INET, SOCK_DGRAM, 0);
	
	ZeroMemory(&send_addr, sizeof(sockaddr_in));
	ZeroMemory(&recv_addr, sizeof(sockaddr_in));

	// ソケットにポート番号と送信先IPアドレスを設定
	send_addr.sin_family = AF_INET;
	send_addr.sin_port = htons(8000);
	send_addr.sin_addr.s_addr = inet_addr("192.168.3.4");

	while (1)
	{
		recv_cnt = 0;
		send_cnt = 0;
		fromlen = sizeof(recv_addr);

		// 送られてくる1Pの座標情報を退避
		old_pos1P = pos1P;

		//データ送信
		while (send_cnt == 0)
		{
			send_cnt = sendto(sock, (const char*)&pos2P, sizeof(POS), 0, (struct sockaddr*)&send_addr, sizeof(send_addr));
		}

		recv_cnt = recvfrom(sock, (char*)&pos1P, sizeof(POS), 0, (struct sockaddr*)&recv_addr, &fromlen);

		// 受信したサーバが操作するキャラの座標が更新されていたら、更新領域を作って
		// InvalidateRect関数でWM_PAINTメッセージを発行、キャラを再描画する
		if (old_pos1P.x != pos1P.x || old_pos1P.y != pos1P.y)
		{
			rect.left = old_pos1P.x - 10;
			rect.top = old_pos1P.y - 10;
			rect.right = old_pos1P.x + 42;
			rect.bottom = old_pos1P.y + 42;
			InvalidateRect(hwMain, &rect, TRUE);
		}
	}

	return 0;
}