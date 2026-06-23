#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "raylib.h"

#ifndef DEG2RAD
#define DEG2RAD (3.14159265358979323846f/180.0f)
#endif

// Oyun Durumları
#define STATE_WELCOME  0
#define STATE_PLAYING  1
#define STATE_WON      2

// Taş Tipleri
#define TYPE_DOTS       0 // Noktalar (Pin)
#define TYPE_BAMBOO     1 // Bambular (Sou)
#define TYPE_CHARS      2 // Karakterler (Wan)
#define TYPE_WINDS      3 // Rüzgarlar (Doğu, Güney, Batı, Kuzey)
#define TYPE_DRAGONS    4 // Ejderhalar (Kırmızı, Yeşil, Beyaz)
#define TYPE_SEASONS    5 // Mevsimler (İlkbahar, Yaz, Sonbahar, Kış)
#define TYPE_FLOWERS    6 // Çiçekler (Erik, Orkide, Krizantem, Bambu)

// Taş Yapısı
typedef struct {
    int type;   // Taş Tipi
    int value;  // Taş Değeri (1-9 veya 1-4)
    int id;     // Benzersiz ID
} Tile;

// Tahta Hücresi Yapısı
typedef struct {
    Tile tile;
    int x;
    int y;
    int z;
    bool active;
} BoardTile;

// Geri Alma Yapısı
typedef struct {
    int z1, y1, x1;
    Tile t1;
    int z2, y2, x2;
    Tile t2;
} UndoAction;

// Buton Yapısı
typedef struct {
    Rectangle rect;
    const char *text;
    Color color;
    Color hover_color;
} GuiButton;

// Oyun Durumu Yapısı
typedef struct {
    BoardTile board[4][8][12];
    int selected_z, selected_y, selected_x;
    int hint_z1, hint_y1, hint_x1;
    int hint_z2, hint_y2, hint_x2;
    int undo_ptr;
    UndoAction undo_stack[72];
    char status_log[256];
    time_t start_time;
    int elapsed_seconds;
    bool has_won;
    int score;
} GameState;

GameState game;
int current_state = STATE_WELCOME;

// Layout koordinat sabitleyicileri
const int start_x = 70;
const int start_y = 60;
const int spacing_x = 54;
const int spacing_y = 70;
const int tile_w = 50;
const int tile_h = 68;

// Taş Eşleşme Mantığı
bool tiles_match(Tile t1, Tile t2) {
    if (t1.type != t2.type) return false;
    // Mevsimler ve Çiçekler kendi aralarında eşleşebilir
    if (t1.type == TYPE_SEASONS || t1.type == TYPE_FLOWERS) {
        return true;
    }
    return t1.value == t2.value;
}

// Hücre aktif mi kontrolü
bool is_tile_active(int z, int y, int x) {
    if (z < 0 || z >= 4 || y < 0 || y >= 8 || x < 0 || x >= 12) return false;
    return game.board[z][y][x].active;
}

// Taşın serbest (seçilebilir) olup olmadığını kontrol eder
bool is_tile_free(int z, int y, int x) {
    if (!is_tile_active(z, y, x)) return false;
    
    // Üzerinde aktif bir taş var mı?
    if (z < 3 && is_tile_active(z + 1, y, x)) {
        return false;
    }
    
    // Solunda veya sağında aktif bir taş yoksa serbesttir
    bool left_free = (x == 0) || !is_tile_active(z, y, x - 1);
    bool right_free = (x == 11) || !is_tile_active(z, y, x + 1);
    
    return left_free || right_free;
}

// Mevcut aktif taş sayısını döndürür
int get_remaining_tiles() {
    int count = 0;
    for (int z = 0; z < 4; z++) {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 12; x++) {
                if (game.board[z][y][x].active) count++;
            }
        }
    }
    return count;
}

// Yapılabilecek hamleleri bulur ve sayısını döndürür
int get_available_moves(int *p_z1, int *p_y1, int *p_x1, int *p_z2, int *p_y2, int *p_x2) {
    int count = 0;
    for (int i = 0; i < 384; i++) {
        int z1 = i / (8 * 12);
        int y1 = (i / 12) % 8;
        int x1 = i % 12;
        
        if (!is_tile_free(z1, y1, x1)) continue;
        
        for (int j = i + 1; j < 384; j++) {
            int z2 = j / (8 * 12);
            int y2 = (j / 12) % 8;
            int x2 = j % 12;
            
            if (!is_tile_free(z2, y2, x2)) continue;
            
            if (tiles_match(game.board[z1][y1][x1].tile, game.board[z2][y2][x2].tile)) {
                if (count == 0 && p_z1 && p_y1 && p_x1 && p_z2 && p_y2 && p_x2) {
                    *p_z1 = z1; *p_y1 = y1; *p_x1 = x1;
                    *p_z2 = z2; *p_y2 = y2; *p_x2 = x2;
                }
                count++;
            }
        }
    }
    return count;
}

// Tahtayı yeniden karıştırır
void reshuffle_board() {
    Tile active_tiles[144];
    int active_count = 0;
    
    for (int z = 0; z < 4; z++) {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 12; x++) {
                if (game.board[z][y][x].active) {
                    active_tiles[active_count++] = game.board[z][y][x].tile;
                }
            }
        }
    }
    
    if (active_count == 0) return;
    
    // Karıştır
    for (int i = active_count - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Tile temp = active_tiles[i];
        active_tiles[i] = active_tiles[j];
        active_tiles[j] = temp;
    }
    
    // Geri yerleştir
    int idx = 0;
    for (int z = 0; z < 4; z++) {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 12; x++) {
                if (game.board[z][y][x].active) {
                    game.board[z][y][x].tile = active_tiles[idx++];
                }
            }
        }
    }
}

// Geri alma stack işlemleri
void push_undo(int z1, int y1, int x1, Tile t1, int z2, int y2, int x2, Tile t2) {
    if (game.undo_ptr < 72) {
        game.undo_stack[game.undo_ptr++] = (UndoAction){z1, y1, x1, t1, z2, y2, x2, t2};
    }
}

bool pop_undo(int *z1, int *y1, int *x1, Tile *t1, int *z2, int *y2, int *x2, Tile *t2) {
    if (game.undo_ptr > 0) {
        game.undo_ptr--;
        *z1 = game.undo_stack[game.undo_ptr].z1;
        *y1 = game.undo_stack[game.undo_ptr].y1;
        *x1 = game.undo_stack[game.undo_ptr].x1;
        *t1 = game.undo_stack[game.undo_ptr].t1;
        *z2 = game.undo_stack[game.undo_ptr].z2;
        *y2 = game.undo_stack[game.undo_ptr].y2;
        *x2 = game.undo_stack[game.undo_ptr].x2;
        *t2 = game.undo_stack[game.undo_ptr].t2;
        return true;
    }
    return false;
}

// Şablon Maske Tanımlama (144 Taşlık Simetrik Piramit)
void init_layout_mask(bool mask[4][8][12]) {
    memset(mask, 0, sizeof(bool) * 4 * 8 * 12);
    
    // Kat 0 (Alt Kat - 80 taş)
    for (int x = 3; x <= 8; x++) mask[0][0][x] = true;
    for (int x = 1; x <= 10; x++) mask[0][1][x] = true;
    for (int y = 2; y <= 5; y++) {
        for (int x = 0; x <= 11; x++) mask[0][y][x] = true;
    }
    for (int x = 1; x <= 10; x++) mask[0][6][x] = true;
    for (int x = 3; x <= 8; x++) mask[0][7][x] = true;
    
    // Kat 1 (44 taş)
    for (int x = 3; x <= 8; x++) mask[1][1][x] = true;
    for (int y = 2; y <= 5; y++) {
        for (int x = 2; x <= 9; x++) mask[1][y][x] = true;
    }
    for (int x = 3; x <= 8; x++) mask[1][6][x] = true;
    
    // Kat 2 (16 taş)
    for (int y = 2; y <= 5; y++) {
        for (int x = 4; x <= 7; x++) mask[2][y][x] = true;
    }
    
    // Kat 3 (En Üst Kat - 4 taş)
    for (int y = 3; y <= 4; y++) {
        for (int x = 5; x <= 6; x++) mask[3][y][x] = true;
    }
}

// Deste oluşturma
void generate_deck(Tile deck[144]) {
    int idx = 0;
    // Noktalar
    for (int val = 1; val <= 9; val++) {
        for (int i = 0; i < 4; i++) {
            deck[idx] = (Tile){TYPE_DOTS, val, idx};
            idx++;
        }
    }
    // Bambular
    for (int val = 1; val <= 9; val++) {
        for (int i = 0; i < 4; i++) {
            deck[idx] = (Tile){TYPE_BAMBOO, val, idx};
            idx++;
        }
    }
    // Karakterler
    for (int val = 1; val <= 9; val++) {
        for (int i = 0; i < 4; i++) {
            deck[idx] = (Tile){TYPE_CHARS, val, idx};
            idx++;
        }
    }
    // Rüzgarlar
    for (int val = 1; val <= 4; val++) {
        for (int i = 0; i < 4; i++) {
            deck[idx] = (Tile){TYPE_WINDS, val, idx};
            idx++;
        }
    }
    // Ejderhalar
    for (int val = 1; val <= 3; val++) {
        for (int i = 0; i < 4; i++) {
            deck[idx] = (Tile){TYPE_DRAGONS, val, idx};
            idx++;
        }
    }
    // Mevsimler
    for (int val = 1; val <= 4; val++) {
        deck[idx] = (Tile){TYPE_SEASONS, val, idx};
        idx++;
    }
    // Çiçekler
    for (int val = 1; val <= 4; val++) {
        deck[idx] = (Tile){TYPE_FLOWERS, val, idx};
        idx++;
    }
}

// Yeni oyun başlatma
void init_game() {
    bool mask[4][8][12];
    init_layout_mask(mask);
    
    Tile deck[144];
    generate_deck(deck);
    
    // Karıştır (Fisher-Yates)
    for (int i = 143; i > 0; i--) {
        int j = rand() % (i + 1);
        Tile temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
    
    int tile_idx = 0;
    for (int z = 0; z < 4; z++) {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 12; x++) {
                if (mask[z][y][x]) {
                    game.board[z][y][x].tile = deck[tile_idx++];
                    game.board[z][y][x].x = x;
                    game.board[z][y][x].y = y;
                    game.board[z][y][x].z = z;
                    game.board[z][y][x].active = true;
                } else {
                    game.board[z][y][x].active = false;
                }
            }
        }
    }
    
    game.selected_z = game.selected_y = game.selected_x = -1;
    game.hint_z1 = game.hint_y1 = game.hint_x1 = -1;
    game.hint_z2 = game.hint_y2 = game.hint_x2 = -1;
    game.undo_ptr = 0;
    strcpy(game.status_log, "[Info] Game started! Good luck.");
    game.start_time = time(NULL);
    game.elapsed_seconds = 0;
    game.score = 0;
    game.has_won = false;
}

// Taş sembolünü alır
void get_tile_symbol(Tile t, char *buf) {
    switch (t.type) {
        case TYPE_DOTS:       sprintf(buf, "D%d", t.value); break;
        case TYPE_BAMBOO:     sprintf(buf, "B%d", t.value); break;
        case TYPE_CHARS:      sprintf(buf, "C%d", t.value); break;
        case TYPE_WINDS:
            if (t.value == 1) strcpy(buf, "WE");
            else if (t.value == 2) strcpy(buf, "WS");
            else if (t.value == 3) strcpy(buf, "WW");
            else strcpy(buf, "WN");
            break;
        case TYPE_DRAGONS:
            if (t.value == 1) strcpy(buf, "DR");
            else if (t.value == 2) strcpy(buf, "DG");
            else strcpy(buf, "DW");
            break;
        case TYPE_SEASONS:    sprintf(buf, "S%d", t.value); break;
        case TYPE_FLOWERS:    sprintf(buf, "F%d", t.value); break;
        default:              strcpy(buf, "??"); break;
    }
}

// Taş rengini alır
Color get_tile_color(Tile t) {
    switch (t.type) {
        case TYPE_DOTS:       return (Color){ 30, 120, 220, 255 };  // Mavi
        case TYPE_BAMBOO:     return (Color){ 40, 160, 60, 255 };   // Yeşil
        case TYPE_CHARS:      return (Color){ 200, 50, 50, 255 };   // Kırmızı
        case TYPE_WINDS:      return (Color){ 80, 80, 90, 255 };    // Koyu Gri
        case TYPE_DRAGONS:
            if (t.value == 1) return (Color){ 200, 50, 50, 255 };
            if (t.value == 2) return (Color){ 40, 160, 60, 255 };
            return (Color){ 100, 100, 110, 255 };
        case TYPE_SEASONS:    return (Color){ 220, 110, 20, 255 };  // Turuncu
        case TYPE_FLOWERS:    return (Color){ 210, 40, 150, 255 };  // Pembe
        default:              return BLACK;
    }
}

// Katman çizgi rengini alır
Color get_layer_border_color(int z) {
    switch (z) {
        case 0: return (Color){ 160, 160, 165, 255 }; // Kat 0: Gri
        case 1: return (Color){ 100, 180, 110, 255 }; // Kat 1: Yeşil
        case 2: return (Color){ 80, 150, 220, 255 };  // Kat 2: Mavi
        case 3: return (Color){ 230, 130, 40, 255 };  // Kat 3: Turuncu
        default: return DARKGRAY;
    }
}

// Buton çizimi ve kontrolü
bool DrawButton(GuiButton btn) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, btn.rect);
    Color col = hovered ? btn.hover_color : btn.color;
    
    DrawRectangleRounded(btn.rect, 0.2f, 4, col);
    DrawRectangleRoundedLines(btn.rect, 0.2f, 4, DARKGRAY);
    
    int text_width = MeasureText(btn.text, 18);
    int text_x = btn.rect.x + (btn.rect.width - text_width) / 2;
    int text_y = btn.rect.y + (btn.rect.height - 18) / 2;
    DrawText(btn.text, text_x, text_y, 18, WHITE);
    
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void DrawSingleShape(int x, int y, int shape_type, Color color) {
    switch (shape_type) {
        case 0: // Daire (Circle)
            DrawCircle(x, y, 5, color);
            DrawCircleLines(x, y, 5, ColorAlpha(BLACK, 0.4f));
            break;
        case 1: // Kare (Square)
            DrawRectangle(x - 5, y - 5, 10, 10, color);
            DrawRectangleLines(x - 5, y - 5, 10, 10, ColorAlpha(BLACK, 0.4f));
            break;
        case 2: // Üçgen (Triangle)
            DrawTriangle((Vector2){(float)x, (float)y - 6}, (Vector2){(float)x - 6, (float)y + 5}, (Vector2){(float)x + 6, (float)y + 5}, color);
            DrawTriangleLines((Vector2){(float)x, (float)y - 6}, (Vector2){(float)x - 6, (float)y + 5}, (Vector2){(float)x + 6, (float)y + 5}, ColorAlpha(BLACK, 0.4f));
            break;
        case 3: // Yıldız (Star - David Star)
            DrawTriangle((Vector2){(float)x, (float)y - 7}, (Vector2){(float)x - 6, (float)y + 3}, (Vector2){(float)x + 6, (float)y + 3}, color);
            DrawTriangle((Vector2){(float)x - 6, (float)y - 3}, (Vector2){(float)x + 6, (float)y - 3}, (Vector2){(float)x, (float)y + 7}, color);
            break;
        case 4: // Karo (Diamond)
            DrawTriangle((Vector2){(float)x, (float)y - 7}, (Vector2){(float)x - 6, (float)y}, (Vector2){(float)x + 6, (float)y}, color);
            DrawTriangle((Vector2){(float)x - 6, (float)y}, (Vector2){(float)x + 6, (float)y}, (Vector2){(float)x, (float)y + 7}, color);
            DrawTriangleLines((Vector2){(float)x, (float)y - 7}, (Vector2){(float)x - 6, (float)y}, (Vector2){(float)x + 6, (float)y}, ColorAlpha(BLACK, 0.4f));
            DrawTriangleLines((Vector2){(float)x - 6, (float)y}, (Vector2){(float)x + 6, (float)y}, (Vector2){(float)x, (float)y + 7}, ColorAlpha(BLACK, 0.4f));
            break;
        case 5: // Kalp (Heart)
            DrawCircle(x - 3, y - 2, 4, color);
            DrawCircle(x + 3, y - 2, 4, color);
            DrawTriangle((Vector2){(float)x - 7, (float)y - 1}, (Vector2){(float)x + 7, (float)y - 1}, (Vector2){(float)x, (float)y + 7}, color);
            break;
        case 6: // Çarpı (Cross)
            DrawLineEx((Vector2){(float)x - 5, (float)y - 5}, (Vector2){(float)x + 5, (float)y + 5}, 2.0f, color);
            DrawLineEx((Vector2){(float)x - 5, (float)y + 5}, (Vector2){(float)x + 5, (float)y - 5}, 2.0f, color);
            break;
    }
}

void DrawMultipleShapes(int cx, int cy, int count, int shape_type, Color color) {
    if (count == 1) {
        DrawSingleShape(cx, cy, shape_type, color);
    } else if (count == 2) {
        DrawSingleShape(cx, cy - 12, shape_type, color);
        DrawSingleShape(cx, cy + 12, shape_type, color);
    } else if (count == 3) {
        DrawSingleShape(cx - 10, cy - 12, shape_type, color);
        DrawSingleShape(cx, cy, shape_type, color);
        DrawSingleShape(cx + 10, cy + 12, shape_type, color);
    } else if (count == 4) {
        DrawSingleShape(cx - 10, cy - 12, shape_type, color);
        DrawSingleShape(cx + 10, cy - 12, shape_type, color);
        DrawSingleShape(cx - 10, cy + 12, shape_type, color);
        DrawSingleShape(cx + 10, cy + 12, shape_type, color);
    } else if (count == 5) {
        DrawSingleShape(cx - 11, cy - 13, shape_type, color);
        DrawSingleShape(cx + 11, cy - 13, shape_type, color);
        DrawSingleShape(cx - 11, cy + 13, shape_type, color);
        DrawSingleShape(cx + 11, cy + 13, shape_type, color);
        DrawSingleShape(cx, cy, shape_type, color);
    } else if (count == 6) {
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 2; c++) {
                DrawSingleShape(cx - 10 + c * 20, cy - 15 + r * 15, shape_type, color);
            }
        }
    } else if (count == 7) {
        DrawSingleShape(cx, cy - 16, shape_type, color);
        for (int r = 0; r < 2; r++) {
            for (int c = 0; c < 3; c++) {
                DrawSingleShape(cx - 12 + c * 12, cy - 2 + r * 14, shape_type, color);
            }
        }
    } else if (count == 8) {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 2; c++) {
                DrawSingleShape(cx - 10 + c * 20, cy - 18 + r * 12, shape_type, color);
            }
        }
    } else if (count == 9) {
        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                DrawSingleShape(cx - 12 + c * 12, cy - 14 + r * 14, shape_type, color);
            }
        }
    }
}

void DrawTileSymbol(int bx, int by, Tile t) {
    int cx = bx + tile_w / 2;
    int cy = by + tile_h / 2;
    
    int shape_type = 0;
    Color shape_color = BLACK;
    
    switch (t.type) {
        case TYPE_DOTS:
            shape_type = 0; // Daire
            shape_color = (Color){ 30, 120, 220, 255 }; // Mavi
            break;
        case TYPE_BAMBOO:
            shape_type = 1; // Kare
            shape_color = (Color){ 40, 160, 60, 255 }; // Yeşil
            break;
        case TYPE_CHARS:
            shape_type = 2; // Üçgen
            shape_color = (Color){ 200, 50, 50, 255 }; // Kırmızı
            break;
        case TYPE_WINDS:
            shape_type = 3; // Yıldız
            shape_color = (Color){ 220, 180, 40, 255 }; // Altın
            break;
        case TYPE_DRAGONS:
            shape_type = 4; // Karo
            shape_color = (Color){ 120, 120, 140, 255 }; // Gri
            break;
        case TYPE_SEASONS:
            shape_type = 5; // Kalp
            shape_color = (Color){ 220, 80, 80, 255 }; // Pembe/Kırmızı
            break;
        case TYPE_FLOWERS:
            shape_type = 6; // Çarpı
            shape_color = (Color){ 180, 60, 180, 255 }; // Mor
            break;
    }
    
    DrawMultipleShapes(cx, cy, t.value, shape_type, shape_color);
}

// Tek bir taşı çizen yardımcı fonksiyon
void DrawTileObject(int z, int y, int x, Tile tile, bool hovered, bool selected, bool is_hint) {
    int bx = start_x + x * spacing_x - z * 4;
    int by = start_y + y * spacing_y - z * 4;
    
    // 3D Gölge/Kalınlık katmanı (Yeşil taban)
    DrawRectangleRounded((Rectangle){ bx + 3, by + 3, tile_w, tile_h }, 0.15f, 4, (Color){ 30, 70, 50, 255 });
    
    // Taşın fildişi gövdesi
    DrawRectangleRounded((Rectangle){ bx, by, tile_w, tile_h }, 0.15f, 4, (Color){ 245, 245, 240, 255 });
    
    // Kenarlık
    Color border_color = get_layer_border_color(z);
    DrawRectangleRoundedLines((Rectangle){ bx, by, tile_w, tile_h }, 0.15f, 4, border_color);
    
    // Yanıp sönen ipucu efekti
    if (is_hint) {
        float time_sec = GetTime();
        if (((int)(time_sec * 4.0f) % 2) == 0) {
            DrawRectangleRoundedLines((Rectangle){ bx, by, tile_w, tile_h }, 0.15f, 4, GREEN);
        }
    }
    
    // Seçim efekti (yarı saydam mavi)
    if (selected) {
        DrawRectangleRounded((Rectangle){ bx, by, tile_w, tile_h }, 0.15f, 4, (Color){ 50, 150, 250, 100 });
    }
    
    // Üzerinde gezinme (hover) efekti
    if (hovered) {
        DrawRectangleRoundedLines((Rectangle){ bx, by, tile_w, tile_h }, 0.15f, 4, GOLD);
    }
    
    // Sembol çizimi (Grafiksel şekiller)
    DrawTileSymbol(bx, by, tile);
    
    // Küçük Kod Göstergesi (Sağ Üst Köşe)
    char sym[8];
    get_tile_symbol(tile, sym);
    DrawText(sym, bx + tile_w - MeasureText(sym, 10) - 5, by + 4, 10, DARKGRAY);
    
    // Katman numarası
    char z_str[2];
    sprintf(z_str, "%d", z);
    DrawText(z_str, bx + 5, by + 4, 10, DARKGRAY);
}

int main() {
    // Grafik penceresini oluştur
    InitWindow(1020, 720, "Mahjong Solitaire - Shanghai");
    SetTargetFPS(60);
    
    // Rastgele sayı üretecini başlat
    srand(time(NULL));
    
    // Buton şablonları
    GuiButton btn_hint = { { 845, 160, 140, 40 }, "HINT", (Color){ 50, 120, 220, 255 }, (Color){ 70, 140, 240, 255 } };
    GuiButton btn_undo = { { 845, 215, 140, 40 }, "UNDO", (Color){ 120, 80, 180, 255 }, (Color){ 140, 100, 200, 255 } };
    GuiButton btn_shuffle = { { 845, 270, 140, 40 }, "SHUFFLE", (Color){ 200, 100, 50, 255 }, (Color){ 220, 120, 70, 255 } };
    GuiButton btn_restart = { { 845, 325, 140, 40 }, "NEW GAME", (Color){ 40, 160, 100, 255 }, (Color){ 60, 180, 120, 255 } };
    GuiButton btn_exit = { { 845, 380, 140, 40 }, "EXIT", (Color){ 180, 50, 50, 255 }, (Color){ 200, 70, 70, 255 } };
    
    while (!WindowShouldClose()) {
        // --- 1. GÜNCELLEME DÖNGÜSÜ ---
        if (current_state == STATE_PLAYING) {
            game.elapsed_seconds = (int)(time(NULL) - game.start_time);
            
            // Galibiyet kontrolü
            if (get_remaining_tiles() == 0) {
                game.has_won = true;
                current_state = STATE_WON;
            }
        }
        
        // --- 2. ÇİZİM DÖNGÜSÜ ---
        BeginDrawing();
        ClearBackground((Color){ 24, 80, 48, 255 }); // Yeşil masa örtüsü rengi
        
        if (current_state == STATE_WELCOME) {
            // --- HOŞ GELDİNİZ EKRANI ---
            DrawText("MAHJONG SOLITAIRE", 1020 / 2 - MeasureText("MAHJONG SOLITAIRE", 40) / 2, 80, 40, GOLD);
            DrawText("Shanghai Pyramid Matching Game", 1020 / 2 - MeasureText("Shanghai Pyramid Matching Game", 20) / 2, 130, 20, WHITE);
            
            // Bilgi Kutusu
            Rectangle info_rec = { 180, 180, 660, 360 };
            DrawRectangleRounded(info_rec, 0.05f, 4, (Color){ 30, 30, 35, 220 });
            DrawRectangleRoundedLines(info_rec, 0.05f, 4, GOLD);
            
            int y_off = 210;
            DrawText("RULES:", 210, y_off, 20, GOLD); y_off += 35;
            DrawText("- Clear the board by matching pairs of identical free tiles.", 210, y_off, 16, WHITE); y_off += 25;
            DrawText("- A tile is 'free' if it has no tiles on top and is open on either left or right.", 210, y_off, 16, WHITE); y_off += 25;
            DrawText("- Seasons (Hearts) and Flowers (Crosses) match any tile of their own group.", 210, y_off, 16, WHITE); y_off += 25;
            DrawText("- Other suits (Circles, Squares, Triangles, Stars, Diamonds) must match exactly.", 210, y_off, 16, WHITE); y_off += 35;
            
            DrawText("CONTROLS:", 210, y_off, 20, GOLD); y_off += 35;
            DrawText("- Left Mouse Click: Select tiles and click buttons.", 210, y_off, 16, WHITE); y_off += 25;
            DrawText("- Tile layers are color-coded from level 0 to 3 (Gray to Orange).", 210, y_off, 16, WHITE);
            
            // Başla Butonu
            GuiButton btn_start = { { 1020 / 2 - 100, 570, 200, 50 }, "START GAME", (Color){ 40, 160, 100, 255 }, (Color){ 60, 180, 120, 255 } };
            if (DrawButton(btn_start)) {
                init_game();
                current_state = STATE_PLAYING;
            }
            
        } else if (current_state == STATE_PLAYING) {
            // --- OYUN EKRANI ---
            
            // Panel Bölgesi (Sağ Bölge)
            Rectangle sidebar_rec = { 825, 15, 180, 690 };
            DrawRectangleRounded(sidebar_rec, 0.03f, 4, (Color){ 30, 30, 35, 200 });
            DrawRectangleRoundedLines(sidebar_rec, 0.03f, 4, GRAY);
            
            DrawText("MAHJONG", 825 + (180 - MeasureText("MAHJONG", 28)) / 2, 40, 28, GOLD);
            DrawText("Shanghai", 825 + (180 - MeasureText("Shanghai", 18)) / 2, 75, 18, LIGHTGRAY);
            DrawLine(840, 110, 990, 110, GRAY);
            
            // Buton İşlevleri
            if (DrawButton(btn_hint)) {
                int z1, y1, x1, z2, y2, x2;
                int moves = get_available_moves(&z1, &y1, &x1, &z2, &y2, &x2);
                if (moves > 0) {
                    game.hint_z1 = z1; game.hint_y1 = y1; game.hint_x1 = x1;
                    game.hint_z2 = z2; game.hint_y2 = y2; game.hint_x2 = x2;
                    game.score = (game.score >= 50) ? game.score - 50 : 0;
                    
                    char s1[8];
                    get_tile_symbol(game.board[z1][y1][x1].tile, s1);
                    sprintf(game.status_log, "[Info] Hint: Matching tile type: %s (-50 Points)", s1);
                } else {
                    strcpy(game.status_log, "[Error] No matches left! Shuffle the board.");
                }
            }
            
            if (DrawButton(btn_undo)) {
                int z1, y1, x1, z2, y2, x2;
                Tile t1, t2;
                if (pop_undo(&z1, &y1, &x1, &t1, &z2, &y2, &x2, &t2)) {
                    game.board[z1][y1][x1].tile = t1;
                    game.board[z1][y1][x1].active = true;
                    game.board[z2][y2][x2].tile = t2;
                    game.board[z2][y2][x2].active = true;
                    game.score = (game.score >= 20) ? game.score - 20 : 0;
                    
                    char s1[8], s2[8];
                    get_tile_symbol(t1, s1);
                    get_tile_symbol(t2, s2);
                    sprintf(game.status_log, "[Info] Undid match: %s and %s", s1, s2);
                    
                    game.selected_z = game.selected_y = game.selected_x = -1;
                    game.hint_z1 = game.hint_y1 = game.hint_x1 = -1;
                    game.hint_z2 = game.hint_y2 = game.hint_x2 = -1;
                } else {
                    strcpy(game.status_log, "[Error] No moves to undo.");
                }
            }
            
            if (DrawButton(btn_shuffle)) {
                reshuffle_board();
                game.selected_z = game.selected_y = game.selected_x = -1;
                game.hint_z1 = game.hint_y1 = game.hint_x1 = -1;
                game.hint_z2 = game.hint_y2 = game.hint_x2 = -1;
                strcpy(game.status_log, "[Info] Remaining tiles shuffled.");
            }
            
            if (DrawButton(btn_restart)) {
                init_game();
            }
            
            if (DrawButton(btn_exit)) {
                break;
            }
            
            // İstatistik Göstergeleri
            Rectangle stat_rec = { 835, 450, 160, 230 };
            DrawRectangleRounded(stat_rec, 0.05f, 4, (Color){ 20, 20, 25, 230 });
            DrawRectangleRoundedLines(stat_rec, 0.05f, 4, GOLD);
            
            int rem = get_remaining_tiles();
            int moves = get_available_moves(NULL, NULL, NULL, NULL, NULL, NULL);
            int m = game.elapsed_seconds / 60;
            int s = game.elapsed_seconds % 60;
            
            int sy = 475;
            char buf[64];
            
            sprintf(buf, "SCORE: %d", game.score);
            DrawText(buf, 850, sy, 16, GOLD); sy += 35;
            
            sprintf(buf, "REMAIN: %d / 144", rem);
            DrawText(buf, 850, sy, 15, WHITE); sy += 35;
            
            sprintf(buf, "MATCHES: %d", moves);
            DrawText(buf, 850, sy, 15, (moves > 0) ? GREEN : RED); sy += 35;
            
            sprintf(buf, "TIME: %02d:%02d", m, s);
            DrawText(buf, 850, sy, 15, LIGHTGRAY); sy += 35;
            
            char sel_buf[32] = "SELECT: None";
            if (game.selected_z != -1) {
                char s_sym[8];
                get_tile_symbol(game.board[game.selected_z][game.selected_y][game.selected_x].tile, s_sym);
                sprintf(sel_buf, "SELECT: L%d-%s", game.selected_z, s_sym);
            }
            DrawText(sel_buf, 850, sy, 14, YELLOW);
            
            // Tahta Etkileşimi
            Vector2 m_pos = GetMousePosition();
            int hovered_z = -1, hovered_y = -1, hovered_x = -1;
            
            for (int temp_z = 3; temp_z >= 0; temp_z--) {
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 12; x++) {
                        if (game.board[temp_z][y][x].active) {
                            int bx = start_x + x * spacing_x - temp_z * 4;
                            int by = start_y + y * spacing_y - temp_z * 4;
                            Rectangle r = { bx, by, tile_w, tile_h };
                            
                            if (CheckCollisionPointRec(m_pos, r)) {
                                hovered_z = temp_z;
                                hovered_y = y;
                                hovered_x = x;
                                goto found_hover2;
                            }
                        }
                    }
                }
            }
            found_hover2:
            
            // Fare Tıklamaları
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && m_pos.x < 810) {
                if (hovered_z != -1) {
                    if (is_tile_free(hovered_z, hovered_y, hovered_x)) {
                        game.hint_z1 = game.hint_y1 = game.hint_x1 = -1;
                        game.hint_z2 = game.hint_y2 = game.hint_x2 = -1;
                        
                        if (game.selected_z == -1) {
                            game.selected_z = hovered_z;
                            game.selected_y = hovered_y;
                            game.selected_x = hovered_x;
                            
                            char s_name[8];
                            get_tile_symbol(game.board[hovered_z][hovered_y][hovered_x].tile, s_name);
                            sprintf(game.status_log, "[Info] Selected: L%d - %s. Find a match.", hovered_z, s_name);
                        } else {
                            if (game.selected_z == hovered_z && game.selected_y == hovered_y && game.selected_x == hovered_x) {
                                game.selected_z = game.selected_y = game.selected_x = -1;
                                strcpy(game.status_log, "[Info] Selection cleared.");
                            } else {
                                Tile t1 = game.board[game.selected_z][game.selected_y][game.selected_x].tile;
                                Tile t2 = game.board[hovered_z][hovered_y][hovered_x].tile;
                                
                                if (tiles_match(t1, t2)) {
                                    push_undo(game.selected_z, game.selected_y, game.selected_x, t1, 
                                              hovered_z, hovered_y, hovered_x, t2);
                                              
                                    game.board[game.selected_z][game.selected_y][game.selected_x].active = false;
                                    game.board[hovered_z][hovered_y][hovered_x].active = false;
                                    game.score += 100;
                                    
                                    char s1[8], s2[8];
                                    get_tile_symbol(t1, s1);
                                    get_tile_symbol(t2, s2);
                                    sprintf(game.status_log, "[Match] Removed %s and %s! (+100 Points)", s1, s2);
                                    
                                    game.selected_z = game.selected_y = game.selected_x = -1;
                                } else {
                                    strcpy(game.status_log, "[Error] Tiles do not match!");
                                    game.selected_z = game.selected_y = game.selected_x = -1;
                                }
                            }
                        }
                    } else {
                        strcpy(game.status_log, "[Error] Selected tile is blocked!");
                    }
                }
            }
            
            // Tahtayı Çiz ( Painters Algorithm )
            for (int z = 0; z < 4; z++) {
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 12; x++) {
                        if (game.board[z][y][x].active) {
                            bool is_h = (z == hovered_z && y == hovered_y && x == hovered_x);
                            bool is_s = (z == game.selected_z && y == game.selected_y && x == game.selected_x);
                            bool is_hi = ((z == game.hint_z1 && y == game.hint_y1 && x == game.hint_x1) ||
                                          (z == game.hint_z2 && y == game.hint_y2 && x == game.hint_x2));
                                          
                            DrawTileObject(z, y, x, game.board[z][y][x].tile, is_h, is_s, is_hi);
                        }
                    }
                }
            }
            
            // Log Bölgesi
            Rectangle log_rec = { start_x, 650, 730, 45 };
            DrawRectangleRounded(log_rec, 0.2f, 4, (Color){ 20, 20, 25, 230 });
            DrawRectangleRoundedLines(log_rec, 0.2f, 4, GRAY);
            
            Color log_col = LIGHTGRAY;
            if (strncmp(game.status_log, "[Match]", 7) == 0) log_col = GREEN;
            else if (strncmp(game.status_log, "[Error]", 7) == 0) log_col = RED;
            else if (strncmp(game.status_log, "[Info]", 6) == 0) log_col = SKYBLUE;
            
            DrawText(game.status_log, start_x + 15, 663, 16, log_col);
            
        } else if (current_state == STATE_WON) {
            // --- KAZANDINIZ EKRANI ---
            DrawText("CONGRATULATIONS!", 1020 / 2 - MeasureText("CONGRATULATIONS!", 50) / 2, 100, 50, GOLD);
            DrawText("You cleared the board and won!", 1020 / 2 - MeasureText("You cleared the board and won!", 22) / 2, 160, 22, WHITE);
            
            Rectangle win_rec = { 310, 220, 400, 220 };
            DrawRectangleRounded(win_rec, 0.05f, 4, (Color){ 30, 30, 35, 220 });
            DrawRectangleRoundedLines(win_rec, 0.05f, 4, GOLD);
            
            char s_buf[64];
            sprintf(s_buf, "Total Score: %d", game.score);
            DrawText(s_buf, 350, 260, 20, GOLD);
            
            int m = game.elapsed_seconds / 60;
            int s = game.elapsed_seconds % 60;
            sprintf(s_buf, "Time Elapsed: %02d:%02d", m, s);
            DrawText(s_buf, 350, 310, 20, WHITE);
            
            GuiButton btn_play_again = { { 320, 480, 170, 45 }, "PLAY AGAIN", (Color){ 40, 160, 100, 255 }, (Color){ 60, 180, 120, 255 } };
            GuiButton btn_win_exit = { { 530, 480, 170, 45 }, "EXIT", (Color){ 180, 50, 50, 255 }, (Color){ 200, 70, 70, 255 } };
            
            if (DrawButton(btn_play_again)) {
                init_game();
                current_state = STATE_PLAYING;
            }
            if (DrawButton(btn_win_exit)) {
                break;
            }
        }
        
        EndDrawing();
    }
    
    // Grafik motorunu temizle ve kapat
    CloseWindow();
    return 0;
}
