#include "gl.h"	
#include <conio.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dos.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_SIMD
typedef int int32_t;
typedef unsigned int uint32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
#include "stbimage.hpp"

#define XRES 640
#define YRES 480

typedef struct ImageLayer {
        int width, height;
        unsigned int *data;
        int x0,y0,x1,y1;
} ImageLayer;

bool loadPNG(ImageLayer *layer, const char *fileName) {
  int channels;
  layer->data = (uint32_t*)stbi_load(fileName, &layer->width, &layer->height, &channels, 4);
  if (layer->data==NULL) {printf("Missing file:%s\n",fileName);exit(0);}
  return true;
}

class OBJVertex {
public:
  float x,y,z;
};

class OBJNormal {
public:
  float x,y,z;
};

class OBJTexCoord {
public:
  float x,y;
};

class OBJColor {
public:
  float r,g,b,a;
};

class OBJFace {
public:
  int v[4];
  int n[4];
  int t[4];
  int c[4];
};

class OBJGroup {
public:
  OBJGroup() {
    firstFace = 0;
    faceCount = 0;
    textureWidth = 0;
    textureHeight = 0;
    textureData = NULL;
    glTextureId = 0;
  }
  ~OBJGroup() {
    if (textureData != NULL) 
      delete[] textureData;
    if (glTextureId != 0)
      glDeleteTextures(1,&glTextureId);
  }
  int firstFace;
  int faceCount;
  int textureWidth;
  int textureHeight;
  unsigned int *textureData;
  unsigned int glTextureId;
};

class OBJ {
public:
  OBJ() {
    groups = NULL;
    vertices = NULL;
    normals = NULL;
    texCoords = NULL;
    colors = NULL;
    faces = NULL;
    groupCount = 0;
    vertexCount = 0;
    normalCount = 0;
    texCoordCount = 0;
    colorCount = 0;
    faceCount = 0;
  }
  ~OBJ() {
    if (groups != NULL) delete[] groups;
    if (vertices != NULL) delete[] vertices;
    if (normals != NULL) delete[] normals;
    if (texCoords != NULL) delete[] texCoords;
    if (colors != NULL) delete[] colors;
    if (faces != NULL) delete[] faces;
  }
  OBJGroup *groups;
  OBJVertex *vertices;
  OBJNormal *normals;
  OBJTexCoord *texCoords;
  OBJColor *colors;
  OBJFace *faces;
  int groupCount;
  int vertexCount;
  int normalCount;
  int texCoordCount;
  int colorCount;
  int faceCount;
};

OBJ *loadOBJ(const char *fileName) {
  FILE *in = fopen(fileName,"r");
  if (in == NULL) return NULL;
  char buffer[1024];
  int vertexCount = 0;
  int normalCount = 0;
  int textureCount = 0;
  int colorCount = 0;
  int groupCount = 0;
  int faceCount = 0;
  while(fgets(buffer,1024,in)) {
    if (strlen(buffer)>=3) {
      if (buffer[0]=='v') {
        if (buffer[1]==' ') vertexCount++;
        if (buffer[1]=='c'&&buffer[2]==' ') colorCount++;
        if (buffer[1]=='n'&&buffer[2]==' ') normalCount++;
        if (buffer[1]=='t'&&buffer[2]==' ') textureCount++;
      }
      if (buffer[0]=='g' && buffer[1]==' ') groupCount++;
      if (buffer[0]=='f' && buffer[1]==' ') faceCount++;
    }
  }
  OBJ *o = new OBJ();
  if (groupCount>0) o->groups = new OBJGroup[groupCount];
  if (vertexCount>0) o->vertices = new OBJVertex[vertexCount];
  if (colorCount>0) o->colors = new OBJColor[colorCount];
  if (normalCount>0) o->normals = new OBJNormal[normalCount];
  if (textureCount>0) o->texCoords = new OBJTexCoord[textureCount];
  if (faceCount>0) o->faces = new OBJFace[faceCount];
  o->groupCount = groupCount;
  o->vertexCount = vertexCount;
  o->normalCount = normalCount;
  o->texCoordCount = textureCount;
  o->colorCount = colorCount;
  o->faceCount = faceCount;
  fseek(in,0,SEEK_SET);
  int vertex = 0;
  int normal = 0;
  int texture = 0;
  int color = 0;
  int group = 0;
  int face = 0;
  char mtllibname[256];
  while(fgets(buffer,1024,in)) {
    if (strlen(buffer)>=3) {
      if (buffer[0]=='v') {
        if (buffer[1]==' ') {sscanf(buffer,"v %f %f %f",&(o->vertices[vertex].x),&(o->vertices[vertex].y),&(o->vertices[vertex].z));vertex++;}
        if (buffer[1]=='c'&&buffer[2]==' ') color++;
        if (buffer[1]=='n'&&buffer[2]==' ') {sscanf(buffer,"vn %f %f %f",&(o->normals[normal].x),&(o->normals[normal].y),&(o->normals[normal].z));normal++;}
        if (buffer[1]=='t'&&buffer[2]==' ') {sscanf(buffer,"vt %f %f",&(o->texCoords[texture].x),&(o->texCoords[texture].y));texture++;}
      }
      if (buffer[0]=='g' && buffer[1]==' ') {o->groups[group].firstFace=face;if (group>0) o->groups[group-1].faceCount=face-o->groups[group-1].firstFace; group++;}
      if (buffer[0]=='f' && buffer[1]==' ') {
        sscanf(buffer,"f %d/%d/%d %d/%d/%d %d/%d/%d"
          ,&(o->faces[face].v[0]),&(o->faces[face].t[0]),&(o->faces[face].n[0])
          ,&(o->faces[face].v[1]),&(o->faces[face].t[1]),&(o->faces[face].n[1])
          ,&(o->faces[face].v[2]),&(o->faces[face].t[2]),&(o->faces[face].n[2])
        );
        face++;
      }
      if (buffer[0]=='m' && buffer[1]=='t' && buffer[2]=='l') {
        sscanf(buffer,"mtllib %s",mtllibname);
      }
      if (buffer[0]=='u' && buffer[1]=='s' && buffer[2]=='e') {
        char mtlname[256];
        char mtlname2[256];
        sscanf(buffer,"usemtl %s",mtlname);
        sprintf(mtlname2,"TEXTURES\\%s.png",mtlname);
        ImageLayer l;
        loadPNG(&l,mtlname2);
        if (l.width>0&&l.height>0) {
#define TS 512
          unsigned int *t = new unsigned int[TS*TS];
          for (int x = 0; x < TS; x++)
            for (int y = 0; y < TS; y++)
              t[x+y*TS]=l.data[x*l.width/TS+y*l.height/TS*l.width];
          delete[] l.data;
          o->groups[group-1].textureWidth = TS;
          o->groups[group-1].textureHeight = TS;
          o->groups[group-1].textureData = t;
          glGenTextures(1,&(o->groups[group-1].glTextureId));
          glBindTexture(GL_TEXTURE_2D, o->groups[group-1].glTextureId);
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, TS, TS, 0, GL_RGBA, GL_UNSIGNED_BYTE, t);
          glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
          delete[] t;
        }
      }
    }
  }
  if (group>0) o->groups[group-1].faceCount=face-o->groups[group-1].firstFace;
  fclose(in);
  return o;
}

float frand() {
  return (float)rand()/RAND_MAX;
}

float tme = 0.f;

float h(int x, int y) {
  return sin((x+tme*3.f)/5.f+sin(x*0.75)*0.1)*cos((y-tme*2.f)/10.f+sin(y*0.75)*0.1)*0.5+0.5;
}

float h2(int x, int y) {
  return (h(x+1,y)-h(x-1,y))*20+(h(x,y+1)-h(x,y-1))*20+2.f;
}

void paintOBJ(OBJ *o) {
  for (int j = 0; j < o->groupCount; j++) {
    if (o->groups[j].glTextureId!=0)
      glBindTexture(GL_TEXTURE_2D, o->groups[j].glTextureId);
    glBegin(GL_TRIANGLES);
    for (int i = o->groups[j].firstFace; i < o->groups[j].firstFace+o->groups[j].faceCount; i++) {
      OBJFace *f = &(o->faces[i]);

      glTexCoord2f(o->texCoords[f->t[0]-1].x,o->texCoords[f->t[0]-1].y);
      glNormal3f(o->normals[f->n[0]-1].x,o->normals[f->n[0]-1].y,o->normals[f->n[0]-1].z);
      glVertex3f(o->vertices[f->v[0]-1].x,o->vertices[f->v[0]-1].y,o->vertices[f->v[0]-1].z);

      glTexCoord2f(o->texCoords[f->t[1]-1].x,o->texCoords[f->t[1]-1].y);
      glNormal3f(o->normals[f->n[1]-1].x,o->normals[f->n[1]-1].y,o->normals[f->n[1]-1].z);
      glVertex3f(o->vertices[f->v[1]-1].x,o->vertices[f->v[1]-1].y,o->vertices[f->v[1]-1].z);

      glTexCoord2f(o->texCoords[f->t[2]-1].x,o->texCoords[f->t[2]-1].y);
      glNormal3f(o->normals[f->n[2]-1].x,o->normals[f->n[2]-1].y,o->normals[f->n[2]-1].z);
      glVertex3f(o->vertices[f->v[2]-1].x,o->vertices[f->v[2]-1].y,o->vertices[f->v[2]-1].z);
    }
    glEnd();
  }
}

int main(int argc, char **argv) {
  OBJ *girl = loadOBJ("_girl.obj");

  const bool pureVGA = false;
  if (pureVGA) {
    glVGA();
  } else {
    if (!glVesa(XRES,YRES,32)) {
      printf("No Vesa %dx%dx32 found.\n", XRES, YRES);
      sleep(1);
      if (!glVGA())
      {
        printf("No VGA device found.\n");
        exit(0);
      }
    }
  }

  //glEnable(GL_TEXTURE_2D);
  //glEnable(GL_CULL_FACE);
  //glEnable(GL_BLEND);
  //glBlendFunc(GL_ONE,GL_ONE);
  //glBlendEquation(GL_FUNC_ADD);
  //glCullFace(GL_BACK);
  //glDepthFunc(GL_ALWAYS);
/*
  unsigned int tex;
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
  float borderColor[] = {1.f,0.f,1.f,1.f};
  glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,borderColor);
#define texx 32
#define texy 32
  unsigned int *t = new unsigned int[texx*texy];
  for (int y = 0; y < texy; y++) {
    for (int x = 0; x < texx; x++) {
      float d = sqrt(x*x/10+y*y/10); 
      t[x+y*texx] = 0x80+d*200*200*100+d*200+d*200*200;
    }
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texx, texy, 0, GL_RGBA, GL_UNSIGNED_BYTE, t);
*/
  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
  glBlendEquation(GL_FUNC_ADD);

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHT1);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);
  do
  {		
    tme = (float)clock() / CLOCKS_PER_SEC;
    glClearColor(0.2f,0.4f,0.6f,0.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(90.0, (double)16.0/(double)9.0, 0.01, 1000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    float r = 0.75;
    gluLookAt(sin(tme)*r,sin(tme*0.3)+1,-cos(tme)*r,0,1,0,0,1,0);
    float pos[4] = {0,-1,1,0};
    float pos2[4] = {1,0,0,1};
    float spc[4] = {1,1,1,0};
    float dif[4] = {1,0.6,0.2,0};
    glLightfv(GL_LIGHT0, GL_POSITION,pos);
    glLightfv(GL_LIGHT0, GL_SPECULAR,spc);
    glLightfv(GL_LIGHT1, GL_POSITION,pos2);
    glLightfv(GL_LIGHT1, GL_DIFFUSE,dif);
    glMaterialfv(0, GL_SPECULAR,spc);
    glMaterialfv(0, GL_DIFFUSE,dif);
    for (int i = 0; i < 5*0+1; i++) {
      paintOBJ(girl);
      glTranslatef(2,0,0);
      glRotatef(-40.f,0,1,0);
    }
    /*
    glBegin(GL_QUADS);
    const int SEGX = 40;
    const int SEGY = 40;
    for (int x = -SEGX; x < SEGX; x++) {
      for (int y = -SEGY; y < SEGY; y++) {
        float xp[4],yp[4],zp[4],cp[4], tx[4], ty[4];
        for (int y2 = 0; y2 < 2; y2++) {
          for (int x2 = 0; x2 < 2; x2++) {
            cp[x2+y2*2] = h(x+x2,y+y2);
            xp[x2+y2*2] = (x+x2)*80;
            yp[x2+y2*2] = -cp[x2+y2*2]*400;
            zp[x2+y2*2] = (y+y2)*40+tme*10.f;
            tx[x2+y2*2] = (x+x2)/10.f;
            ty[x2+y2*2] = (y+y2)/10.f;
          }
        }
        float c; 
        c = h2(x,y)*0.5+0.5;
        glColor3f(c*0.4,c*0.5,c*0.6);
        glTexCoord2f(tx[0],ty[0]);
        glVertex3f(xp[0],yp[0],zp[0]);
        c = h2(x+1,y)*0.5+0.5;
        glColor3f(c*0.4,c*0.5,c*0.6);
        glTexCoord2f(tx[1],ty[1]);
        glVertex3f(xp[1],yp[1],zp[1]);
        c = h2(x+1,y+1)*0.5+0.5;
        glColor3f(c*0.4,c*0.5,c*0.6);
        glTexCoord2f(tx[3],ty[3]);
        glVertex3f(xp[3],yp[3],zp[3]);
        c = h2(x,y+1)*0.5+0.5;
        glColor3f(c*0.4,c*0.5,c*0.6);
        glTexCoord2f(tx[2],ty[2]);
        glVertex3f(xp[2],yp[2],zp[2]);
      }
    }
    glEnd();      
    */
    glRefresh();
  } while((inp(0x60)&0x80)==0x80);
  glDone();
  return 0;
}