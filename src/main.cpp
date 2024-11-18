#define LGFX_M5PAPER
#include <M5EPD.h>
#include <BluetoothA2DPSink.h>
#include "efontEnableAll.h"
#include "efont.h"
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>
#include "M5EPD.h"
#include "esp_mac.h"

M5EPD_Canvas canvas(&M5.EPD);
BluetoothA2DPSink a2dp_sink;

// 음악 정보 저장 변수
String track_title = "제목: 없음";
String track_artist = "가수: 없음";

// M5Paper용 printEfont 함수 구현
void printEfont(M5EPD_Canvas *canvas, const char *str, int x = -1, int y = -1, int textsize = 1, int color = 0)
{
  static int posX = 0;
  static int posY = 0;
  uint32_t textcolor = 0;
  uint32_t textbgcolor = 15; // M5Paper의 흰색 배경값

  if (color != 0)
  {
    textcolor = 15;
    textbgcolor = 0;
  }

  if (x != -1)
  {
    posX = x;
  }
  if (y != -1)
  {
    posY = y;
  }

  byte font[32];
  while (*str != 0x00)
  {
    // 개행 처리
    if (*str == '\n')
    {
      posY += 16 * textsize;
      posX = x != -1 ? x : 0; // 개행시 지정된 x 위치로 복귀
      str++;
      continue;
    }

    // UTF-8을 UTF-16으로 변환하고 폰트 데이터 가져오기
    uint16_t strUTF16;
    str = efontUFT8toUTF16(&strUTF16, (char *)str);
    getefontData(font, strUTF16);

    // 문자 폭 계산
    int width = 16 * textsize;
    if (strUTF16 < 0x0100)
    {
      width = 8 * textsize; // ASCII 문자는 절반 폭
    }

    // 배경 채우기
    canvas->fillRect(posX, posY, width, 16 * textsize, textbgcolor);

    // 폰트 데이터 그리기
    for (uint8_t row = 0; row < 16; row++)
    {
      word fontdata = font[row * 2] * 256 + font[row * 2 + 1];
      for (uint8_t col = 0; col < 16; col++)
      {
        if ((0x8000 >> col) & fontdata)
        {
          int drawX = posX + col * textsize;
          int drawY = posY + row * textsize;
          if (textsize == 1)
          {
            canvas->drawPixel(drawX, drawY, textcolor);
          }
          else
          {
            canvas->fillRect(drawX, drawY, textsize, textsize, textcolor);
          }
        }
      }
    }

    // 커서 이동
    posX += width;

    // 자동 줄바꿈
    if (canvas->width() <= posX)
    {
      posX = x != -1 ? x : 0;
      posY += 16 * textsize;
    }
  }
}

void update_display()
{
  // 캔버스 초기화 (가로 모드로 크기 변경)
  canvas.createCanvas(960, 540);
  canvas.fillCanvas(15);

  // 텍스트 출력 - 위치 조정
  printEfont(&canvas, track_title.c_str(), 40, 100, 3);
  printEfont(&canvas, track_artist.c_str(), 40, 250, 3);

  // 캔버스 출력
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
  canvas.deleteCanvas();
}

void avrc_metadata_callback(uint8_t attribute_id, const uint8_t *value)
{
  String attribute_value = String((const char *)value);

  switch (attribute_id)
  {
  case ESP_AVRC_MD_ATTR_TITLE:
    track_title = "제목: " + attribute_value;
    break;
  case ESP_AVRC_MD_ATTR_ARTIST:
    track_artist = "가수: " + attribute_value;
    break;
  default:
    break;
  }

  update_display();
}

void setup()
{
  M5.begin();
  // 가로 모드로 회전
  M5.TP.SetRotation(0);  // 터치 회전
  M5.EPD.SetRotation(0); // 디스플레이 회전
  M5.EPD.Clear(true);
  delay(500);

  // 캔버스 생성 (가로 모드)
  canvas.createCanvas(960, 540);

  // 초기 화면 출력
  canvas.fillCanvas(15);

  // 제목을 화면 상단 중앙에 배치
  printEfont(&canvas, "M5Paper Music Display", 40, 20, 3);

  // 테스트를 위한 다국어 출력 - 위치 조정
  printEfont(&canvas, "music info", 40, 100, 3);
  printEfont(&canvas, "노래 정보", 40, 170, 3);
  printEfont(&canvas, "ミュージック·インフォ", 40, 230, 3);
  printEfont(&canvas, "音乐信息", 40, 290, 3);
  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);

  // 블루투스 설정
  Serial.begin(115200);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.start("M5Paper Music Display");
}

void loop()
{
  M5.update();
  delay(100);
}