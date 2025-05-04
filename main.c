#pragma GCC optimize("O2")
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <locale.h>
#include <wchar.h>
#include <string.h>
#include <math.h>
#include "resource.h"
#include "conf.h"
#include "bgm.h"

#define IF_DECIDE_KEY(key) (key==VK_RETURN)||(key==VK_SEPARATOR)

/* 座標系 */
typedef struct{
	int x;
	int y;
} POSITION;

/* 選択肢 */
typedef struct{
	int len;
	int focus;
} SELECT;

/* ステージ */
typedef struct{
	int main; //現在の指示
	int page; //サブ画面id
	int fade; //遅延表示用
	int is_fade; //遅延表示有効化
	int is_select; //選択肢有効化
} STAGE;

typedef struct{
	POSITION p; //座標
	int dr; //向き
} BLOCK;

typedef struct{
	int get;
	int revel;
	int is_end;
} MY_POINT;

typedef struct{
	HDC buffer;
	HBITMAP img;
	HBITMAP oldbit;
} IMAGES;

HDC hMemDC;
IMAGES image[] = {{NULL,NULL,NULL}, {NULL,NULL,NULL}};
STAGE stage = {SID_MENU, 0, 0, 0, 0};
SELECT options = {0, 0};
MY_POINT point = {0,0,0};
int block_map[MAP_H][MAP_W];
BLOCK my_block = {{4,0}, FRONT};


/* 記録 */
/*
 * ファイル構造
 * 点数 レベル 時刻
*/
#define DAT_FILE "save.dat"
typedef struct{
	time_t time;
	wchar_t date[19];
	int score;
	int revel;
} DAT_FILE_DATA;

DAT_FILE_DATA read_dat(void){
	DAT_FILE_DATA result = {0, L"-", 0, 0};
	FILE *fp = fopen(DAT_FILE, "rb");
	if (fp == NULL){
		return result;
	}
	fread(&result.score, sizeof(result.score), 1, fp);
	fread(&result.revel, sizeof(result.revel), 1, fp);
	fread(&result.time, sizeof(result.time), 1, fp);
	struct tm *local = localtime(&result.time);
	wcsftime(result.date, 19, L"%Y年%m月%d日 %H時%M分", local);
	fclose(fp);
	return result;
}

void save_dat(void){
	DAT_FILE_DATA wd, old;
	wd.revel = point.revel;
	wd.score = point.get;
	wd.time = time(NULL);
	old = read_dat();
	int is_add = 0;
	if ((old.time == 0) || (old.score < wd.score)){
		is_add = 1;
	}
	if (is_add == 0){
		return;
	}
	FILE *fp = fopen(DAT_FILE, "wb");
	int buf[] = {wd.score, wd.revel};
	fwrite(buf, sizeof(buf), 1, fp);
	fwrite(&wd.time, sizeof(wd.time), 1,fp);
	fclose(fp);
}

/* 選択肢 */
void move_focus(HWND hwnd, WPARAM key){
	if (stage.is_select == 0){
		return;
	}
	switch (key){
		case VK_LEFT:
		options.focus--;
		break;
		case VK_UP:
		options.focus--;
		break;
		case VK_RIGHT:
		options.focus++;
		break;
		case VK_DOWN:
		options.focus++;
		break;
		default:
		return;
	}
	if (options.focus < 0){
		options.focus = options.len -1;
	} else if (options.focus > options.len -1){
		options.focus = 0;
	}
	InvalidateRect(hwnd, NULL, 0);
}


/* 時間毎 */
void timer_action(HWND hwnd, int tid){
	switch (tid){
		case SID_ALL:{
			if (my_block.p.y < MAP_H-1 && block_map[my_block.p.y +1][my_block.p.x] == 0){
				my_block.p.y += 1;
			} else {
				block_map[my_block.p.y][my_block.p.x] = my_block.dr;
				//ゲームオーバー
				if (my_block.p.y == 0 && my_block.p.x == 4){
					point.is_end = 1;
					KillTimer(hwnd, tid);
					stop_bgm();
					save_dat();
					InvalidateRect(hwnd, NULL, 0);
					return;
				}
				my_block.p.x = 4;
				my_block.p.y = 0;
				my_block.dr = rand()%4;
				my_block.dr += 1;
				int i, j;
				int tmp[3] = {0,0,0};
				POSITION h4 = {0,0};
				POSITION v4 = {0,0};
				POSITION c4 = {0,0};
				//横
				for (i = 0; i < MAP_H; ++i){
					for (j = 0; j < MAP_W -3; ++j){
						if (block_map[i][j]==FRONT&&block_map[i][j+1]==BEHIND&&block_map[i][j+2]==LEFT&&block_map[i][j+3]==RIGHT){
							tmp[0] = 1;
							h4.x = j;
							h4.y = i;
						}
					}
				}
				//縦
				for (i = 0; i < MAP_H -3; ++i){
					for (j = 0; j < MAP_W; ++j){
						if (block_map[i][j]==FRONT&&block_map[i+1][j]==BEHIND&&block_map[i+2][j]==LEFT&&block_map[i+3][j]==RIGHT){
							tmp[1] = 1;
							v4.x = j;
							v4.y = i;
						}
					}
				}
				//斜め
				for (i = 0; i < MAP_H -3; ++i){
					for (j = 0; j < MAP_W -3; ++j){
						if (block_map[i][j]==FRONT&&block_map[i+1][j+1]==BEHIND&&block_map[i+2][j+2]==LEFT&&block_map[i+3][j+3]==RIGHT){
							tmp[2] = 1;
							c4.x = j;
							c4.y = i;
						}
					}
				}

				/* 横が消える */
				if (tmp[0]){
					point.get += 2;
					block_map[h4.y][h4.x] = 0;
					block_map[h4.y][h4.x+1] = 0;
					block_map[h4.y][h4.x+2] = 0;
					block_map[h4.y][h4.x+3] = 0;
					for (i = 0; i < 4; ++i){
						if (block_map[h4.y-1][h4.x+i]){
							for (j = h4.y-1; j > 0; --j){
								SWAP(block_map[j][h4.x+i], block_map[j+1][h4.x+i]);
							}
						}
					}
				}
				/* 縦が消える */
				if (tmp[1]){
					point.get += 4;
					block_map[v4.y][v4.x] = 0;
					block_map[v4.y+1][v4.x] = 0;
					block_map[v4.y+2][v4.x] = 0;
					block_map[v4.y+3][v4.x] = 0;
					if (block_map[v4.y-1][v4.x]){
						for (i = v4.y -1; i > 0; ++i){
							SWAP(block_map[i][v4.x], block_map[i+1][v4.x]);
						}
					}
				}
				/* 斜めが消える */
				if (tmp[2]){
					point.get += 8;
					block_map[c4.y][c4.x] = 0;
					block_map[c4.y+1][c4.x+1] = 0;
					block_map[c4.y+1][c4.x+2] = 0;
					block_map[c4.y+1][c4.x+3] = 0;
					for (i = 0; i < 4; ++i){
						if (block_map[c4.y+i-1][c4.x+i]){
							for (j = c4.y+i-1; j > 0; --j){
								SWAP(block_map[j][c4.x+i], block_map[j+1][c4.x+i]);
							}
						}
					}
				}



				/* 同じ文字が連続すると減点? */
				tmp[0] = 0;
				tmp[1] = 0;
				tmp[2] = 0;
				h4.x = 0;
				h4.y = 0;
				v4.x = 0;
				v4.y = 0;
				c4.x = 0;
				c4.y = 0;
				//横
				for (i = 0; i < MAP_H; ++i){
					for (j = 0; j < MAP_W -3; ++j){
						if (block_map[i][j] == 0) continue;
						if (block_map[i][j]==block_map[i][j+1]&&block_map[i][j+2]==block_map[i][j]&&block_map[i][j+3]==block_map[i][j]){
							tmp[0] = 1;
							h4.x = j;
							h4.y = i;
						}
					}
				}
				//縦
				for (i = 0; i < MAP_H -3; ++i){
					for (j = 0; j < MAP_W; ++j){
						if (block_map[i][j] == 0) continue;
						if (block_map[i][j]==block_map[i+1][j]&&block_map[i+2][j]==block_map[i][j]&&block_map[i+3][j]==block_map[i][j]){
							tmp[1] = 1;
							v4.x = j;
							v4.y = i;
						}
					}
				}
				//斜め
				for (i = 0; i < MAP_H -3; ++i){
					for (j = 0; j < MAP_W -3; ++j){
						if (block_map[i][j] == 0) continue;
						if (block_map[i][j]==block_map[i+1][j+1]&&block_map[i+2][j+2]==block_map[i][j]&&block_map[i+3][j+3]==block_map[i][j]){
							tmp[2] = 1;
							c4.x = j;
							c4.y = i;
						}
					}
				}

				/* 横が消える */
				if (tmp[0]){
					point.get -= 1;
					block_map[h4.y][h4.x] = 0;
					block_map[h4.y][h4.x+1] = 0;
					block_map[h4.y][h4.x+2] = 0;
					block_map[h4.y][h4.x+3] = 0;
					for (i = 0; i < 4; ++i){
						if (block_map[h4.y-1][h4.x+i]){
							for (j = h4.y-1; j > 0; --j){
								SWAP(block_map[j][h4.x+i], block_map[j+1][h4.x+i]);
							}
						}
					}
				}
				/* 縦が消える */
				if (tmp[1]){
					point.get -= 1;
					block_map[v4.y][v4.x] = 0;
					block_map[v4.y+1][v4.x] = 0;
					block_map[v4.y+2][v4.x] = 0;
					block_map[v4.y+3][v4.x] = 0;
					if (block_map[v4.y-1][v4.x]){
						for (i = v4.y -1; i > 0; ++i){
							SWAP(block_map[i][v4.x], block_map[i+1][v4.x]);
						}
					}
				}
				/* 斜めが消える */
				if (tmp[2]){
					point.get -= 1;
					block_map[c4.y][c4.x] = 0;
					block_map[c4.y+1][c4.x+1] = 0;
					block_map[c4.y+1][c4.x+2] = 0;
					block_map[c4.y+1][c4.x+3] = 0;
					for (i = 0; i < 4; ++i){
						if (block_map[c4.y+i-1][c4.x+i]){
							for (j = c4.y+i-1; j > 0; --j){
								SWAP(block_map[j][c4.x+i], block_map[j+1][c4.x+i]);
							}
						}
					}
				}

				if (point.get > pow(point.revel, 1.5)){
					point.revel += 1;
					KillTimer(hwnd, SID_ALL);
					SetTimer(hwnd, SID_ALL, 1000/point.revel, NULL);
				}
				if (point.get < 0){
					point.is_end = 1;
					KillTimer(hwnd, tid);
					stop_bgm();
					save_dat();
				}
			}
			InvalidateRect(hwnd, NULL, 0);
		}
		break;

		case SID_MENU:{
			stage.fade++;
			if (stage.fade > 5){
				stage.is_fade = 0;
				KillTimer(hwnd, tid);
			}
		}
		break;
		case SID_MAIN:{
			stage.fade++;
			if (stage.fade > 4){
				stop_bgm();
				stage.is_fade = 0;
				KillTimer(hwnd, tid);
				SetTimer(hwnd, SID_ALL, 1000, NULL);
			}
		}
		break;
		case SID_SAVE:{
			stage.fade++;
			if (stage.fade > 5){
				stage.is_fade = 0;
				KillTimer(hwnd, tid);
			}
		}
		break;
		case SID_END1:{
			stage.fade++;
			if (stage.fade > 6){
				stage.is_fade = 0;
				stage.main = SID_MENU;
				KillTimer(hwnd, tid);
			}
		}
		break;
		case SID_END2:{
			stage.fade++;
			if (stage.fade > 6){
				stage.is_fade = 0;
				stage.main = SID_MENU;
				KillTimer(hwnd, tid);
			}
		}
		break;
		case SID_END3:{
			stage.fade++;
			if (stage.fade > 6){
				stage.is_fade = 0;
				stage.main = SID_MENU;
				KillTimer(hwnd, tid);
			}
		}
		break;
		case SID_END4:{
			stage.fade++;
			if (stage.fade > 6){
				stage.is_fade = 0;
				stage.main = SID_MENU;
				KillTimer(hwnd, tid);
			}
		}
		break;
	}
	InvalidateRect(hwnd, NULL, 0);
}

/* 入力 */
void input_action(HWND hwnd, WPARAM key){
	switch (stage.main){
		case SID_MENU:{
			stage.is_select = 1;
			options.len = 4;
			move_focus(hwnd, key);
			if (IF_DECIDE_KEY(key)){
				switch (options.focus){
					case 0:
					stage.main = SID_MAIN;
					stage.fade = 0;
					stage.is_fade = 1;
					my_block.p.x = 4;
					my_block.p.y = 0;
					my_block.dr = rand()%4;
					my_block.dr += 1;
					int i, j;
					for (i = 0; i < MAP_H; ++i){
						for (j = 0; j < MAP_W; ++j){
							block_map[i][j] = 0;
						}
					}
					point.get = 0;
					point.revel = 0;
					point.is_end = 0;
					stop_bgm();
					play_bgm("wav/kit_radio.wav", 0);
					SetTimer(hwnd, stage.main, 1000, NULL);
					break;
					case 1:
					stage.main = SID_SAVE;
					stage.fade = 0;
					stage.is_fade = 1;
					SetTimer(hwnd, stage.main, 100, NULL);
					break;
					case 2:
					SendMessage(hwnd, WM_DESTROY, 0, 0);
					return;
					break;
					case 3:
					if (read_conf(conf_file_name, "audio") == 0){
						write_conf(conf_file_name, "audio", 1);
					} else {
						stop_bgm();
						write_conf(conf_file_name, "audio", 0);
					}
					options.focus = 0;
					break;
				}
				stage.is_select = 0;
				InvalidateRect(hwnd, NULL, 0);
			}
		}
		break;
		case SID_MAIN:{
			if (point.is_end && (IF_DECIDE_KEY(key))){
				if (point.get < 20){
					stage.main = SID_END1;
				} else if (point.get < 50){
					stage.main = SID_END2;
				} else if (point.get < 100){
					stage.main = SID_END3;
				} else {
					stage.main = SID_END4;
				}
				stage.is_fade = 1;
				stage.fade = 0;
				stage.page = 0;
				KillTimer(hwnd, SID_ALL);
				int gap = read_conf(conf_file_name, "page_speed");
				if (gap < 100 || gap > 1000*20){
					gap = 1000 *5;
				}
				SetTimer(hwnd, stage.main, gap, NULL);
				InvalidateRect(hwnd, NULL, 0);
				return;
			}

			POSITION mae = {my_block.p.x, my_block.p.y};
			switch (key){
				case VK_UP:
				/*
				my_block.dr += 1;
				if (my_block.dr > 4){
					my_block.dr = 1;
				}
				*/
				break;
				case VK_LEFT:
				my_block.p.x -=1;
				break;
				case VK_RIGHT:
				my_block.p.x += 1;
				break;
				case VK_DOWN:
				my_block.p.y += 1;
				break;
			}
			if (my_block.p.x < 0 || my_block.p.x >= MAP_W){
				my_block.p.x = mae.x;
			}
			if (my_block.p.y >= MAP_H){
				my_block.p.y = mae.y;
			}
			if (block_map[my_block.p.y][my_block.p.x]){
				my_block.p.x = mae.x;
				my_block.p.y = mae.y;
			}
			InvalidateRect(hwnd, NULL, 0);
		}
		break;
		case SID_SAVE:{
			if (IF_DECIDE_KEY(key)){
				stage.main = SID_MENU;
				stage.is_fade = 1;
				stage.fade = 0;
				SetTimer(hwnd, stage.main, 100, NULL);
				InvalidateRect(hwnd, NULL, 0);
			}
		}
		break;
		/* endは自動再生にしようかな */
		case SID_END1:{
		}
		break;
		case SID_END2:{
		}
		break;
		case SID_END3:{
		}
		break;
		case SID_END4:{
		}
		break;
	}
}

/* 表示 */
void show(PAINTSTRUCT ps){
	switch (stage.main){
		case SID_MENU:{
			play_bgm("wav/Schubert_Deutsche_Messe_SATB.wav", 1);
			HBRUSH hBrush = (HBRUSH)CreateSolidBrush(bg_color);
			FillRect(hMemDC, &ps.rcPaint, hBrush);
			DeleteObject(hBrush); //解放
			if (stage.is_fade){
				return;
			}

			int cs[4] = {link_color, link_color, link_color, link_color};
			int bs[4] = {0, 0, 0, 0};
			if (options.focus < 4 && options.focus >= 0){
				cs[options.focus] = focus_color;
				bs[options.focus] = 700;
			}
			print_text_ex(hMemDC, L"開始", WIDTH /2 -50, 150, 25, cs[0], 0, bs[0]);
			print_text_ex(hMemDC, L"情報", WIDTH /2 -50, 200, 25, cs[1], 0, bs[1]);
			print_text_ex(hMemDC, L"終了", WIDTH /2 -50, 250, 25, cs[2], 0, bs[2]);
			if (read_conf(conf_file_name, "audio") == 0){
				print_text_ex(hMemDC, L"音出す", WIDTH -100, HEIGHT -70, 25, cs[3], 0, bs[3]);
			} else {
				print_text_ex(hMemDC, L"音消す", WIDTH -100, HEIGHT -70, 25, cs[3], 0, bs[3]);
			}
			print_text(hMemDC, title, 10, 10, 30, h1_color);
			print_text(hMemDC, L"使い方", 10, 300, 22, color_b);
			print_text(hMemDC, L"全てキーボードで操作します。", 10, 330, 22, color_b);
			print_text(hMemDC, L"移動: 方向キー、決定: Enter", 10, 360, 22, color_b);
			print_text(hMemDC, copyright, WIDTH /2 -100, HEIGHT -50, 20, color_b);
		}
		break;
		case SID_MAIN:{
			HBRUSH hBrush = (HBRUSH)CreateSolidBrush(bg_color);
			FillRect(hMemDC, &ps.rcPaint, hBrush);
			DeleteObject(hBrush); //解放
			if (stage.is_fade){
				return;
			}
			if (point.is_end){
				print_text(hMemDC, L"ゲームオーバー", 200, 200, 30, color_b);
				return;
			}
			play_bgm("wav/kitami.wav", 1);
			for (int i = 0; i < 2; ++i){
				image[i].buffer = CreateCompatibleDC(hMemDC);
				image[i].oldbit = (HBITMAP)SelectObject(image[i].buffer, image[i].img);
			}
			int i, j;
			for (i = 0; i < MAP_H; ++i){
				for (j = 0; j < MAP_W; ++j){
					TransparentBlt(hMemDC, j*24, i*24, 24, 24, image[0].buffer, 0, 0, 24, 24, color_w);
					switch (block_map[i][j]){
						case 0:
						//print_text(hMemDC, L"_", j*24, i*24, 24, color_b);
						break;
						case FRONT:
						print_text(hMemDC, L"北", j*24, i*24, 24, RGB(255,150,55));
						break;
						case BEHIND:
						print_text(hMemDC, L"見", j*24, i*24, 24, RGB(170,172,246));
						break;
						case LEFT:
						print_text(hMemDC, L"工", j*24, i*24, 24, RGB(255,77,141));
						break;
						case RIGHT:
						print_text(hMemDC, L"大", j*24, i*24, 24, RGB(31,185,56));
						break;
					}
				}
			}
			if (!point.is_end){
				switch (my_block.dr){
					case FRONT:
					print_text(hMemDC, L"北", my_block.p.x*24, my_block.p.y*24, 24, color_w);
					break;
					case BEHIND:
					print_text(hMemDC, L"見", my_block.p.x*24, my_block.p.y*24, 24, color_w);
					break;
					case LEFT:
					print_text(hMemDC, L"工", my_block.p.x*24, my_block.p.y*24, 24, color_w);
					break;
					case RIGHT:
					print_text(hMemDC, L"大", my_block.p.x*24, my_block.p.y*24, 24, color_w);
					break;
				}
			}

			wchar_t score[128], revel[128];
			swprintf(score, 128, L"得点:　%d", point.get);
			swprintf(revel, 128, L"レベル:%d", point.revel);
			print_text(hMemDC, L"ステータス", 250, 10, 30, RGB(255,177,55));
			print_text(hMemDC, score, 300, 50, 25, RGB(255,55,145));
			print_text(hMemDC, revel, 300, 80, 25, RGB(55,55,255));
			print_text(hMemDC, L"よこ2点 たて4点 ななめ8点", 250, 150, 25, color_b);
			print_text(hMemDC, L"おなじ もじが ならぶと減点", 250, 180, 25, color_b);
			TransparentBlt(hMemDC, 300, 300, 200, 200, image[1].buffer, 0, 0, 200, 200, color_w);
			print_text(hMemDC, L"ルール:「北見工大」を作ろう。", 10, HEIGHT -100, 25, color_b);
			for (i = 0; i < 2; ++i){
				SelectObject(image[i].buffer, image[i].oldbit);
				DeleteDC(image[i].buffer);
			}
		}
		break;
		case SID_SAVE:{
			HBRUSH hBrush = (HBRUSH)CreateSolidBrush(bg_color);
			FillRect(hMemDC, &ps.rcPaint, hBrush);
			DeleteObject(hBrush); //解放
			if (stage.is_fade){
				return;
			}
			print_text(hMemDC, L"最高記録", 10, 10, 30, h1_color);
			DAT_FILE_DATA dat = read_dat();
			wchar_t score[128], revel[128];
			swprintf(score, 128, L"得点:%d", dat.score);
			swprintf(revel, 128, L"レベル:%d", dat.revel);
			print_text(hMemDC, score, 50, 100, 25, color_b);
			print_text(hMemDC, revel, 150, 100, 25, color_b);
			print_text(hMemDC, dat.date, 300, 100, 20, color_b);
		}
		break;
		case SID_END1:{
			HBRUSH hBrush = (HBRUSH)CreateSolidBrush(color_b);
			FillRect(hMemDC, &ps.rcPaint, hBrush);
			DeleteObject(hBrush); //解放
			print_text(hMemDC, L"END1", WIDTH -100, HEIGHT -100, 25, color_w);
			print_text(hMemDC, L"部長の手記", 10, 10, 25, color_w);
			switch (stage.fade){
				case 0:
				print_text(hMemDC, L"気が付いたら部長でした。", 100, 300, 25, color_w);
				break;
				case 1:
				print_text(hMemDC, L"・・・", 100, 300, 25, color_w);
				break;
				case 2:
				print_text(hMemDC, L"本当は、", 10, 250, 25, color_w);
				print_text(hMemDC, L"一年生に通帳を渡した時点で怪しいと思ったんだ。", 10, 300, 25, color_w);
				break;
				case 3:
				break;
				case 4:
				print_text(hMemDC, L"・・・", 100, 300, 25, color_w);
				break;
				case 5:
				break;
				case 6:
				print_text(hMemDC, L"何も活動しなまま一年が終わりそうだよ", 50, 300, 25, color_w);
				break;
			}
		}
		break;
		case SID_END2:{
			HBRUSH hBrush = (HBRUSH)CreateSolidBrush(color_b);
			FillRect(hMemDC, &ps.rcPaint, hBrush);
			DeleteObject(hBrush); //解放
			print_text(hMemDC, L"END2", WIDTH -100, HEIGHT -100, 25, color_w);
			print_text(hMemDC, L"部長の手記", 10, 10, 25, color_w);
			switch (stage.fade){
				case 0:
				print_text(hMemDC, L"いつの間にか役員になっていました。", 50, 300, 25, color_w);
				break;
				case 1:
				print_text(hMemDC, L"どうも役員は部長から選ばれるらしい。", 50, 300, 25, color_w);
				break;
				case 2:
				print_text(hMemDC, L"・・・", 100, 300, 25, color_w);
				break;
				case 3:
				print_text(hMemDC, L"なかに同級生がいたのでちょっと安心したよ", 50, 300, 25, color_w);
				break;
				case 4:
				print_text(hMemDC, L"黎明期の部長は自分だけだけど...", 100, 300, 25, color_w);
				break;
				case 5:
				break;
				case 6:
				print_text(hMemDC, L"もちろん、積み重ねた歴史などありゃしない", 50, 300, 25, color_w);
				break;
			}
		}
		break;
		case SID_END3:{
			HBRUSH hBrush = (HBRUSH)CreateSolidBrush(color_b);
			FillRect(hMemDC, &ps.rcPaint, hBrush);
			DeleteObject(hBrush); //解放
			print_text(hMemDC, L"END3", WIDTH -100, HEIGHT -100, 25, color_w);
			print_text(hMemDC, L"部長の手記", 10, 10, 25, color_w);
			switch (stage.fade){
				case 0:
				print_text(hMemDC, L"さて、どうしたのもか。部活動紹介。", 50, 300, 25, color_w);
				break;
				case 1:
				print_text(hMemDC, L"話すことなんて何もない。", 100, 300, 25, color_w);
				break;
				case 2:
				print_text(hMemDC, L"・・・", 100, 300, 25, color_w);
				break;
				case 3:
				print_text(hMemDC, L"よし。こじつけで無理やり書こう。", 50, 300, 25, color_w);
				break;
				case 4:
				break;
				case 5:
				print_text(hMemDC, L"部員がごく僅かなことも、", 10, 250, 25, color_w);
				print_text(hMemDC, L"部室がすっからかんなことも", 50, 300, 25, color_w);
				break;
				case 6:
				print_text(hMemDC, L"全部ネタにしてしまえ！！", 100, 300, 30, color_w);
				break;
			}
		}
		break;
		case SID_END4:{
			HBRUSH hBrush = (HBRUSH)CreateSolidBrush(color_b);
			FillRect(hMemDC, &ps.rcPaint, hBrush);
			DeleteObject(hBrush); //解放
			print_text(hMemDC, L"END4", WIDTH -100, HEIGHT -100, 25, color_w);
			print_text(hMemDC, L"部長の手記", 10, 10, 25, color_w);
			switch (stage.fade){
				case 0:
				print_text(hMemDC, L"部活動紹介、", 10, 250, 25, color_w);
				print_text(hMemDC, L"少々ネタに走り過ぎたかと心配したが杞憂でした。", 10, 300, 25, color_w);
				break;
				case 1:
				print_text(hMemDC, L"。。。", 100, 300, 25, color_w);
				break;
				case 2:
				print_text(hMemDC, L"・・・", 100, 300, 25, color_w);
				break;
				case 3:
				print_text(hMemDC, L"しかし部員を確保したはいいものの", 50, 300, 25, color_w);
				break;
				case 4:
				print_text(hMemDC, L"・・・", 100, 300, 25, color_w);
				break;
				case 5:
				print_text(hMemDC, L"どうやって運営すればいいの？", 100, 300, 25, color_w);
				break;
				case 6:
				print_text(hMemDC, L"、、、、、、、", 100, 300, 30, color_w);
				break;
			}
		}
		break;
	}
}

/* event */
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	static HBITMAP hMemPrev;
	switch (msg){
		case WM_ERASEBKGND:{
			return 0;
		}
		break;
		case WM_KEYDOWN:{
			input_action(hwnd, wParam);
		}
		break;
		case WM_KEYUP:{
		}
		break;
		case WM_TIMER:{
			timer_action(hwnd, wParam);
		}
		break;
		case WM_CREATE:{
			HDC hdc = GetDC(hwnd);
			hMemDC = CreateCompatibleDC(hdc);
			HBITMAP hBitmap = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
			hMemPrev = (HBITMAP)SelectObject(hMemDC, hBitmap);
			ReleaseDC(hwnd, hdc);

			unsigned int seed = (unsigned int)time(NULL);
			srand(seed);
		}
		break;
		case WM_PAINT:{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			show(ps);
			BitBlt(hdc, 0, 0, WIDTH, HEIGHT, hMemDC, 0, 0, SRCCOPY);
			EndPaint(hwnd, &ps);
		}
		break;

		case WM_DESTROY:{
		HBITMAP hBitmap = (HBITMAP)SelectObject(hMemDC, hMemPrev);
		DeleteObject(hBitmap);
		DeleteObject(hMemDC);
		PostQuitMessage(0);
		return 0;
		}
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* main */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd){
	HANDLE hMutex = CreateMutexW(NULL, TRUE, L"kit_tetris1.00");
	if (GetLastError() == ERROR_ALREADY_EXISTS){
		return 0;
	}
	//UTF8(標準入出力でしか使えない)
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	WNDCLASSA wc = {0};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(APP_ICON));
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	RegisterClassA(&wc);
	HWND hMainWindow = CreateWindowExA(
		WS_EX_APPWINDOW, CLASS_NAME, window_title,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		//WS_OVERLAPPEDWINDOW|WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,// 初期位置
		WIDTH, HEIGHT,	// 初期サイズ
		NULL,
		NULL,
		hInstance,
		NULL
	);

	if (hMainWindow == NULL){
		return 0;
	}
	HICON hSmallIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(APP_ICON), IMAGE_ICON, 16, 16, 0);
	HICON hLargeIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(APP_ICON), IMAGE_ICON, 32, 32, 0);
	image[0].img = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(BGIMG), IMAGE_BITMAP, 24, 24, 0);
	image[1].img = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(ICON_IMG), IMAGE_BITMAP, 200, 200, 0);

	SendMessage(hMainWindow, WM_SETICON, ICON_SMALL, (LPARAM)hSmallIcon);
	SendMessage(hMainWindow, WM_SETICON, ICON_BIG, (LPARAM)hLargeIcon);
	ShowWindow(hMainWindow, nShowCmd);
	UpdateWindow(hMainWindow);

	MSG msg = {0};
	while (GetMessage(&msg, NULL, 0, 0)){
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	end_conf();

	ReleaseMutex(hMutex);
	CloseHandle(hMutex);
	return 0;
}

