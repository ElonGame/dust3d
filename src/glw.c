#include <OpenGL/gl.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <CoreFoundation/CoreFoundation.h>
#include "glw_internal.h"
#include "glw_style.h"
#include "icons.h"
#include "lodepng.h"

#ifndef M_PI
#define M_PI 3.14159265
#endif

#define glwSwapColor(color1, color2) do {\
  unsigned int tmp = color1;\
  color1 = color2;\
  color2 = tmp;\
} while (0)

void glwDrawSystemText(glwWin *win, int x, int y, char *text,
    unsigned int color) {
  int vleft, vwidth;
  float tleft, twidth;
  glwSystemFontTexture *systemFontTexture = glwGetSystemFontTexture(win);
  
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, systemFontTexture->texId);
  glColor3f(glwR(color), glwG(color), glwB(color));
  for (vleft = 0; *text; vleft += vwidth, text++) {
    int idx = (*text) - GLW_SYSTEM_FONT_BEGIN_CHAR;
    if (idx < 0 || idx > GLW_SYSTEM_FONT_CHAR_NUM) {
      continue;
    }
    tleft = (float)systemFontTexture->lefts[idx] /
      systemFontTexture->size.width;
    vwidth = systemFontTexture->widths[idx];
    twidth = (float)vwidth / systemFontTexture->originSize.width;
    
    glBegin(GL_QUADS);
      glTexCoord2f(tleft, 0);
      glVertex2i(x + vleft, y);
    
      glTexCoord2f(tleft + twidth, 0);
      glVertex2i(x + vleft + vwidth, y);
    
      glTexCoord2f(tleft + twidth, 1);
      glVertex2i(x + vleft + vwidth, y + systemFontTexture->originSize.height);
    
      glTexCoord2f(tleft, 1);
      glVertex2i(x + vleft, y + systemFontTexture->originSize.height);
    glEnd();
  }
  glDisable(GL_TEXTURE_2D);
}

static int glwGetLineHeight(glwWin *win) {
  glwSystemFontTexture *systemFontTexture = glwGetSystemFontTexture(win);
  return systemFontTexture->originSize.height * (1 + GLW_VER_AUTO_MARGIN * 2);
}

void glwDrawSystemIcon(glwWin *win, int x, int y, int icon,
    unsigned int color) {
  glwWinContext *ctx = glwGetWinContext(win);
  float texLeft, texWidth, texTop, texHeight;
  icon--;
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, ctx->iconTexId);
  glColor3f(glwR(color), glwG(color), glwB(color));
  texLeft = (float)iconTable[icon][ICON_ITEM_LEFT] / ICON_IMAGE_WIDTH;
  texTop = (float)iconTable[icon][ICON_ITEM_TOP] / ICON_IMAGE_WIDTH;
  texWidth = (float)iconTable[icon][ICON_ITEM_WIDTH] / ICON_IMAGE_WIDTH;
  texHeight = (float)iconTable[icon][ICON_ITEM_HEIGHT] / ICON_IMAGE_WIDTH;
  x += iconTable[icon][ICON_ITEM_TRIM_OFFSET_LEFT];
  y += (iconTable[icon][ICON_ITEM_TRIM_OFFSET_TOP] +
    (glwGetLineHeight(win) - iconTable[icon][ICON_ITEM_ORIGINAL_HEIGHT]) / 4);
  glBegin(GL_QUADS);
    glTexCoord2f(texLeft, texTop);
    glVertex2i(x, y);
  
    glTexCoord2f(texLeft + texWidth, texTop);
    glVertex2i(x + iconTable[icon][ICON_ITEM_WIDTH], y);
  
    glTexCoord2f(texLeft + texWidth, texTop + texHeight);
    glVertex2i(x + iconTable[icon][ICON_ITEM_WIDTH],
      y + iconTable[icon][ICON_ITEM_HEIGHT]);
  
    glTexCoord2f(texLeft, texTop + texHeight);
    glVertex2i(x,
      y + iconTable[icon][ICON_ITEM_HEIGHT]);
  glEnd();
  glDisable(GL_TEXTURE_2D);
}

void glwMouseEvent(glwWin *win, int button, int state, int x, int y) {
  glwImGui *imGUI = glwGetImGUI(win);
  imGUI->mouseButton = button;
  imGUI->mouseState = state;
  imGUI->mouseX = x;
  imGUI->mouseY = y;
}

int glwPointTest(int x, int y, int left, int top, int width, int height,
    int allowOffset) {
  int right = left + width + allowOffset;
  int bottom = top + height + 5 + allowOffset;
  left -= allowOffset;
  top -= allowOffset;
  return x >= left && x <= right && y >= top && y <= bottom;
}

void glwInitSystemFontTexture(glwWin *win) {
  char asciiTable[GLW_SYSTEM_FONT_CHAR_NUM + 1];
  int i;
  GLuint texture = 0;
  glwFont *font;
  int pixelsWide = 0;
  int pixelsHigh = 0;
  float scaleX, scaleY;
  glwSystemFontTexture *systemFontTexture = glwGetSystemFontTexture(win);
  
  for (i = 0; i <= GLW_SYSTEM_FONT_CHAR_NUM; ++i) {
    asciiTable[i] = GLW_SYSTEM_FONT_BEGIN_CHAR + i;
  }
  asciiTable[GLW_SYSTEM_FONT_CHAR_NUM] = '\0';
  
  font = glwCreateFont(GLW_SYSTEM_FONT_NAME, GLW_SYSTEM_FONT_WEIGHT,
    GLW_SYSTEM_FONT_SIZE, 0);
  systemFontTexture->lefts[0] = 0;
  systemFontTexture->size.width = 0;
  systemFontTexture->size.height = 0;
  for (i = 0; i < GLW_SYSTEM_FONT_CHAR_NUM; ++i) {
    char ch[2] = {asciiTable[i], '\0'};
    glwSize chSize = glwMeasureText(ch, font);
    if (chSize.height > systemFontTexture->size.height) {
      systemFontTexture->size.height = chSize.height;
    }
    systemFontTexture->size.width += chSize.width;
    systemFontTexture->widths[i] = chSize.width;
    if (i > 0) {
      systemFontTexture->lefts[i] = (float)(systemFontTexture->lefts[i - 1] +
        systemFontTexture->widths[i - 1]);
    }
  }
  systemFontTexture->originSize = systemFontTexture->size;
  for (i = 0; i < GLW_SYSTEM_FONT_CHAR_NUM; ++i) {
    char ch[2] = {asciiTable[i], '\0'};
    glwSize chSize = {systemFontTexture->widths[i],
      systemFontTexture->originSize.height};
    unsigned char *rgba = glwRenderTextToRGBA(ch, font, chSize,
      &pixelsWide, &pixelsHigh);
    if (0 == i) {
      scaleX = (float)pixelsWide / chSize.width;
      scaleY = (float)pixelsHigh / chSize.height;
      systemFontTexture->size.width *= scaleX;
      systemFontTexture->size.height *= scaleY;
      glEnable(GL_TEXTURE_2D);
      glGenTextures(1, &texture);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
      glTexImage2D(GL_TEXTURE_2D, 0, 4,
        (GLsizei)(systemFontTexture->size.width),
        (GLsizei)(systemFontTexture->size.height), 0,
        GL_RGBA, GL_UNSIGNED_BYTE, 0);
    }
    systemFontTexture->lefts[i] *= scaleX;
    glTexSubImage2D(GL_TEXTURE_2D, 0,
      systemFontTexture->lefts[i], 0,
      (GLsizei)pixelsWide,
      (GLsizei)pixelsHigh,
      GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    free(rgba);
  }
  systemFontTexture->texId = (int)texture;
  glwDestroyFont(font);
}

void glwInitSystemIconTexture(glwWin *win) {
  glwWinContext *ctx = glwGetWinContext(win);
  if (0 == ctx->iconTexId) {
    char filename[1024];
    unsigned int err;
    unsigned char *imageData = 0;
    unsigned int width = 0;
    unsigned int height = 0;
    GLuint texture = 0;
    snprintf(filename, sizeof(filename), "%s/icons.png", ctx->root);
    err = lodepng_decode32_file(&imageData, &width, &height,
      filename);
    if (err) {
      fprintf(stderr, "%s: lodepng_decode32_file error: %s\n", __FUNCTION__,
        lodepng_error_text(err));
      return;
    }
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
    glTexImage2D(GL_TEXTURE_2D, 0, 4,
      (GLsizei)ICON_IMAGE_WIDTH,
      (GLsizei)ICON_IMAGE_HEIGHT, 0,
      GL_RGBA, GL_UNSIGNED_BYTE, imageData);
    free(imageData);
    ctx->iconTexId = (int)texture;
    ctx->iconTexWidth = width;
    ctx->iconTexHeight = height;
  }
}

static glwVec2 glwRoundedCorners[GLW_SMALL_ROUNDED_CORNER_SLICES] = {{0}};

static void createRoundedCorners(glwVec2 *arr, int num) {
  float slice = M_PI / 2 / num;
  int i;
  float a = 0;
  for (i = 0; i < num; a += slice, ++i) {
    arr[i].x = cosf(a);
    arr[i].y = sinf(a);
  }
}

void glwInitPrimitives(void) {
  createRoundedCorners(glwRoundedCorners, GLW_SMALL_ROUNDED_CORNER_SLICES);
}

static void glwDrawRightTopVertexs(float left, float top, float right,
    float bottom, float radius) {
  int i;
  for (i = GLW_SMALL_ROUNDED_CORNER_SLICES - 1; i >= 0; --i) {
    glVertex2f(right - radius + radius * glwRoundedCorners[i].x,
      top + radius - radius * glwRoundedCorners[i].y);
  }
}

static void glwDrawRightBottomVertexs(float left, float top, float right,
    float bottom, float radius) {
  int i;
  for (i = 0; i < GLW_SMALL_ROUNDED_CORNER_SLICES; ++i) {
    glVertex2f(right - radius + radius * glwRoundedCorners[i].x,
      bottom - radius + radius * glwRoundedCorners[i].y);
  }
}

static void glwDrawLeftBottomVertexs(float left, float top, float right,
    float bottom, float radius) {
  int i;
  for (i = GLW_SMALL_ROUNDED_CORNER_SLICES - 1; i >= 0; --i) {
    glVertex2f(left + radius - radius * glwRoundedCorners[i].x,
      bottom - radius + radius * glwRoundedCorners[i].y);
  }
}

static void glwDrawLeftTopVertexs(float left, float top, float right,
    float bottom, float radius) {
  int i;
  for (i = 0; i < GLW_SMALL_ROUNDED_CORNER_SLICES; ++i) {
    glVertex2f(left + radius - radius * glwRoundedCorners[i].x,
      top + radius - radius * glwRoundedCorners[i].y);
  }
}

void glwDrawCheckSymbol(int x, int y, int width, int height,
    unsigned int color) {
  glColor3f(glwR(color), glwG(color), glwB(color));
  glLineWidth(2);
  glBegin(GL_LINE_STRIP);
    glVertex2f(x + width * 0.15, y + height * 0.3);
    glVertex2f(x + width * 0.45, y + height * 0.65);
    glVertex2f(x + width * 1.05, y - height * 0.05);
  glEnd();
  glLineWidth(1);
}

void glwDrawDropdownArrow(int x, int y, int width, int height,
    unsigned int color) {
  glColor3f(glwR(color), glwG(color), glwB(color));
  glBegin(GL_TRIANGLES);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width * 0.5, y + height);
  glEnd();
}

void glwDrawVLine(int x, int y, int width, int height,
    unsigned int color) {
  glColor3f(glwR(color), glwG(color), glwB(color));
  glLineWidth(width);
  glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x, y + height - 1);
  glEnd();
  glLineWidth(1);
}

void glwDrawHLine(int x, int y, int width, int height,
    unsigned int color) {
  glColor3f(glwR(color), glwG(color), glwB(color));
  glLineWidth(height);
  glBegin(GL_LINES);
    glVertex2f(x, y);
    glVertex2f(x + width - 1, y);
  glEnd();
  glLineWidth(1);
}

void glwDrawRoundedRectBorder(float x, float y, float width, float height,
    float radius, unsigned int color) {
  float left = x;
  float top = y;
  float bottom = y + height - 1;
  float right = x + width - 1;
  glDisable(GL_TEXTURE_2D);
  glColor3f(glwR(color), glwG(color), glwB(color));
  glBegin(GL_LINE_LOOP);
    glVertex2f(left, top + radius);
    glwDrawLeftTopVertexs(left, top, right, bottom, radius);
    glVertex2f(left + radius, top);
  
    glVertex2f(right - radius, top);
    glwDrawRightTopVertexs(left, top, right, bottom, radius);
    glVertex2f(right, top + radius);
  
    glVertex2f(right, bottom - radius);
    glwDrawRightBottomVertexs(left, top, right, bottom, radius);
    glVertex2f(right - radius, bottom);
  
    glVertex2f(left + radius, bottom);
    glwDrawLeftBottomVertexs(left, top, right, bottom, radius);
    glVertex2f(left, bottom - radius);
  glEnd();
}

void glwDrawTopRoundedRectBorder(float x, float y, float width, float height,
    float radius, unsigned int color) {
  float left = x;
  float top = y;
  float bottom = y + height - 1;
  float right = x + width - 1;
  glDisable(GL_TEXTURE_2D);
  glColor3f(glwR(color), glwG(color), glwB(color));
  glBegin(GL_LINE_LOOP);
    glVertex2f(left, top + radius);
    glwDrawLeftTopVertexs(left, top, right, bottom, radius);
    glVertex2f(left + radius, top);
  
    glVertex2f(right - radius, top);
    glwDrawRightTopVertexs(left, top, right, bottom, radius);
    glVertex2f(right, top + radius);
  
    glVertex2f(right, bottom);
    glVertex2f(left, bottom);
  glEnd();
}

void glwDrawLeftRoundedRectGradientFill(float x, float y,
    float width, float height,
    float radius, unsigned int topColor, unsigned int bottomColor) {
  float left = x;
  float top = y;
  float bottom = y + height - 1;
  float right = x + width - 1;
  int i;
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUAD_STRIP);
    for (i = 0; i < GLW_SMALL_ROUNDED_CORNER_SLICES; ++i) {
      glColor3f(glwR(bottomColor), glwG(bottomColor), glwB(bottomColor));
      glVertex2f(left + radius - radius * glwRoundedCorners[i].x,
        bottom - radius + radius * glwRoundedCorners[i].y);
      glColor3f(glwR(topColor), glwG(topColor), glwB(topColor));
      glVertex2f(left + radius - radius * glwRoundedCorners[i].x,
        top + radius - radius * glwRoundedCorners[i].y);
    }
    glColor3f(glwR(bottomColor), glwG(bottomColor), glwB(bottomColor));
    glVertex2f(right, bottom);
    glColor3f(glwR(topColor), glwG(topColor), glwB(topColor));
    glVertex2f(right, top);
  glEnd();
}

void glwDrawTopRoundedRectGradientFill(float x, float y,
    float width, float height,
    float radius, unsigned int topColor, unsigned int bottomColor) {
  float left = x;
  float top = y;
  float bottom = y + height - 1;
  float right = x + width - 1;
  int i;
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUAD_STRIP);
    for (i = 0; i < GLW_SMALL_ROUNDED_CORNER_SLICES; ++i) {
      glColor3f(glwR(bottomColor), glwG(bottomColor), glwB(bottomColor));
      glVertex2f(left + radius - radius * glwRoundedCorners[i].x,
        bottom);
      glColor3f(glwR(topColor), glwG(topColor), glwB(topColor));
      glVertex2f(left + radius - radius * glwRoundedCorners[i].x,
        top + radius - radius * glwRoundedCorners[i].y);
    }
    for (i = GLW_SMALL_ROUNDED_CORNER_SLICES - 1; i >= 0; --i) {
      glColor3f(glwR(bottomColor), glwG(bottomColor), glwB(bottomColor));
      glVertex2f(right - radius + radius * glwRoundedCorners[i].x,
        bottom);
      glColor3f(glwR(topColor), glwG(topColor), glwB(topColor));
      glVertex2f(right - radius + radius * glwRoundedCorners[i].x,
        top + radius - radius * glwRoundedCorners[i].y);
    }
  glEnd();
}

void glwDrawRectGradientFill(float x, float y,
    float width, float height,
    unsigned int topColor, unsigned int bottomColor) {
  float left = x;
  float top = y;
  float bottom = y + height - 1;
  float right = x + width - 1;
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUAD_STRIP);
    glColor3f(glwR(bottomColor), glwG(bottomColor), glwB(bottomColor));
    glVertex2f(left, bottom);
    glColor3f(glwR(topColor), glwG(topColor), glwB(topColor));
    glVertex2f(left, top);
    glColor3f(glwR(bottomColor), glwG(bottomColor), glwB(bottomColor));
    glVertex2f(right, bottom);
    glColor3f(glwR(topColor), glwG(topColor), glwB(topColor));
    glVertex2f(right, top);
  glEnd();
}

void glwDrawRightRoundedRectGradientFill(float x, float y,
    float width, float height,
    float radius, unsigned int topColor, unsigned int bottomColor) {
  float left = x;
  float top = y;
  float bottom = y + height - 1;
  float right = x + width - 1;
  int i;
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUAD_STRIP);
    glColor3f(glwR(bottomColor), glwG(bottomColor), glwB(bottomColor));
    glVertex2f(left, bottom);
    glColor3f(glwR(topColor), glwG(topColor), glwB(topColor));
    glVertex2f(left, top);
    for (i = GLW_SMALL_ROUNDED_CORNER_SLICES - 1; i >= 0; --i) {
      glColor3f(glwR(bottomColor), glwG(bottomColor), glwB(bottomColor));
      glVertex2f(right - radius + radius * glwRoundedCorners[i].x,
        bottom - radius + radius * glwRoundedCorners[i].y);
      glColor3f(glwR(topColor), glwG(topColor), glwB(topColor));
      glVertex2f(right - radius + radius * glwRoundedCorners[i].x,
        top + radius - radius * glwRoundedCorners[i].y);
    }
  glEnd();
}

void glwDrawRoundedRectGradientFill(float x, float y, float width, float height,
    float radius, unsigned int topColor, unsigned int bottomColor) {
  float left = x;
  float top = y;
  float bottom = y + height - 1;
  float right = x + width - 1;
  int i;
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUAD_STRIP);
    for (i = 0; i < GLW_SMALL_ROUNDED_CORNER_SLICES; ++i) {
      glColor3f(glwR(bottomColor), glwG(bottomColor), glwB(bottomColor));
      glVertex2f(left + radius - radius * glwRoundedCorners[i].x,
        bottom - radius + radius * glwRoundedCorners[i].y);
      glColor3f(glwR(topColor), glwG(topColor), glwB(topColor));
      glVertex2f(left + radius - radius * glwRoundedCorners[i].x,
        top + radius - radius * glwRoundedCorners[i].y);
    }
    for (i = GLW_SMALL_ROUNDED_CORNER_SLICES - 1; i >= 0; --i) {
      glColor3f(glwR(bottomColor), glwG(bottomColor), glwB(bottomColor));
      glVertex2f(right - radius + radius * glwRoundedCorners[i].x,
        bottom - radius + radius * glwRoundedCorners[i].y);
      glColor3f(glwR(topColor), glwG(topColor), glwB(topColor));
      glVertex2f(right - radius + radius * glwRoundedCorners[i].x,
        top + radius - radius * glwRoundedCorners[i].y);
    }
  glEnd();
}

static void glwDrawGradientBackground(float x, float y, float width, float height,
    glwCtrlState state, int sunken) {
  unsigned int topColor = 0;
  unsigned int bottomColor = 0;
  switch (state)
  {
  case GLW_CTRL_STATE_PRESS:
    topColor = GLW_FILL_GRADIENT_COLOR_1_H;
    bottomColor = GLW_FILL_GRADIENT_COLOR_2_H;
    break;
  case GLW_CTRL_STATE_NORMAL:
  default:
    topColor = GLW_FILL_GRADIENT_COLOR_1;
    bottomColor = GLW_FILL_GRADIENT_COLOR_2;
    break;
  }
  if (sunken) {
    glwSwapColor(topColor, bottomColor);
  }
  glwDrawRoundedRectGradientFill(x, y, width, height,
      GLW_BUTTON_CORNER_RADIUS,
      topColor, bottomColor);
  glwDrawRoundedRectBorder(x, y, width, height,
    GLW_BUTTON_CORNER_RADIUS, GLW_BORDER_COLOR_2);
}

void glwDrawButtonBackground(float x, float y, float width, float height,
    glwCtrlState state) {
  glwDrawGradientBackground(x, y, width, height, state, 0);
}

glwSize glwMeasureSystemText(glwWin *win, char *text) {
  glwSystemFontTexture *systemFontTexture = glwGetSystemFontTexture(win);
  glwSize size = {0, systemFontTexture->originSize.height};
  for (; *text; text++) {
    int idx = (*text) - GLW_SYSTEM_FONT_BEGIN_CHAR;
    if (idx < 0 || idx > GLW_SYSTEM_FONT_CHAR_NUM) {
      continue;
    }
    size.width += systemFontTexture->widths[idx];
  }
  return size;
}

static void glwImGUIActiveIdCheck(glwImGui *imGUI, int id, int x, int y,
    int width, int height) {
  if (GLW_DOWN == imGUI->mouseState) {
    if (!imGUI->activeId) {
      int hit = glwPointTest(imGUI->mouseX, imGUI->mouseY, x, y,
        width, height, 0);
      if (hit) {
        imGUI->activeId = id;
      }
    }
  } else if (GLW_UP == imGUI->mouseState) {
    if (imGUI->activeId == id) {
      imGUI->activeId = 0;
    }
  }
}

int glwImButton(glwWin *win, int id, int x, int y, char *text, int icon) {
  glwImGui *imGUI = glwGetImGUI(win);
  glwSize textSize = glwMeasureSystemText(win, text);
  int width = textSize.width * (1 + GLW_HOR_AUTO_MARGIN * 2);
  int height = textSize.height * (1 + GLW_VER_AUTO_MARGIN * 2);
  glwImGUIActiveIdCheck(imGUI, id, x, y, width, height);
  glwDrawButtonBackground(x, y, width, height,
    imGUI->activeId == id ? GLW_CTRL_STATE_PRESS : GLW_CTRL_STATE_NORMAL);
  glwDrawSystemText(win, x + textSize.width * GLW_HOR_AUTO_MARGIN,
    y + textSize.height * GLW_VER_AUTO_MARGIN,
    text, GLW_SYSTEM_FONT_COLOR);
  imGUI->nextX = x + width;
  imGUI->nextY = y;
  return imGUI->activeId == id;
}

static glwSize glwMeasureLabel(glwWin *win, char *text, int icon) {
  glwSize size = glwMeasureSystemText(win, text);
  int padding = glwGetLineHeight(win) / 2;
  size.width += padding + padding + padding;
  return size;
}

static void glwDrawLabel(glwWin *win, int x, int y, char *text, int icon,
    unsigned int color) {
  int lineHeight = glwGetLineHeight(win);
  int padding = glwGetLineHeight(win) / 2;
  if (icon) {
    int iconIdx = icon - 1;
    glwDrawSystemIcon(win, x + padding,
      y + lineHeight * GLW_VER_AUTO_MARGIN, icon, color);
    glwDrawSystemText(win, x + padding +
        iconTable[iconIdx][ICON_ITEM_ORIGINAL_WIDTH] + padding / 2,
      y + lineHeight * GLW_VER_AUTO_MARGIN, text, color);
  } else {
    glwDrawSystemText(win, x + padding,
      y + lineHeight * GLW_VER_AUTO_MARGIN, text, color);
  }
}

int glwImDropdownBox(glwWin *win, int id, int x, int y, char *text) {
  glwImGui *imGUI = glwGetImGUI(win);
  glwSize textSize = glwMeasureSystemText(win, text);
  int arrowHeight = textSize.height * 0.3;
  int arrowWidth = arrowHeight * 2;
  int width = textSize.width * (1 + GLW_HOR_AUTO_MARGIN * 2) + arrowWidth * 3;
  int height = textSize.height * (1 + GLW_VER_AUTO_MARGIN * 2);
  glwImGUIActiveIdCheck(imGUI, id, x, y, width, height);
  glwDrawButtonBackground(x, y, width, height,
    imGUI->activeId == id ? GLW_CTRL_STATE_PRESS : GLW_CTRL_STATE_NORMAL);
  glwDrawSystemText(win, x + textSize.width * (GLW_HOR_AUTO_MARGIN),
    y + textSize.height * (GLW_VER_AUTO_MARGIN),
    text, GLW_SYSTEM_FONT_COLOR);
  glwDrawDropdownArrow(x + width - textSize.width * GLW_HOR_AUTO_MARGIN -
      arrowWidth,
    y + (height - arrowHeight) / 2, arrowWidth, arrowHeight, 0);
  imGUI->nextX = x + width;
  imGUI->nextY = y;
  return imGUI->activeId == id;
}

int glwImCheck(glwWin *win, int id, int x, int y, char *text, int checked) {
  glwImGui *imGUI = glwGetImGUI(win);
  glwSize textSize = glwMeasureSystemText(win, text);
  int height = textSize.height;
  int boxWidth = height * 0.8;
  int boxHeight = boxWidth;
  int width = boxWidth + textSize.height * (1 + GLW_HOR_AUTO_MARGIN * 2) +
    textSize.width;
  int oldActiveId = imGUI->activeId;
  glwImGUIActiveIdCheck(imGUI, id, x, y, width, height);
  glwDrawGradientBackground(x, y + (height - boxHeight) / 2,
    boxWidth, boxHeight,
    (imGUI->activeId == id || checked) ?
      GLW_CTRL_STATE_PRESS : GLW_CTRL_STATE_NORMAL, 1);
  glwDrawSystemText(win, x + textSize.height * GLW_HOR_AUTO_MARGIN + boxWidth,
    y, text, GLW_SYSTEM_FONT_COLOR);
  if (checked) {
    glwDrawCheckSymbol(x, y, boxWidth, height, GLW_CHECK_COLOR);
  }
  imGUI->nextX = x + width;
  imGUI->nextY = y;
  return (imGUI->activeId == id && oldActiveId != id) ? !checked : checked;
}

static int glwCalcListMaxWidth(glwWin *win, char **titles, int *icons,
    int *height, int *len) {
  int i;
  int itemWidth = 0;
  int maxItemWidth = 0;

  for (i = 0; titles[i]; ++i) {
    glwSize textSize = glwMeasureLabel(win, titles[i], icons ? icons[i] : 0);
    itemWidth = textSize.width;
    if (icons && icons[i]) {
      itemWidth += iconTable[icons[i]][ICON_ITEM_ORIGINAL_WIDTH];
    }
    if (itemWidth > maxItemWidth) {
      maxItemWidth = itemWidth;
    }
    if (height && !(*height)) {
      *height = textSize.height * (1 + GLW_VER_AUTO_MARGIN * 2);
    }
    if (len) {
      ++(*len);
    }
  }
  
  return maxItemWidth;
}

int glwImButtonGroup(glwWin *win, int id, int x, int y, char **titles,
    int *icons, int sel) {
  glwImGui *imGUI = glwGetImGUI(win);
  int width = 0;
  int height = 0;
  int left = 0;
  int i;
  int listLen = 0;
  int offset;
  int maxItemWidth = 0;
  unsigned int topColor = 0;
  unsigned int bottomColor = 0;
  maxItemWidth = glwCalcListMaxWidth(win, titles, icons, &height, &listLen);
  width = maxItemWidth * listLen;
  left = x;
  glwImGUIActiveIdCheck(imGUI, id, left, y, width, height);
  glwDrawRoundedRectGradientFill(left, y, width, height,
    GLW_BUTTON_CORNER_RADIUS,
    GLW_FILL_GRADIENT_COLOR_1, GLW_FILL_GRADIENT_COLOR_2);
  for (i = 0, offset = left; titles[i]; ++i) {
    if (imGUI->activeId == id) {
      int hit = glwPointTest(imGUI->mouseX, imGUI->mouseY, offset, y,
        maxItemWidth, height, 0);
      if (hit) {
        sel = i;
      }
    }
    if (sel == i) {
      topColor = GLW_FILL_GRADIENT_COLOR_1_H;
      bottomColor = GLW_FILL_GRADIENT_COLOR_2_H;
      if (imGUI->activeId == id) {
        glwSwapColor(topColor, bottomColor);
      }
      if (1 == listLen) {
        glwDrawRoundedRectGradientFill(left, y, width, height,
          GLW_BUTTON_CORNER_RADIUS,
          topColor, bottomColor);
      } else {
        if (0 == i) {
          glwDrawLeftRoundedRectGradientFill(offset, y,
            maxItemWidth, height,
            GLW_BUTTON_CORNER_RADIUS,
            topColor, bottomColor);
        } else if (listLen - 1 == i) {
          glwDrawRightRoundedRectGradientFill(offset, y,
            maxItemWidth, height,
            GLW_BUTTON_CORNER_RADIUS,
            topColor, bottomColor);
        } else {
          glwDrawRectGradientFill(offset, y,
            maxItemWidth, height,
            topColor, bottomColor);
        }
      }
    }
    if (0 != i) {
      glwDrawVLine(offset, y, 1, height, GLW_BORDER_COLOR_2);
    }
    glwDrawLabel(win, offset, y, titles[i], icons ? icons[i] : 0,
      GLW_SYSTEM_FONT_COLOR);
    offset += maxItemWidth;
  }
  imGUI->nextX = offset;
  imGUI->nextY = y;
  glwDrawRoundedRectBorder(left, y, width, height,
    GLW_BUTTON_CORNER_RADIUS, GLW_BORDER_COLOR_2);
  return sel;
}

int glwImTabBox(glwWin *win, int id, int x, int y, int width, int height,
    char **titles, int *icons, int sel) {
  glwImGui *imGUI = glwGetImGUI(win);
  int tabWidth = 0;
  int tabHeight = 0;
  int left = 0;
  int i;
  int listLen = 0;
  int offset;
  int maxItemWidth = 0;
  maxItemWidth = glwCalcListMaxWidth(win, titles, icons, &tabHeight, &listLen);
  if (maxItemWidth < GLW_MIN_TAB_WIDTH) {
    maxItemWidth = GLW_MIN_TAB_WIDTH;
  }
  tabWidth = maxItemWidth * listLen;
  left = x;
  imGUI->nextX = x;
  imGUI->nextY = y + tabHeight;
  glwImGUIActiveIdCheck(imGUI, id, left, y, tabWidth, tabHeight);
  glwDrawRectGradientFill(x + 1, y + tabHeight,
    width - 2, height - tabHeight - 2,
    GLW_TAB_FILL_GRADIENT_COLOR_2, GLW_TAB_FILL_GRADIENT_COLOR_2);
  glwDrawHLine(x, y + tabHeight - 1, width, 1, GLW_BORDER_COLOR_1);
  glwDrawHLine(x, y + height - 1, width, 1, GLW_BORDER_COLOR_1);
  glwDrawVLine(x, y + tabHeight, 1, height - tabHeight,
    GLW_BORDER_COLOR_1);
  glwDrawVLine(x + width - 1, y + tabHeight, 1, height - tabHeight,
    GLW_BORDER_COLOR_1);
  for (i = 0, offset = left; titles[i]; ++i) {
    if (imGUI->activeId == id) {
      int hit = glwPointTest(imGUI->mouseX, imGUI->mouseY, offset, y,
        maxItemWidth, tabHeight, 0);
      if (hit) {
        sel = i;
      }
    }
    if (sel == i) {
      glwDrawTopRoundedRectBorder(offset, y,
        maxItemWidth, tabHeight,
        GLW_BUTTON_CORNER_RADIUS, GLW_BORDER_COLOR_1);
      glwDrawTopRoundedRectGradientFill(offset + 1, y + 1,
        maxItemWidth - 2, tabHeight,
        GLW_BUTTON_CORNER_RADIUS,
        GLW_TAB_FILL_GRADIENT_COLOR_1, GLW_TAB_FILL_GRADIENT_COLOR_2);
    }
    glwDrawLabel(win, offset, y, titles[i], icons ? icons[i] : 0,
      GLW_SYSTEM_FONT_COLOR);
    offset += maxItemWidth;
  }
  return sel;
}

int glwImPanel(glwWin *win, int id, int x, int y, int width, int height) {
  glwDrawRectGradientFill(x, y, width, height,
    GLW_PANEL_FILL_COLOR, GLW_PANEL_FILL_COLOR);
  glwDrawRoundedRectBorder(x, y, width, height,
    GLW_BUTTON_CORNER_RADIUS, GLW_BORDER_COLOR_1);
  return 0;
}

int glwImToolBar(glwWin *win, int id, int x, int y, int width, int height) {
  glwImGui *imGUI = glwGetImGUI(win);
  glwDrawRectGradientFill(x + 1, y + 1, width - 2, height - 2,
    GLW_TOOLBAR_BACKGROUND_COLOR, GLW_TOOLBAR_BACKGROUND_COLOR);
  glwDrawHLine(x, y + height - 1, width, 1, GLW_BORDER_COLOR_1);
  imGUI->nextX = x;
  imGUI->nextY = y + height;
  return 0;
}

int glwImBottomBar(glwWin *win, int id, int x, int y, int width, int height) {
  glwImGui *imGUI = glwGetImGUI(win);
  glwDrawRectGradientFill(x + 1, y + 1, width - 2, height - 2,
    GLW_TOOLBAR_BACKGROUND_COLOR, GLW_TOOLBAR_BACKGROUND_COLOR);
  glwDrawHLine(x, y, width, 1, GLW_BORDER_COLOR_1);
  imGUI->nextX = x;
  imGUI->nextY = y - height;
  return 0;
}

int glwImNextX(glwWin *win) {
  glwImGui *imGUI = glwGetImGUI(win);
  return imGUI->nextX;
}

int glwImNextY(glwWin *win) {
  glwImGui *imGUI = glwGetImGUI(win);
  return imGUI->nextY;
}

int glwImLineHeight(glwWin *win) {
  return glwGetLineHeight(win);
}

void glwSetUserData(glwWin *win, void *userData) {
  glwWinContext *ctx = glwGetWinContext(win);
  ctx->userData = userData;
}

void *glwGetUserData(glwWin *win) {
  glwWinContext *ctx = glwGetWinContext(win);
  return ctx->userData;
}

void glwDisplayFunc(glwWin *win, void (*func)(glwWin *win)) {
  glwWinContext *ctx = glwGetWinContext(win);
  ctx->onDisplay = func;
}

void glwReshapeFunc(glwWin *win, void (*func)(glwWin *win, int width,
    int height)) {
  glwWinContext *ctx = glwGetWinContext(win);
  ctx->onReshape = func;
}

void glwMouseFunc(glwWin *win, void (*func)(glwWin *win, int button, int state,
    int x, int y)) {
  glwWinContext *ctx = glwGetWinContext(win);
  ctx->onMouse = func;
}

void glwKeyboardFunc(glwWin *win, void (*func)(glwWin *win, unsigned char key,
    int x, int y)) {
  glwWinContext *ctx = glwGetWinContext(win);
  ctx->onKeyboard = func;
}

void glwMotionFunc(glwWin *win,
    void (*func)(glwWin *win, int x, int y)) {
  glwWinContext *ctx = glwGetWinContext(win);
  ctx->onMotion = func;
}

void glwPassiveMotionFunc(glwWin *win,
    void (*func)(glwWin *win, int x, int y)) {
  glwWinContext *ctx = glwGetWinContext(win);
  ctx->onPassiveMotion = func;
}

void glwWheelFunc(glwWin *win, void(*func)(glwWin *win, float delta)) {
  glwWinContext *ctx = glwGetWinContext(win);
  ctx->onWheel = func;
}

int glwMouseX(glwWin *win) {
  glwWinContext *ctx = glwGetWinContext(win);
  return ctx->x;
}

int glwMouseY(glwWin *win) {
  glwWinContext *ctx = glwGetWinContext(win);
  return ctx->y;
}

int glwImMenu(glwWin *win, int id, int x, int y, int width, int height,
    char **titles, int sel) {
  glwImGui *imGUI = glwGetImGUI(win);
  int left = 0;
  int i;
  int listLen = 0;
  int offset;
  int maxItemWidth = 0;
  maxItemWidth = glwCalcListMaxWidth(win, titles, 0, &height, &listLen);
  left = x + glwGetLineHeight(win) / 2;
  glwImGUIActiveIdCheck(imGUI, id, left, y, width, height);
  glwDrawRectGradientFill(x, y, width, height,
    GLW_MENU_BACKGROUND_COLOR, GLW_MENU_BACKGROUND_COLOR);
  for (i = 0, offset = left; titles[i]; ++i) {
    if (imGUI->activeId == id) {
      int hit = glwPointTest(imGUI->mouseX, imGUI->mouseY, offset, y,
        maxItemWidth, height, 0);
      if (hit) {
        sel = i;
      }
    }
    glwDrawLabel(win, offset, y, titles[i], 0, sel == i ?
      GLW_SYSTEM_FONT_COLOR_H : GLW_SYSTEM_FONT_COLOR);
    offset += maxItemWidth;
  }
  glwDrawHLine(x, y + height - 1, width, 1, GLW_BORDER_COLOR_1);
  imGUI->nextX = x;
  imGUI->nextY = y + height;
  return sel;
}

static void glwDrawGradientLine(int x, int y, int width, int height,
    unsigned int beginColor, unsigned int endColor) {
  glLineWidth(height);
  glBegin(GL_LINES);
    glColor3f(glwR(beginColor), glwG(beginColor), glwB(beginColor));
    glVertex2f(x, y);
    glColor3f(glwR(endColor), glwG(endColor), glwB(endColor));
    glVertex2f(x + width, y);
  glEnd();
  glLineWidth(1);
}

float glwImSlider(glwWin *win, int id, int x, int y, int width,
    float min, float max, float cur, char *fmt, ...) {
  glwImGui *imGUI = glwGetImGUI(win);
  char text[100];
  float leftSegLen = width * (cur - min) / max;
  float thumLeft = x + leftSegLen - (GLW_SLIDER_THUMB_WIDTH / 2);
  float thumTop = y + glwGetLineHeight(win) - (GLW_SLIDER_THUMB_HEIGHT / 2);
  unsigned int topColor = 0;
  unsigned int bottomColor = 0;
  va_list args;
  va_start(args, fmt);
  vsnprintf(text, sizeof(text), fmt, args);
  glwImGUIActiveIdCheck(imGUI, id, thumLeft, thumTop,
    GLW_SLIDER_THUMB_WIDTH, GLW_SLIDER_THUMB_HEIGHT);
  topColor = GLW_FILL_GRADIENT_COLOR_1_H;
  bottomColor = GLW_FILL_GRADIENT_COLOR_2_H;
  if (imGUI->activeId == id) {
    float mouseFromLeft = (float)glwMouseX(win) - x;
    glwSwapColor(topColor, bottomColor);
    cur = min + (max - min) * mouseFromLeft / width;
  }
  if (cur < min) {
    cur = min;
  } else if (cur > max) {
    cur = max;
  }
  glwDrawSystemText(win, x, y, text, GLW_SYSTEM_FONT_COLOR);
  glwDrawGradientLine(x, y + glwGetLineHeight(win), leftSegLen,
    GLW_SLIDER_HEIGHT,
    GLW_TOOLBAR_BACKGROUND_COLOR, 0x4d4d4d);
  glwDrawGradientLine(x + leftSegLen, y + glwGetLineHeight(win),
    width - leftSegLen, GLW_SLIDER_HEIGHT,
    0x4d4d4d, GLW_TOOLBAR_BACKGROUND_COLOR);
  glwDrawRoundedRectGradientFill(thumLeft, thumTop,
    GLW_SLIDER_THUMB_WIDTH, GLW_SLIDER_THUMB_HEIGHT,
    GLW_SLIDER_CORNER_RADIUS,
    topColor, bottomColor);
  glwDrawRoundedRectBorder(thumLeft, thumTop,
    GLW_SLIDER_THUMB_WIDTH, GLW_SLIDER_THUMB_HEIGHT,
    GLW_SLIDER_CORNER_RADIUS, GLW_BORDER_COLOR_2);
  return cur;
}

static void *activeThreadWrapper(void *param) {
  glwWin *win = (glwWin *)param;
  glwImGui *imGUI = glwGetImGUI(win);
  imGUI->activeThreadCall(win, imGUI->activeThreadTag);
  imGUI->activeThreadDoneFlag = 1;
  return 0;
}

int glwImThread(glwWin *win, int id, void (*thread)(glwWin *win, void *tag),
    void *tag) {
  glwImGui *imGUI = glwGetImGUI(win);
  int finished = 0;
  if (0 == imGUI->activeThreadId) {
    imGUI->activeThreadId = id;
    imGUI->activeThreadTag = tag;
    imGUI->activeThreadDoneFlag = 0;
    imGUI->activeThreadCall = thread;
    pthread_create(&imGUI->activeThread, NULL, activeThreadWrapper, win);
  } else if (id == imGUI->activeThreadId) {
    if (imGUI->activeThreadDoneFlag) {
      pthread_join(imGUI->activeThread, NULL);
      imGUI->activeThreadId = 0;
      finished = 1;
    }
  }
  return finished;
}
