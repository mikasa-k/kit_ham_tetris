/* 諸設定と関数定義 */
#define ROW_MAX 2048
#define ROWS_MAX 1000
#define WORD_MAX 256

#define MAP_H 20
#define MAP_W 10
const int WIDTH = 600;
const int HEIGHT = 600;


//文字列
#define CLASS_NAME "main"
#define window_title "KIT tetris"
#define title L"北見工大アマチュア無線部"
#define copyright L"©北見工業大学無線部"
#define conf_file_name "kit_tetris.ini"

//色
#define color_w RGB(255,255,255)
#define color_b RGB(0,0,0)
#define bg_color RGB(200,240,255)
#define h1_color RGB(50, 70, 230)
#define link_color RGB(17, 85, 170)
#define link_color2 RGB(200, 200, 200)
#define focus_color RGB(170, 85, 20)
#define focus_color2 RGB(250, 250, 0)

//方向
#define FRONT 1
#define BEHIND 2
#define LEFT 3
#define RIGHT 4


//id
#define SID_ALL 1000
#define SID_MENU 1
#define SID_MAIN 2
#define SID_SAVE 3
#define SID_END1 4
#define SID_END2 5
#define SID_END3 6
#define SID_END4 7


#define SWAP(a, b) ((a != b) ? (a += b, b = a - b, a -= b) : 0)

//configファイル
typedef struct{
	char key[WORD_MAX];
	int value;
} CONF_LIST;
static size_t conf_list_len = 0;
static size_t conf_list_use = 0;
static CONF_LIST *conf_list;


/* ファイル操作 */
int file_put_contents(const char * file_name, const char * contents, const char * mode){
	FILE* fp = fopen(file_name, mode);
	if (fp == NULL) return 1;
	fprintf(fp, contents);
	fclose(fp);
	return 0;
}

/* conf読み込み */
int read_conf(const char *file_name, const char *key){
	if (conf_list_len == 0){
		conf_list_len += 1;
		conf_list = (CONF_LIST *)malloc(conf_list_len*sizeof(CONF_LIST));
	}
	for (int i = 0; i < conf_list_len; ++i){
		if (strcmp(conf_list[i].key, key) == 0){
			return conf_list[i].value;
		}
	}
	FILE *fp = fopen(file_name, "rb");
	int value = 0;
	if (fp != NULL){
		char row[ROW_MAX];
		char k[WORD_MAX];
		int v;
		while (fgets(row, ROW_MAX, fp) != NULL){
			if (sscanf(row, "%s = %d", k, &v) > 0){
				if (strcmp(k, key) == 0){
					value = v;
					break;
				}
			}
		}
		fclose(fp);
	}
	if (conf_list_len == conf_list_use){
		conf_list_len += 1;
		conf_list = (CONF_LIST *)realloc(conf_list, (conf_list_len *sizeof(CONF_LIST)));
	}
	sprintf(conf_list[conf_list_use].key, "%s", key);
	conf_list[conf_list_use].value = value;
	conf_list_use += 1;
	return value;
}
void write_conf(const char* file_name, const char* key, int value){
	if (conf_list_len == 0){
		conf_list_len += 1;
		conf_list = (CONF_LIST *)malloc(conf_list_len*sizeof(CONF_LIST));
	}
	for (int i = 0; i < conf_list_len; ++i){
		if (strcmp(conf_list[i].key, key) == 0){
			conf_list[i].value = value;
			return;
		}
	}
	FILE *fp = fopen(file_name, "rb");
	char new_row[ROW_MAX];
	sprintf(new_row, "%s = %d\n", key, value);
	if (fp == NULL){
		file_put_contents(file_name, new_row, "wb");
		return;
	}
	char row[ROW_MAX];
	char rows[ROWS_MAX][ROW_MAX];
	char k[WORD_MAX];
	int v;
	int l = 0;
	int find = 0;
	while (fgets(row, ROW_MAX, fp) != NULL){
		if (sscanf(row, "%s = %d", k, &v) > 0){
			if (strcmp(k, key) == 0){
				strcpy(rows[l], new_row);
				find = 1;
			} else {
				strcpy(rows[l], row);
			}
		} else {
			strcpy(rows[l], row);
		}
		l++;
	}
	fclose(fp);
	remove(file_name);
	FILE *fp2 = fopen(file_name, "wb");
	for (int i = 0; i < l; ++i){
		fprintf(fp2, "%s", rows[i]);
	}
	if (find == 0){
		fprintf(fp2, "%s", new_row);
	}
	fclose(fp2);
}
void end_conf(void){
	free(conf_list);
}

HFONT set_font(int font_size, int bold){
	if (bold == 0){
		bold = FW_NORMAL;
	}
	return CreateFontW(
		font_size,0,0,0,bold,FALSE,FALSE,FALSE,SHIFTJIS_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
		DEFAULT_PITCH|FF_DONTCARE,L"Arial");
}

void print_text(HDC hdc, const wchar_t *str, int x, int y, int fs, int fg){
	HFONT f = set_font(fs, 0);
	SetTextColor(hdc, fg);
	SetBkMode(hdc, TRANSPARENT);
	HFONT of = (HFONT)SelectObject(hdc, f);
	TextOutW(hdc, x, y, str, wcslen(str));
	SelectObject(hdc, of);
	DeleteObject(f); //解放
}
void print_text_ex(HDC hdc, const wchar_t *str, int x, int y, int fs, int fg, int bg, int bold){
	HFONT f = set_font(fs, bold);
	SetTextColor(hdc, fg);
	if (bg == 0){
		SetBkMode(hdc, TRANSPARENT);
	} else {
		SetBkColor(hdc, bg);
	}
	HFONT of = (HFONT)SelectObject(hdc, f);
	TextOutW(hdc, x, y, str, wcslen(str));
	SelectObject(hdc, of);
	DeleteObject(f); //解放

}




