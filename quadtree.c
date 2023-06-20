#include "quadtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>     /* OpenGL functions */
#endif

unsigned int first = 1;
char desenhaBorda = 1;
int originalWidth = 0;
int originalHeight = 0;

QuadNode* newNode(int x, int y, int width, int height)
{
    QuadNode* n = malloc(sizeof(QuadNode));
    n->x = x;
    n->y = y;
    n->width = width;
    n->height = height;
    n->NW = n->NE = n->SW = n->SE = NULL;
    n->color[0] = n->color[1] = n->color[2] = 0;
    n->id = first++;
    return n;
}

QuadNode* geraQuadtree(Img* pic, float minError)
{
    int width = pic->width;
    int height = pic->height;
    originalWidth = width;
    originalHeight = height;
    // Converte o vetor RGBPixel para uma MATRIZ que pode acessada por pixels[linha][coluna]
    RGBPixel (*pixels)[pic->width] = (RGBPixel(*)[pic->height]) pic->img;

    int grayPixels[originalWidth][originalHeight];
    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            // pixels[i][j] = pic->img[i * width + j];
            grayPixels[i][j] = (0.3 * pixels[i][j].r) + (0.59 * pixels[i][j].g) + (0.11 * pixels[i][j].b);
        }
    }

    QuadNode* raiz = newNode(0, 0, width, height);

    recursiveQuadtree(width, height, raiz, pixels, grayPixels, minError);

    return raiz;
}

void recursiveQuadtree(int width, int height, QuadNode* raiz, RGBPixel pixels[originalWidth][originalHeight], int grayPixels[originalWidth][originalHeight], float minError){
    int halfWidth = width / 2;
    int halfHeight = height / 2;

    long erroRegiao = calculaErroRegiao(raiz->x, raiz->y, width, height, grayPixels);
    RGBPixel corMedia = calculaCorMedia(raiz->x, raiz->y, width, height, pixels);

    raiz->color[0] = corMedia.r;
    raiz->color[1] = corMedia.g;
    raiz->color[2] = corMedia.b;

    if (minError < erroRegiao && halfHeight > 0 && halfWidth > 0){
        QuadNode* nw = newNode(raiz->x, raiz->y, halfWidth, halfHeight);
        QuadNode* ne = newNode(halfWidth + raiz->x, raiz->y, halfWidth, halfHeight);
        QuadNode* sw = newNode(raiz->x, halfHeight + raiz->y, halfWidth, halfHeight);
        QuadNode* se = newNode(halfWidth + raiz->x, halfHeight + raiz->y, halfWidth, halfHeight);

        raiz->NW = nw;
        raiz->NE = ne;
        raiz->SW = sw;
        raiz->SE = se;

        if (halfHeight == 1 || halfWidth == 1){
            raiz->NE->status = CHEIO;
            raiz->NW->status = CHEIO;
            raiz->SW->status = CHEIO;
            raiz->SE->status = CHEIO;
            // printf("red: %d, green: %d, blue: %d]\n", corMedia.r, corMedia.g, corMedia.b);
            return;
        }

        raiz->status = PARCIAL; 

        recursiveQuadtree(halfWidth, halfHeight, raiz->NW, pixels, grayPixels, minError);
        recursiveQuadtree(halfWidth, halfHeight, raiz->NE, pixels, grayPixels, minError);
        recursiveQuadtree(halfWidth, halfHeight, raiz->SW, pixels, grayPixels, minError);
        recursiveQuadtree(halfWidth, halfHeight, raiz->SE, pixels, grayPixels, minError);
    }
    else {
        raiz->status = CHEIO;
        raiz->NE = NULL;
        raiz->NW = NULL;
        raiz->SE = NULL;
        raiz->SW = NULL;
        return;
    }

}

long calculaErroRegiao(int x, int y, int width, int height, int grayPixels[originalWidth][originalHeight]){
    int histograma[256] = { 0 };
    
    for (int i = y; i < height + y; i++){
        for (int j = x; j < width + x; j++){
            histograma[grayPixels[i][j]]++;
        }
    }

    long totalPixelsRegiao = width * height;

    long somaIntensidade = 0;

    for (int i = 0; i < 256; i++){
        somaIntensidade += i * histograma[i];
    }

    long mediaIntensidade = somaIntensidade / totalPixelsRegiao;

    long erro = 0;
    for (int i = y; i < height + y; i++){
        for (int j = x; j < width + x; j++){
            erro += pow(grayPixels[i][j] - mediaIntensidade, 2);
        }
    }

    long erroFinal = erro / totalPixelsRegiao;
    erroFinal = sqrt(erroFinal);

    return erroFinal;
}

RGBPixel calculaCorMedia(int x, int y, int width, int height, RGBPixel pixels[originalWidth][originalHeight]){
   int somaRed = 0, somaGreen = 0, somaBlue = 0, qtdPixels = 0;

    // Calcula a soma das componentes R, G e B
    for (int i = y; i < height + y; i++) {
        for (int j = x; j < width + x; j++) {
            somaRed += pixels[i][j].r;
            somaGreen += pixels[i][j].g;
            somaBlue += pixels[i][j].b;
            qtdPixels++;
        }
    }
    // qtdPixels = width * height;

    int mediaRed = somaRed / qtdPixels;
    int mediaGreen = somaGreen / qtdPixels;
    int mediaBlue = somaBlue / qtdPixels;

    RGBPixel corMedia;
    corMedia.r = mediaRed;
    corMedia.g = mediaGreen;
    corMedia.b = mediaBlue;
    return corMedia;
}

// Limpa a memória ocupada pela árvore
void clearTree(QuadNode* n)
{
    if(n == NULL) return;
    if(n->status == PARCIAL)
    {
        clearTree(n->NE);
        clearTree(n->NW);
        clearTree(n->SE);
        clearTree(n->SW);
    }
    //printf("Liberando... %d - %.2f %.2f %.2f %.2f\n", n->status, n->x, n->y, n->width, n->height);
    free(n);
}

// Ativa/desativa o desenho das bordas de cada região
void toggleBorder() {
    desenhaBorda = !desenhaBorda;
    printf("Desenhando borda: %s\n", desenhaBorda ? "SIM" : "NÃO");
}

// Desenha toda a quadtree
void drawTree(QuadNode* raiz) {
    if(raiz != NULL)
        drawNode(raiz);
}

// Grava a árvore no formato do Graphviz
void writeTree(QuadNode* raiz) {
    FILE* fp = fopen("quad.dot", "w");
    fprintf(fp, "digraph quadtree {\n");
    if (raiz != NULL)
        writeNode(fp, raiz);
    fprintf(fp, "}\n");
    fclose(fp);
    printf("\nFim!\n");
}

void writeNode(FILE* fp, QuadNode* n)
{
    if(n == NULL) return;

    if(n->NE != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->NE->id);
    if(n->NW != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->NW->id);
    if(n->SE != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->SE->id);
    if(n->SW != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->SW->id);
    writeNode(fp, n->NE);
    writeNode(fp, n->NW);
    writeNode(fp, n->SE);
    writeNode(fp, n->SW);
}

// Desenha todos os nodos da quadtree, recursivamente
void drawNode(QuadNode* n)
{
    if(n == NULL) return;

    glLineWidth(0.1);

    if(n->status == CHEIO) {
        glBegin(GL_QUADS);
        glColor3ubv(n->color);
        glVertex2f(n->x, n->y);
        glVertex2f(n->x+n->width-1, n->y);
        glVertex2f(n->x+n->width-1, n->y+n->height-1);
        glVertex2f(n->x, n->y+n->height-1);
        glEnd();
    }

    else if(n->status == PARCIAL)
    {
        if(desenhaBorda) {
            glBegin(GL_LINE_LOOP);
            glColor3ubv(n->color);
            glVertex2f(n->x, n->y);
            glVertex2f(n->x+n->width-1, n->y);
            glVertex2f(n->x+n->width-1, n->y+n->height-1);
            glVertex2f(n->x, n->y+n->height-1);
            glEnd();
        }
        drawNode(n->NE);
        drawNode(n->NW);
        drawNode(n->SE);
        drawNode(n->SW);
    }
    // Nodos vazios não precisam ser desenhados... nem armazenados!
}

