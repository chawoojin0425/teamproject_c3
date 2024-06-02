#include <SDL2/SDL.h>   // SDL 라이브러리 헤더파일
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define W 800     // 창의 너비
#define H 600     // 창의 높이
#define BW 60     // 블록의 너비
#define BH 30     // 블록의 높이
#define ROWS 5    // 블록의 행 수
#define COLS 10   // 블록의 열 수
#define PW 100    // 패들의 너비
#define PH 20     // 패들의 높이
#define BALL_SIZE 20 // 공의 크기
#define PS 400.0f // 패들의 속도
#define BS 300.0f // 공의 속도

typedef struct {    
    // x,y: 패들의 좌표값(왼쪽 상단 모서리 기준 x: 왼->오, y: 위->아래), w: 패들의 너비, h: 패들의 높이
    float x, y, w, h;
} Pdl;      // 패들을 나타내는 구조체

typedef struct {    // x,y: 공의 좌표값, vx: 공의 x축 방향 속도, vy: 공의 y축 방향 속도, r: 공의 반지름
    float x, y, vx, vy, r;
} Ball;     // 공을 나타내는 구조체

typedef struct {
    float x, y, w, h;
    int destroyed;      // 블록이 파괴되었는지 여부 (1=파괴, 0=파괴x)
} Block;    // 블록을 나타내는 구조체

void initPdl(Pdl* p, float x, float y) {    // 구조체 포인트를 이용하여 초기 패들의 위치와 크기를 선언
    p->x = x;
    p->y = y;
    p->w = PW;
    p->h = PH;
}

void initBall(Ball* b, float x, float y) {      // 구조체 포인트를 이용하여 초기 공의 위치, 속도, 크기를 선언
    b->x = x; b->y = y; b->vx = BS * cosf(45.0f * M_PI / 180.0f);
    b->vy = BS * sinf(45.0f * M_PI / 180.0f); b->r = BALL_SIZE / 2;
}

void initBlocks(Block blocks[ROWS][COLS]) {
    // 2차원 배열을 이용하여 앞서 #define을 통해 정의한 매크로 상수를 각각의 행렬의 개수로 지정함.
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            blocks[i][j].x = j * (BW + 5) + 35;
            blocks[i][j].y = i * (BH + 5) + 35;
            blocks[i][j].w = BW;
            blocks[i][j].h = BH;
            blocks[i][j].destroyed = 0;
        }
    }
}

void drawPdl(SDL_Renderer* r, Pdl* p) {
    // 렌더러(renderer): 컴퓨터 프로그램을 이용하여 2차원이나 3차원 이미지를 출력하는 방식.
    SDL_Rect rect = { (int)(p->x - p->w / 2), (int)(p->y - p->h / 2), (int)p->w, (int)p->h };
    // 패들의 중심점 - (너비 or 높이) / 2 
    SDL_RenderFillRect(r, &rect);       // RenderFillRect 함수를 사용하여 rect 사각형을 채움.
}

void drawBall(SDL_Renderer* r, Ball* b) {
    SDL_Rect rect = { (int)(b->x - b->r), (int)(b->y - b->r), (int)(b->r * 2), (int)(b->r * 2) };
    // 공의 반지름
    SDL_RenderFillRect(r, &rect);       // RenderFillRect 함수를 사용하여 rect 동그라미를 채움.
}

void drawBlock(SDL_Renderer* r, Block* b) {
    if (!b->destroyed) {
        SDL_Rect rect = { (int)b->x, (int)b->y, (int)b->w, (int)b->h };
        SDL_RenderFillRect(r, &rect);
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        // SDL_Init(SDL_INIT_VIDEO): SDL을 초기화. 여기서는 비디오 기능을 위해 초기화를 함.
        printf("SDL Init Failed: %s\n", SDL_GetError());
        return 1;       // 초기화에 실패하면 오류 메시지를 출력하고 프로그램을 종료시킴.
    }

    SDL_Window* win = SDL_CreateWindow("SDL Breakout", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_SHOWN);
    // SDL_CreateWindow(1. 윈도우 창 제목 문자열, 2. 윈도우의 위치 (여기서는 화면중앙에 배치.), 3. 너비, 4. 높이, 5. 윈도우의 특성을 결정하는 플래그(ex. 전체화면, 테두리 제거, 숨김 등))
    if (!win) {
        printf("Window Creation Failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    // SDL_CreateRenderer (1. 렌더링될 창을 나타냄., 2. 렌더러 인덱스 (-1: 기본 렌더러) 3. 랜더링의 특성)
    // SDL_RENDERER_ACCELERATED: 하드웨어를 가속하여 렌더링 속도를 향상시킴.
    // SDL_RENDERER_PRESENTVSYNC: 렌더링된 프레임을 디스플레이의 새로고침 속도와 동기화(synchronization)하여 화면을 부드럽게 이어줌.(게임에 중요함.) -> 하지 않을 시 화면 찢김 현상(어떤 부분은 빠르고 어떤 부분은 느림.)
    if (!ren) {
        printf("Renderer Creation Failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1;
    }

    Pdl pdl;
    Ball ball;
    // 패들과 공의 구조체를 가각 'pdl'과 'ball'로 선언.
    Block blocks[ROWS][COLS];
    // 2차원 배열 'blocks'를 선언함.

    initPdl(&pdl, W / 2, H - PH - 10);
    //initPdl 함수를 호출하여 pdl 구조체에 패들의 위치와 크기를 설정.
    //패들의 시작 x좌표: 창의 너비의 절반(창의 중앙에 위치함)
    //패들의 시작 y좌표: 창의 높이 - 블록의 높이 - 10
    initBall(&ball, W / 2, H / 2);
    //공의 시작 x좌표: 창의 너비의 절반
    //공의 시작 y좌표: 창의 높이의 절반
    //공의 시작 위치: 창의 한가운데
    initBlocks(blocks);
    // initBlocks 함수를 호출하여 'blocks' 2차원 배열에 각 블록의 위치, 파괴 여부, 색상 등을 설정함.
    

    int run = 1, play = 1;
    // run에서 1: 게임 루프를 실행, 0: 게임 루프를 종료.
    // play에서 1: 게임이 진행중, 0: 게임 종료.
    SDL_Event e;        // 아래에 SDL 이벤트를 구조체에 저장
    float dt = 0;       // dt 변수에 프레임 시간 간격을 나타냄.
    Uint32 last = SDL_GetTicks();
    // Unit 32: 부호 없는 정수형으로 시간을 저장
    // 현재 시스템 시간을 밀리초로 얻고 'last'라고 하는 변수에 저장.
    

    while (run) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) run = 0;    // 사용자가 창을 닫는다면 게임 루프 종료.
            else if (e.type == SDL_KEYDOWN) {   // 사용자가 키를 누르면
                if (e.key.keysym.sym == SDLK_ESCAPE) run = 0; // 키보드에서 'esc'를 누르면 게임 루프 종료.
                // e.key.keysym.sym: 사용자가 어떤 키를 눌렀는지 확인함.
                else if (e.key.keysym.sym == SDLK_SPACE) {  // 키보드에서 스페이스바를 누르면
                    if (!play) {    // !play: 게임이 진행 중이 아닌 상태
                        play = 1;   // 게임 재실행.
                        initPdl(&pdl, W / 2, H - PH - 10);
                        initBall(&ball, W / 2, H / 2);
                        initBlocks(blocks);
                        last = SDL_GetTicks();
                    }
                }
            }
        }

        if (play) {     // 게임이 진행 중이라면
            Uint32 now = SDL_GetTicks(); // 현재 시간(밀리초)를 변수'now'에 저장.
            dt = (now - last) / 1000.0f; // 속도 dt = 거리(이전 프레임과 현재 프레임 사이의 시간 간격) / 시간
            last = now;

            const Uint8* ks = SDL_GetKeyboardState(NULL);
            if (ks[SDL_SCANCODE_LEFT] && pdl.x - pdl.w / 2 > 0) pdl.x -= PS * dt;
            // 왼쪽 방향키를 누르고 게임 화면 왼쪽 끝을 넘어가지 않을 경우:
            // PS(패들의 이동속도) * dt(시간 간격) = 거리
            //계산된 거리만큼 패들의 x좌표(pdl.x)가 변함에 따라 왼쪽(-=)으로 이동함.
            if (ks[SDL_SCANCODE_RIGHT] && pdl.x + pdl.w / 2 < W) pdl.x += PS * dt;
            // 오른쪽(+=)으로 동일하게
            

            ball.x += ball.vx * dt; // 공의 x좌표 = (공의 x좌표 속도) * (시간 간격)
            ball.y += ball.vy * dt; // 공의 y좌표 = (공의 y좌표 속도) * (시간 간격)
            
            if (ball.y - ball.r < 0) ball.vy = -ball.vy;
            // 공이 상단 벽에 부딪히는 경우(y좌표 값이 공의 반지름 보다 작을 때 공의 방향을 반대방향으로 바꿈.)
            if (ball.x - ball.r < 0 || ball.x + ball.r > W) ball.vx = -ball.vx;
            // 공이 양옆의 벽에 부딪히는 경우(x좌표 값이 공의 반지름 보다 작거나 x좌표와 공의 반지름의 합이 창의 너비보다 클 때 공의 방향을 반대 방향으로 바꿈.)
            if (ball.y + ball.r > H) play = 0;
            // 공이 패들을 넘어 아래 쪽 끝 부분에 닿을 경우(공의 y좌표와 공의 반지름의 길이의 합이 창의 높이보다 클 때 게임을 종료함.)

            if (ball.y + ball.r > pdl.y - pdl.h / 2 &&
                // 공이 패들의 윗부분에 부딪혔는지? ((공의 y좌표+공의 반지름)이 (패들의 y좌표- 패들의 높이의 절반)보다 클 때 즉, 공이 패들의 중앙선과 접촉하는 순간)
                ball.x > pdl.x - pdl.w / 2 &&
                // 공이 패들의 왼쪽 끝 부분에서 안쪽으로 들어왔는지? ((공의 x좌표)가 패들의 왼쪽 끝 부분(패들의 중앙 x좌표-패들의 너비 절반)보다 커야함.)
                ball.x < pdl.x + pdl.w / 2) {
                // 공이 패들의 오른쪽 끝 부분에서 안쪽으로 들어왔는지? ((공의 x좌표)가 패들의 오른쪽 끝 부분(패들의 중앙 x좌표+패드의 너비 절반)보다 작아야함.)
                ball.vy = -fabs(ball.vy);
                // &&(논리 AND)로 연결된 위  세 조건을 모두 충족 시, 공의 방향을 반대로 바꿔줌.
            }

            for (int i = 0; i < ROWS; i++) {
                for (int j = 0; j < COLS; j++) {
                    Block* b = &blocks[i][j];       // 'blocks'라는 2차원 배열의 i번째 해 j번째 열에 있는 'Block' 구조체를 'b'라는 포인터 변수에 저장
                    if (!b->destroyed &&        // b->destroyed가 0이면 즉, 블록이 파괴되지 않았으면
                        ball.x > b->x &&
                        ball.x < b->x + b->w &&
                        ball.y > b->y &&
                        ball.y < b->y + b->h) {
                        b->destroyed = 1;
                        ball.vy = -ball.vy;
                        // 위 다섯 조건이 모두 충족 될 시에 블록이 파괴되었다는 것을 의미함.
                        // 그리고 공의 y좌표 속도에 '-'를 붙여 공을 튕겨나오게 함.
                        break;
                        // 현재 블록 루프를 종료하고 다음 블록으로 이동함.
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(ren, 50, 50, 50, 255);
        // SDL_SetRenderDrawColor(1. 렌더러 객체 포인터, red, green, blue, 알파(투명도 255가 가장 불투명))
        // 배경색을 회색으로 설정함.
        SDL_RenderClear(ren);

        if (play) {
            SDL_SetRenderDrawColor(ren, 100, 100, 200, 255);
            drawPdl(ren, &pdl);

            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
            
            drawBall(ren, &ball);

            SDL_SetRenderDrawColor(ren, 200, 100, 100, 255);
            for (int i = 0; i < ROWS; i++) {
                for (int j = 0; j < COLS; j++) {
                    drawBlock(ren, &blocks[i][j]);
                }
            }
        } else {
            SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        }

        SDL_RenderPresent(ren);
        // 렌더러(ren)에 그려진 모든 내용을 화면에 표시함.
    }

    SDL_DestroyRenderer(ren);
    // 렌더러 객체(ren)를 해제. 게임이 종료되면 더이상 렌더러가 필요하지 않기 때문에 메모리 절약을 위해 해제함.
    SDL_DestroyWindow(win);
    //창 객체도 해제.
    SDL_Quit(); // SDL 라이브러리를 초기화하고 SDL 자원을 정리함.

    return 0;
}
