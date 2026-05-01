#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define PX_PER_CELL 20
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define COLS (WINDOW_WIDTH / PX_PER_CELL)
#define ROWS (WINDOW_HEIGHT / PX_PER_CELL)
#define TOTAL_TILES (ROWS * COLS)
#define START_INDEX (ROWS / 2 * COLS + COLS / 2)

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef enum { EMPTY, FOOD, SNAKE_BODY } TileItems;
typedef enum { RIGHT, DOWN, LEFT, UP } Direction;
typedef enum { PLAYING, PAUSED } PlayState;
typedef enum { CONTINUE, RESTART, CLOSE } MenuChoice;

typedef struct {
  Direction dir;
  Direction pDir;
  int *cells;
  int head;
  int len;
} Snake;

Snake *snake;
int *board;
int food;

PlayState pState = PLAYING;
MenuChoice mChoice = CONTINUE;

int DELTAS[] = {1, COLS, -1, -COLS};
ULONGLONG seed;

ULONGLONG xorShift(void) {
  seed ^= seed << 13;
  seed ^= seed >> 7;
  seed ^= seed << 17;
  return seed;
}

BOOL CanPlaceFood(int p) {
  for (int i = 0; i < snake->len; i++) {
    int idx = (snake->head - i + TOTAL_TILES) % TOTAL_TILES;
    if (snake->cells[idx] == p) return FALSE;
  }
  return TRUE;
}

void InitNewGame(void) {
  __asm__(
      "mov rdi, %[base]\n"
      "mov ecx, %[count]\n"
      "loop_start:\n"
      "mov dword ptr [rdi], 0\n"
      "add rdi, 4\n"
      "dec ecx\n"
      "jnz loop_start\n"
      :
      : [base] "r"(board), [count] "i"(TOTAL_TILES)
      : "rdi", "ecx");
  seed = GetTickCount64();
  snake->head = 0;
  snake->len = 1;
  snake->dir = RIGHT;
  snake->pDir = RIGHT;
  snake->cells[0] = START_INDEX;
  do {
    food = (int)(xorShift() % TOTAL_TILES);
  } while (!CanPlaceFood(food));
  board[food] = FOOD;
  board[START_INDEX] = SNAKE_BODY;
  pState = PLAYING;
  mChoice = CONTINUE;
}

void *asmAlloc(SIZE_T size) {
  void *result;
  HANDLE heap = GetProcessHeap();
  HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
  void *heapAllocFn = GetProcAddress(k32, "HeapAlloc");
  __asm__(
      "mov rcx, %[heap]\n"
      "mov rdx, 0\n"
      "mov r8, %[sz]\n"
      "call %[fn]\n"
      "mov %[res], rax\n"
      : [res] "=r"(result)
      : [heap] "r"(heap), [sz] "r"(size), [fn] "r"(heapAllocFn)
      : "rcx", "rdx", "r8", "rax");
  return result;
}

void asmFree(void *ptr) {
  HANDLE heap = GetProcessHeap();
  HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
  void *heapFreeFn = GetProcAddress(k32, "HeapFree");
  __asm__(
      "mov rcx, %[heap]\n"
      "mov rdx, 0\n"
      "mov r8, %[ptr]\n"
      "call %[fn]\n"
      :
      : [heap] "r"(heap), [ptr] "r"(ptr), [fn] "r"(heapFreeFn)
      : "rcx", "rdx", "r8", "rax");
}

void HandleInGameKeys(WPARAM key) {
  Direction tempDir = snake->dir;
  switch (key) {
    case VK_RIGHT: tempDir = RIGHT; break;
    case VK_DOWN: tempDir = DOWN; break;
    case VK_LEFT: tempDir = LEFT; break;
    case VK_UP: tempDir = UP; break;
    default: return;
  }
  if (DELTAS[tempDir] + DELTAS[snake->dir] != 0) {
    snake->pDir = tempDir;
  }
}

BOOL HandleMenuing(WPARAM key) {
  switch (key) {
    case VK_SPACE:
    case VK_RETURN: pState = PLAYING; return TRUE;
    case VK_ESCAPE: PostQuitMessage(0); return TRUE;
  }
  return FALSE;
}

BOOL UpdateGame(void) {
  snake->dir = snake->pDir;
  int head = snake->cells[snake->head];
  int next = head + DELTAS[snake->dir];
  if (snake->dir == RIGHT && next % COLS == 0)
    next = next - COLS;
  else if (snake->dir == LEFT && next % COLS == COLS - 1)
    next = next + COLS;
  else if (next < 0)
    next = next + TOTAL_TILES;
  else if (next >= TOTAL_TILES)
    next = next - TOTAL_TILES;
  if (board[next] == SNAKE_BODY) return FALSE;
  if (next == food) {
    snake->head = (snake->head + 1) % TOTAL_TILES;
    snake->cells[snake->head] = next;
    snake->len++;
    board[next] = SNAKE_BODY;
    do {
      food = (int)(xorShift() % TOTAL_TILES);
    } while (!CanPlaceFood(food));
    board[food] = FOOD;
  } else {
    int tail = (snake->head - snake->len + 1 + TOTAL_TILES) % TOTAL_TILES;
    board[snake->cells[tail]] = EMPTY;
    snake->head = (snake->head + 1) % TOTAL_TILES;
    snake->cells[snake->head] = next;
    board[next] = SNAKE_BODY;
  }
  return TRUE;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
  board = (int *)asmAlloc(sizeof(int) * TOTAL_TILES);
  snake = (Snake *)asmAlloc(sizeof(Snake));
  snake->cells = (int *)asmAlloc(sizeof(int) * TOTAL_TILES);

  const wchar_t CLASS_NAME[] = L"SNAKE_WINDOW";
  WNDCLASS wc = {0};
  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);

  RegisterClass(&wc);
  RECT rc = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
  DWORD style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
  AdjustWindowRectEx(&rc, style, FALSE, 0);
  HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"C_Snake_mono", style,
                             CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left,
                             rc.bottom - rc.top, NULL, NULL, hInstance, NULL);
  if (hwnd == NULL) return 0;
  ShowWindow(hwnd, nCmdShow);
  InitNewGame();
  SetTimer(hwnd, 1, 130, NULL);
  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                            LPARAM lParam) {
  switch (uMsg) {
    case WM_DESTROY:
      asmFree(snake->cells);
      asmFree(snake);
      asmFree(board);
      PostQuitMessage(0);
      return 0;
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      FillRect(hdc, &ps.rcPaint, (HBRUSH)GetStockObject(BLACK_BRUSH));
      HBRUSH snakeBrush = CreateSolidBrush(RGB(255, 215, 0));
      for (int i = 0; i < snake->len; i++) {
        int idx = (snake->head - i + TOTAL_TILES) % (TOTAL_TILES);
        int cx = snake->cells[idx] % COLS, cy = snake->cells[idx] / COLS;
        RECT cellRect = {cx * PX_PER_CELL, cy * PX_PER_CELL,
                         cx * PX_PER_CELL + PX_PER_CELL,
                         cy * PX_PER_CELL + PX_PER_CELL};
        if (i > 0)
          FillRect(hdc, &cellRect, (HBRUSH)GetStockObject(WHITE_BRUSH));
        else
          FillRect(hdc, &cellRect, snakeBrush);
      }
      DeleteObject(snakeBrush);
      RECT foodRect = {(food % COLS) * PX_PER_CELL, (food / COLS) * PX_PER_CELL,
                       (food % COLS) * PX_PER_CELL + PX_PER_CELL,
                       (food / COLS) * PX_PER_CELL + PX_PER_CELL};
      HBRUSH foodBrush = CreateSolidBrush(RGB(0, 255, 0));
      FillRect(hdc, &foodRect, foodBrush);
      DeleteObject(foodBrush);
      if (pState == PAUSED) {
        RECT pausedRect = {0, 0, WINDOW_WIDTH, PX_PER_CELL * 2};
        HFONT pausedFont = CreateFont(
            -PX_PER_CELL, 0, 0, 0, FW_BOLD, TRUE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, TEXT("Consolas"));
        HFONT oldFont = (HFONT)SelectObject(hdc, pausedFont);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(255, 255, 255));
        FillRect(hdc, &pausedRect, (HBRUSH)GetStockObject(GRAY_BRUSH));
        FrameRect(hdc, &pausedRect, (HBRUSH)GetStockObject(WHITE_BRUSH));
        DrawText(hdc, L"Paused ~ ESC to quit, SPACE to resume", -1, &pausedRect,
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
        DeleteObject(pausedFont);
      }
      EndPaint(hwnd, &ps);
    }
      return 0;
    case WM_TIMER:
      switch (pState) {
        case PAUSED: break;
        case PLAYING:
          if (!UpdateGame()) InitNewGame();
          InvalidateRect(hwnd, NULL, FALSE);
      }
      return 0;
    case WM_KEYDOWN: {
      switch (pState) {
        case PAUSED:
          if (HandleMenuing(wParam) == TRUE) InvalidateRect(hwnd, NULL, FALSE);
          break;
        case PLAYING:
          if (wParam == VK_SPACE || wParam == VK_ESCAPE)
            pState = PAUSED;
          else
            HandleInGameKeys(wParam);
          InvalidateRect(hwnd, NULL, FALSE);
      }
      return 0;
    }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}