#include <windows.h>
#include <process.h>
#include <mmsystem.h>

#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "winmm.lib")


HWND hwMain;

// 送受信する座標データ
struct POS
{
	int x;
	int y;
};
POS pos1P, pos2P, old_pos1P;
RECT rect;

// プロトタイプ宣言
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI Threadfunc(void*);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_ LPSTR szCmdLine, _In_ int iCmdShow) {

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
	WSAStartup(MAKEWORD(2, 0), &wdData);

	hwMain = CreateWindow("CWindow", "Client",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600, NULL, NULL, hInstance, NULL);

	// ウインドウ表示
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

	return 0;
}

// ウインドウプロシージャ
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
			((LPCREATESTRUCT)lParam)->hInstance, "MAJO");
		if (hBitmap == NULL) {
			MessageBox(hwnd, "Failed to load majo_blue.bmp", "Error", MB_OK);
		}
		// ディスプレイと互換性のある論理デバイス（デバイスコンテキスト）を取得（1P）
		mdc = CreateCompatibleDC(NULL);
		// 論理デバイスに読み込んだビットマップを展開（1P）
		SelectObject(mdc, hBitmap);

		// リソースからビットマップを読み込む（2P）
		hBitmap2P = LoadBitmap(
			((LPCREATESTRUCT)lParam)->hInstance, "MAJO2P");
		if (hBitmap2P == NULL) {
			MessageBox(hwnd, "Failed to load majo_red.bmp", "Error", MB_OK);
		}
		// （2P）
		mdc2P = CreateCompatibleDC(NULL);
		// （2P）
		SelectObject(mdc2P, hBitmap2P);

		// 位置情報を初期化
		pos1P.x = pos1P.y = 0;
		pos2P.x = pos2P.y = 100;
		// データを送受信処理をスレッド（WinMainの流れに関係なく動作する処理の流れ）として生成。
		// データ送受信をスレッドにしないと何かデータを受信するまでRECV関数で止まってしまう。
		hThread = (HANDLE)CreateThread(NULL, 0, /*☆*/, (LPVOID)&pos2P, 0, &dwID);
		break;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_ESCAPE:
			SendMessage(hwnd, WM_CLOSE, NULL, NULL);
			break;
		case VK_RIGHT:
			//☆　→キー押されたらクライアント側キャラのX座標を更新
			break;
		case VK_LEFT:
			//☆　←キー押されたらクライアント側キャラのX座標を更新
			break;
		case VK_DOWN:
			//☆　↓キー押されたらクライアント側キャラのY座標を更新
			break;
		case VK_UP:
			//☆　↑キー押されたらクライアント側キャラのY座標を更新
			break;
		}

		// 指定ウィンドウの指定矩形領域を更新領域に追加
		// hWnd	 ：ウインドウのハンドル
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
		// サーバ側キャラ描画
		BitBlt(hdc, pos1P.x, pos1P.y, 32, 32, mdc, 0, 0, SRCCOPY);
		// クライアント側キャラ描画
		BitBlt(hdc, pos2P.x, pos2P.y, 32, 32, mdc2P, 0, 0, SRCCOPY);

		wsprintf(str, "サーバ側：X:%d Y:%d　　クライアント側：X:%d Y:%d", pos1P.x, pos1P.y, pos2P.x, pos2P.y);
		SetWindowText(hwMain, str);

		// 更新領域を空にする
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		/* ウインドウ破棄時 */
		DeleteObject(hBitmap);
		DeleteDC(mdc);
		DeleteObject(hBitmap2P);
		DeleteDC(mdc2P);

		PostQuitMessage(0);

		return 0;
	}

	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

/* 通信スレッド関数 */
DWORD WINAPI Threadfunc(void* px) {

	SOCKET sConnect;
	WORD wPort = 8000;
	SOCKADDR_IN saConnect;
	int iLen, iRecv;
	char szServer[1024] = { "自分PCのIPアドレス" };

	// ソケットをオープン
	sConnect = socket(/*☆*/, /*☆*/, 0);

	if (sConnect == INVALID_SOCKET) {
		SetWindowText(hwMain, "ソケットオープンエラー");
		return 1;
	}

	// サーバーを名前で取得する
	HOSTENT* lpHost;

	lpHost = gethostbyname(szServer);

	if (lpHost == NULL) {
		/* サーバーをIPアドレスで取得する */
		iLen = inet_addr(szServer);
		lpHost = gethostbyaddr((char*)&iLen, 4, AF_INET);
	}

	// クライアントソケットをサーバーに接続
	memset(&saConnect, 0, sizeof(SOCKADDR_IN));
	saConnect.sin_family = lpHost->h_addrtype;
	saConnect.sin_port = htons(htons(/*☆*/);
	saConnect.sin_addr.s_addr = *((u_long*)lpHost->h_addr);

	if (connect(sConnect, (SOCKADDR*)&saConnect, sizeof(saConnect)) == SOCKET_ERROR) {
		SetWindowText(hwMain, "サーバーと接続できませんでした");
		closesocket(sConnect);
		return 1;
	}

	SetWindowText(hwMain, "サーバーに接続できました");

	iRecv = 0;

	while (1)
	{
		// クライアント側キャラの位置情報を送信
		send(/*☆*/, (const char*)/*☆*/, sizeof(POS), 0);

		// 受信したクライアントが操作するキャラの座標が更新されていたら
		// 更新領域を作ってInvalidateRect関数でWM_PAINTメッセージを発行、キャラを再描画する
		if (old_pos1P.x != pos1P.x || old_pos1P.y != pos1P.y)
		{
			rect.left = old_pos1P.x - 10;
			rect.top = old_pos1P.y - 10;
			rect.right = old_pos1P.x + 42;
			rect.bottom = old_pos1P.y + 42;
			InvalidateRect(hwMain, &rect, TRUE);
		}

		int nRcv;

		old_pos1P = pos1P;

		// サーバ側キャラの位置情報を受け取り
		nRcv = recv(/*☆*/, (char*)/*☆*/, sizeof(POS), 0);

		if (nRcv == SOCKET_ERROR)break;

	}

	shutdown(sConnect, 2);
	closesocket(sConnect);

	return 0;
}