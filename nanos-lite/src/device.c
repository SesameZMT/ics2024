#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  return 0;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  strncpy(buf, dispinfo + offset, len);
}

extern void getScreen(int *width, int *height);
void fb_write(const void *buf, off_t offset, size_t len) {
    int x, y;
    int len1, len2, len3;
    offset = offset >> 2; // 将偏移量右移两位，相当于除以4
    y = offset / _screen.width; // 计算y坐标
    x = offset % _screen.width; // 计算x坐标

    len = len >> 2; // 同样将长度右移两位，相当于除以4
    len1 = len2 = len3 = 0; // 初始化三个长度变量

    len1 = len <= _screen.width - x ? len : _screen.width - x; // 计算第一个矩形的长度
    _draw_rect((uint32_t *)buf, x, y, len1, 1); // 绘制第一个矩形

    if (len > len1 && ((len - len1) > _screen.width)) { // 如果剩余长度大于第一个矩形的长度，并且剩余长度大于屏幕宽度
        len2 = len - len1; // 计算第二个矩形的长度
        _draw_rect((uint32_t *)buf + len1, 0, y + 1, _screen.width, len2 / _screen.width); // 绘制第二个矩形
    }

    if (len - len1 - len2 > 0) { // 如果还有剩余长度
        len3 = len - len1 - len2; // 计算第三个矩形的长度
        _draw_rect((uint32_t *)buf + len1 + len2, 0, y + len2 / _screen.width + 1, len3, 1); // 绘制第三个矩形
    }
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  int width = 0, height = 0;
  getScreen(&width, &height);
  sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", width,height);
}
