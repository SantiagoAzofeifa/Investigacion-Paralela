#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/* ── Parámetros de la imagen ───────────────────── */
#define WIDTH    1920
#define HEIGHT   1080
#define MAX_ITER 1000

/* ── Región del plano complejo a renderizar ─────── */
#define X_MIN -2.5
#define X_MAX  1.0
#define Y_MIN -1.1
#define Y_MAX  1.1

/* ─────────────────────────────────────────────────
   Calcula cuántas iteraciones tarda el punto (cx,cy)
   en escapar del conjunto de Mandelbrot.
───────────────────────────────────────────────── */
int mandelbrot(double cx, double cy) {
    double zx = 0.0, zy = 0.0;
    int iter = 0;
    while (zx*zx + zy*zy <= 4.0 && iter < MAX_ITER) {
        double tmp = zx*zx - zy*zy + cx;
        zy = 2.0 * zx * zy + cy;
        zx = tmp;
        iter++;
    }
    return iter;
}

/* ─────────────────────────────────────────────────
   Convierte iteraciones en color RGB con paleta suave
───────────────────────────────────────────────── */
void iter_to_color(int iter, unsigned char *r,
                             unsigned char *g,
                             unsigned char *b) {
    if (iter == MAX_ITER) { *r = *g = *b = 0; return; }
    double t = (double)iter / MAX_ITER;
    *r = (unsigned char)(9   * (1-t) * t*t*t * 255);
    *g = (unsigned char)(15  * (1-t)*(1-t) * t*t * 255);
    *b = (unsigned char)(8.5 * (1-t)*(1-t)*(1-t) * t * 255);
}

/* ─────────────────────────────────────────────────
   VERSION SECUENCIAL
───────────────────────────────────────────────── */
void render_sequential(unsigned char *image) {
    for (int py = 0; py < HEIGHT; py++) {
        for (int px = 0; px < WIDTH; px++) {
            double cx = X_MIN + (X_MAX - X_MIN) * px / (WIDTH  - 1);
            double cy = Y_MIN + (Y_MAX - Y_MIN) * py / (HEIGHT - 1);
            int iter = mandelbrot(cx, cy);
            unsigned char r, g, b;
            iter_to_color(iter, &r, &g, &b);
            int idx = (py * WIDTH + px) * 3;
            image[idx] = r; image[idx+1] = g; image[idx+2] = b;
        }
    }
}

/* ─────────────────────────────────────────────────
   VERSION PARALELA con OpenMP
   • parallel for  → divide filas entre hilos
   • schedule(dynamic, 4) → balance dinámico de carga
   • Sin critical ni reduction: cada píxel es independiente
───────────────────────────────────────────────── */
void render_parallel(unsigned char *image, int num_threads) {
    omp_set_num_threads(num_threads);

    #pragma omp parallel for schedule(dynamic, 4) \
        shared(image) default(none)
    for (int py = 0; py < HEIGHT; py++) {
        for (int px = 0; px < WIDTH; px++) {
            double cx = X_MIN + (X_MAX - X_MIN) * px / (WIDTH  - 1);
            double cy = Y_MIN + (Y_MAX - Y_MIN) * py / (HEIGHT - 1);
            int iter = mandelbrot(cx, cy);
            unsigned char r, g, b;
            iter_to_color(iter, &r, &g, &b);
            int idx = (py * WIDTH + px) * 3;
            image[idx] = r; image[idx+1] = g; image[idx+2] = b;
        }
    }
}

/* ─────────────────────────────────────────────────
   MAIN
───────────────────────────────────────────────── */
int main(int argc, char *argv[]) {
    int num_threads = 4;
    if (argc >= 2) num_threads = atoi(argv[1]);

    size_t total = WIDTH * HEIGHT * 3;
    unsigned char *img_seq = (unsigned char *)malloc(total);
    unsigned char *img_par = (unsigned char *)malloc(total);

    printf("Fractal de Mandelbrot — %dx%d px | MAX_ITER=%d\n",
           WIDTH, HEIGHT, MAX_ITER);
    printf("Hilos solicitados: %d\n\n", num_threads);

    /* Secuencial */
    double t0 = omp_get_wtime();
    render_sequential(img_seq);
    double t_seq = omp_get_wtime() - t0;

    /* Paralela */
    double t2 = omp_get_wtime();
    render_parallel(img_par, num_threads);
    double t_par = omp_get_wtime() - t2;

    /* Verificación */
    int errores = 0;
    for (size_t i = 0; i < total; i++)
        if (img_seq[i] != img_par[i]) errores++;

    /* Guardar imágenes */
    stbi_write_png("mandelbrot_secuencial.png", WIDTH, HEIGHT, 3, img_seq, WIDTH*3);
    stbi_write_png("mandelbrot_paralelo.png",   WIDTH, HEIGHT, 3, img_par, WIDTH*3);

    /* Reporte */
    double speedup    = t_seq / t_par;
    double mejora     = (1.0 - t_par / t_seq) * 100.0;
    double eficiencia = (speedup / num_threads) * 100.0;

    printf("===== RESULTADOS SECUENCIALES =====\n");
    printf("Tiempo secuencial  : %.4f s\n\n", t_seq);
    printf("===== RESULTADOS PARALELOS =====\n");
    printf("Tiempo paralelo    : %.4f s  (%d hilos)\n", t_par, num_threads);
    printf("Speedup            : %.2fx\n", speedup);
    printf("Mejora             : %.2f%%\n", mejora);
    printf("Eficiencia         : %.2f%%\n", eficiencia);
    printf("Pixeles incorrectos: %d (debe ser 0)\n", errores);
    printf("\nGuardado: mandelbrot_secuencial.png | mandelbrot_paralelo.png\n");

    free(img_seq);
    free(img_par);
    return 0;
}