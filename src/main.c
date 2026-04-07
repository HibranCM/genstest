#include <genesis.h>

// Estruturas Básicas
typedef struct { fix32 x, y, z; } Point3D;
typedef struct { s16 x, y; fix32 z; } PointProj;
typedef struct { u16 indices[4]; u8 color; } Face;

// Configurações de Tela e Geometria
#define SCREEN_W 256
#define SCREEN_H 160
#define SCREEN_CX 128
#define SCREEN_CY 80
#define CUBE_SIZE FIX32(25)
#define CUBE_DISTANCE FIX32(110)

static u16 angX = 0, angY = 0, angZ = 0;
static u8 effect_mode = 1; // 1: Sólido, 0: Wireframe

static void drawCube(const Point3D world[8], const Face faces[6]) {
    PointProj proj[8];
    fix32 rotatedZ[8];
    u16 order[6];
    fix32 faceDepths[6];

    // Leitura direta das LUTs nativas do SGDK
    fix32 cx = cosFix32(angX), sx = sinFix32(angX);
    fix32 cy = cosFix32(angY), sy = sinFix32(angY);
    fix32 cz = cosFix32(angZ), sz = sinFix32(angZ);

    // Loop Unrolling: Macro de Rotação e Projeção
    #define ROTATE_VERTEX(idx) do { \
        fix32 x = world[idx].x, y = world[idx].y, z = world[idx].z; \
        fix32 tx, ty, tz; \
        \
        ty = F32_mul(y, cx) - F32_mul(z, sx); \
        tz = F32_mul(y, sx) + F32_mul(z, cx); \
        y = ty; z = tz; \
        \
        tx = F32_mul(x, cy) + F32_mul(z, sy); \
        tz = F32_mul(-x, sy) + F32_mul(z, cy); \
        x = tx; z = tz; \
        \
        tx = F32_mul(x, cz) - F32_mul(y, sz); \
        ty = F32_mul(x, sz) + F32_mul(y, cz); \
        x = tx; y = ty; \
        \
        rotatedZ[idx] = z + CUBE_DISTANCE; \
        fix32 zp = rotatedZ[idx]; \
        if (zp < FIX32(10)) zp = FIX32(10); \
        \
        fix32 f = F32_div(FIX32(160), zp); \
        proj[idx].x = F32_toInt(F32_mul(x, f)) + SCREEN_CX; \
        proj[idx].y = F32_toInt(F32_mul(y, f)) + SCREEN_CY; \
        proj[idx].z = zp; \
    } while(0)

    // Execução sequencial para economia de ciclos (Branching reduction)
    ROTATE_VERTEX(0);
    ROTATE_VERTEX(1);
    ROTATE_VERTEX(2);
    ROTATE_VERTEX(3);
    ROTATE_VERTEX(4);
    ROTATE_VERTEX(5);
    ROTATE_VERTEX(6);
    ROTATE_VERTEX(7);

    // Z-Sorting (Painter's Algorithm)
    for (u16 i = 0; i < 6; i++) {
        order[i] = i;
        faceDepths[i] = (proj[faces[i].indices[0]].z + proj[faces[i].indices[2]].z) >> 1;
    }
    
    // Bubble Sort simples 
    for (u16 i = 0; i < 5; i++) {
        for (u16 j = i + 1; j < 6; j++) {
            if (faceDepths[order[i]] < faceDepths[order[j]]) {
                u16 t = order[i]; order[i] = order[j]; order[j] = t;
            }
        }
    }

    // Renderização com Anti-Wrap e Backface Culling
    for (u16 i = 0; i < 6; i++) {
        const Face *f = &faces[order[i]];
        Vect2D_s16 p[4] = {
            {proj[f->indices[0]].x, proj[f->indices[0]].y},
            {proj[f->indices[1]].x, proj[f->indices[1]].y},
            {proj[f->indices[2]].x, proj[f->indices[2]].y},
            {proj[f->indices[3]].x, proj[f->indices[3]].y}
        };

        // Backface Culling 
        s32 cross = (s32)(p[1].x - p[0].x) * (s32)(p[2].y - p[0].y) - 
                    (s32)(p[1].y - p[0].y) * (s32)(p[2].x - p[0].x);
        
        if (cross > 0) {
            // Trava Anti-Wrap
            bool safe = true;
            for(u16 v=0; v<4; v++) {
                if(p[v].x < 2 || p[v].x > 253 || p[v].y < 2 || p[v].y > 157) {
                    safe = false; break;
                }
            }

            if (safe) {
                if (effect_mode) {
                    BMP_drawPolygon(p, 4, f->color);
                } else {
                    Line l; l.col = f->color;
                    for(u16 e=0; e<4; e++) {
                        l.pt1 = p[e]; l.pt2 = p[(e+1)%4];
                        BMP_drawLine(&l);
                    }
                }
            }
        }
    }
}

int main(bool hardReset) {
    JOY_init();
    BMP_init(TRUE, BG_A, PAL1, FALSE);

    // Definição das Cores
    u16 palette[16];
    palette[0] = RGB24_TO_VDPCOLOR(0x000000); 
    palette[1] = RGB24_TO_VDPCOLOR(0xFFFFFF); 
    palette[2] = RGB24_TO_VDPCOLOR(0xFF0000); 
    palette[3] = RGB24_TO_VDPCOLOR(0x00FF00); 
    palette[4] = RGB24_TO_VDPCOLOR(0x0000FF); 
    palette[5] = RGB24_TO_VDPCOLOR(0xFFFF00); 
    palette[6] = RGB24_TO_VDPCOLOR(0xFF00FF); 
    palette[7] = RGB24_TO_VDPCOLOR(0x00FFFF); 
    PAL_setPalette(PAL1, palette, CPU);

    // Geometria
    Point3D cube[8] = {
        {-CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE}, {CUBE_SIZE, -CUBE_SIZE, CUBE_SIZE},
        {CUBE_SIZE, CUBE_SIZE, CUBE_SIZE}, {-CUBE_SIZE, CUBE_SIZE, CUBE_SIZE},
        {-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE}, {CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
        {CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE}, {-CUBE_SIZE, CUBE_SIZE, -CUBE_SIZE}
    };
    Face faces[6] = {
        {{0,1,2,3}, 2}, {{1,5,6,2}, 3}, {{5,4,7,6}, 4},
        {{4,0,3,7}, 5}, {{3,2,6,7}, 6}, {{4,5,1,0}, 7}
    };

    // Sincronização inicial do buffer duplo
    BMP_clear(); BMP_flip(1);
    BMP_clear(); BMP_flip(1);

    while (1) {
        angX = (angX + 2) & 1023;
        angY = (angY + 3) & 1023;

        u16 joy = JOY_readJoypad(JOY_1);
        if (joy & BUTTON_UP)    angX = (angX - 10) & 1023;
        if (joy & BUTTON_DOWN)  angX = (angX + 10) & 1023;
        if (joy & BUTTON_LEFT)  angY = (angY - 10) & 1023;
        if (joy & BUTTON_RIGHT) angY = (angY + 10) & 1023;
        if (joy & BUTTON_X)     angZ = (angZ - 8) & 1023;
        if (joy & BUTTON_Z)     angZ = (angZ + 8) & 1023;
        if (joy & BUTTON_A)     effect_mode = 0; 
        if (joy & BUTTON_B)     effect_mode = 1; 

        BMP_clear();
        drawCube(cube, faces);
        BMP_showFPS(1, 1, 1);
        BMP_flip(1); 
    }
    return 0;
}