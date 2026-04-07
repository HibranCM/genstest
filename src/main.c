#include <genesis.h>

// ====================== ESTRUTURAS (16-BIT) ======================
typedef struct { s16 x, y, z; } Point3D;
typedef struct { s16 x, y, z; } PointProj;
typedef struct { u16 indices[4]; u8 color; } Face;

// ====================== CONFIGURAÇÕES ======================
#define SCREEN_W      256
#define SCREEN_H      160
#define SCREEN_CX     128
#define SCREEN_CY     80
#define CUBE_SIZE     25
#define CUBE_DISTANCE 110

static u16 angX = 0, angY = 0, angZ = 0;
static u8  effect_mode = 1; 

// ====================== GEOMETRIA ======================
static const Point3D cube[8] = {
    {-CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE}, { CUBE_SIZE, -CUBE_SIZE,  CUBE_SIZE},
    { CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE}, {-CUBE_SIZE,  CUBE_SIZE,  CUBE_SIZE},
    {-CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE}, { CUBE_SIZE, -CUBE_SIZE, -CUBE_SIZE},
    { CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE}, {-CUBE_SIZE,  CUBE_SIZE, -CUBE_SIZE}
};

static const Face faces[6] = {
    {{0,1,2,3}, 2}, {{1,5,6,2}, 3}, {{5,4,7,6}, 4},
    {{4,0,3,7}, 5}, {{3,2,6,7}, 6}, {{4,5,1,0}, 7}
};

// ====================== MACRO DE ROTACÃO (16-BIT UNROLLED) ======================
#define ROTATE_VERTEX(idx) do { \
    s16 x = cube[idx].x, y = cube[idx].y, z = cube[idx].z; \
    s32 tx, ty, tz; \
    ty = (y * cx - z * sx) >> 6; \
    tz = (y * sx + z * cx) >> 6; \
    y = ty; z = tz; \
    tx = (x * cy + z * sy) >> 6; \
    tz = (-x * sy + z * cy) >> 6; \
    x = tx; z = tz; \
    tx = (x * cz - y * sz) >> 6; \
    ty = (x * sz + y * cz) >> 6; \
    x = tx; y = ty; \
    rotatedZ[idx] = z + CUBE_DISTANCE; \
    s16 zp = rotatedZ[idx]; \
    if (zp < 10) zp = 10; \
    proj[idx].x = (((s32)x * 180) / zp) + SCREEN_CX; \
    proj[idx].y = (((s32)y * 180) / zp) + SCREEN_CY; \
    proj[idx].z = zp; \
} while(0)

// ====================== MOTOR 3D ======================
static void drawCube(void) {
    PointProj proj[8];
    s16 rotatedZ[8];
    u16 order[6];
    s16 faceDepths[6];

    s16 cx = cosFix16(angX), sx = sinFix16(angX);
    s16 cy = cosFix16(angY), sy = sinFix16(angY);
    s16 cz = cosFix16(angZ), sz = sinFix16(angZ);

    ROTATE_VERTEX(0); ROTATE_VERTEX(1); ROTATE_VERTEX(2); ROTATE_VERTEX(3);
    ROTATE_VERTEX(4); ROTATE_VERTEX(5); ROTATE_VERTEX(6); ROTATE_VERTEX(7);

    for (u16 i = 0; i < 6; i++) {
        order[i] = i;
        faceDepths[i] = (proj[faces[i].indices[0]].z + proj[faces[i].indices[2]].z) >> 1;
    }
    
    for (u16 i = 0; i < 5; i++) {
        for (u16 j = i + 1; j < 6; j++) {
            if (faceDepths[order[i]] < faceDepths[order[j]]) {
                u16 t = order[i]; order[i] = order[j]; order[j] = t;
            }
        }
    }

    for (u16 i = 0; i < 6; i++) {
        const Face *f = &faces[order[i]];
        Vect2D_s16 p[4] = {
            {proj[f->indices[0]].x, proj[f->indices[0]].y},
            {proj[f->indices[1]].x, proj[f->indices[1]].y},
            {proj[f->indices[2]].x, proj[f->indices[2]].y},
            {proj[f->indices[3]].x, proj[f->indices[3]].y}
        };

        s32 cross = (s32)(p[1].x - p[0].x) * (s32)(p[2].y - p[0].y) - 
                    (s32)(p[1].y - p[0].y) * (s32)(p[2].x - p[0].x);
        
        if (cross <= 0) continue;

        bool safe = true;
        for (u16 v = 0; v < 4; v++) {
            if (p[v].x < 2 || p[v].x > 253 || p[v].y < 2 || p[v].y > 157) {
                safe = false; break;
            }
        }
        if (!safe) continue;

        if (effect_mode) {
            BMP_drawPolygon(p, 4, f->color);
        } else {
            Line l; l.col = f->color;
            for (u16 e = 0; e < 4; e++) {
                l.pt1 = p[e]; l.pt2 = p[(e + 1) % 4];
                BMP_drawLine(&l);
            }
        }
    }
}

// ====================== PROGRAMA PRINCIPAL ======================
int main(bool hardReset) {
    JOY_init();
    BMP_init(TRUE, BG_A, PAL1, FALSE);

    // Fundo AZUL CLARO 100% puro para não ter erro de conversão
    u16 palette[16] = {
        RGB24_TO_VDPCOLOR(0x0000FF), RGB24_TO_VDPCOLOR(0xFFFFFF),
        RGB24_TO_VDPCOLOR(0xFF0000), RGB24_TO_VDPCOLOR(0x00FF00),
        RGB24_TO_VDPCOLOR(0x0000FF), RGB24_TO_VDPCOLOR(0xFFFF00),
        RGB24_TO_VDPCOLOR(0xFF00FF), RGB24_TO_VDPCOLOR(0x00FFFF)
    };
    PAL_setPalette(PAL1, palette, CPU);

    BMP_clear(); BMP_flip(1);
    BMP_clear(); BMP_flip(1);

    while (1) {
        // FORÇA a atualização do estado físico dos controles
        JOY_update();
        
        u16 joy = JOY_readJoypad(JOY_1);

        // Mapeamento à prova de conflitos (D-pad + A/B/C)
        if (joy & BUTTON_UP)    angX = (angX - 10) & 1023;
        if (joy & BUTTON_DOWN)  angX = (angX + 10) & 1023;
        if (joy & BUTTON_LEFT)  angY = (angY - 10) & 1023;
        if (joy & BUTTON_RIGHT) angY = (angY + 10) & 1023;
        if (joy & BUTTON_A)     angZ = (angZ - 8)  & 1023;
        if (joy & BUTTON_B)     angZ = (angZ + 8)  & 1023;
        
        if (joy & BUTTON_C) {
            effect_mode = !effect_mode;
            for(volatile u16 wait=0; wait<5000; wait++);
        }

        BMP_clear();
        drawCube();
        BMP_showFPS(1, 1, 1);
        BMP_flip(1);
    }
    return 0;
}