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
String track_album = "앨범: 없음";

// 전역 변수 추가
unsigned long g38PressStartTime = 0;
const unsigned long LONG_PRESS_TIME = 2000;
bool isPaused = false;

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

void drawPlayIcon(M5EPD_Canvas *canvas, int x, int y, int size)
{
  canvas->fillTriangle(x, y, x, y + size, x + size, y + size / 2, 0);
}

void drawPauseIcon(M5EPD_Canvas *canvas, int x, int y, int size)
{
  int barWidth = size / 3;
  canvas->fillRect(x, y, barWidth, size, 0);
  canvas->fillRect(x + 2 * barWidth, y, barWidth, size, 0);
}

void drawPreviousIcon(M5EPD_Canvas *canvas, int x, int y, int size)
{
  canvas->fillTriangle(x + size, y, x + size, y + size, x, y + size / 2, 0);
  canvas->fillRect(x + size, y, size / 4, size, 0);
}

void drawNextIcon(M5EPD_Canvas *canvas, int x, int y, int size)
{
  canvas->fillTriangle(x, y, x, y + size, x + size, y + size / 2, 0);
  canvas->fillRect(x - size / 4, y, size / 4, size, 0);
}

void update_display()
{
  // 텍스트 영역만 다시 그리기 위한 작은 캔버스 생성
  const int TEXT_AREA_HEIGHT = 300; // 텍스트 영역 높이
  const int TEXT_Y_START = 100;     // 텍스트 시작 y좌표

  canvas.createCanvas(960, TEXT_AREA_HEIGHT);
  canvas.fillCanvas(15); // 흰색 배경

  // 곡 정보 텍스트 출력
  printEfont(&canvas, track_title.c_str(), 40, 0, 4);
  printEfont(&canvas, track_artist.c_str(), 40, 80, 4);
  printEfont(&canvas, track_album.c_str(), 40, 160, 4);

  // 아이콘 그리기
  drawPreviousIcon(&canvas, 40, 240, 60); // 이전곡 아이콘
  if (isPaused)
  {
    drawPlayIcon(&canvas, 140, 240, 60); // 재생 아이콘
  }
  else
  {
    drawPauseIcon(&canvas, 140, 240, 60); // 일시정지 아이콘
  }
  drawNextIcon(&canvas, 240, 240, 60); // 다음곡 아이콘

  // 텍스트 영역만 부분 업데이트 (DU 모드 사용)
  canvas.pushCanvas(0, TEXT_Y_START, UPDATE_MODE_DU); // DU 모드로 변경하여 깜빡임 감소

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
  case ESP_AVRC_MD_ATTR_ALBUM:
    track_album = "앨범: " + attribute_value;
    break;
  default:
    break;
  }

  update_display();
}

void setup()
{
  M5.begin();
  // 가로 모드
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

  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);

  // 블루투스 설정
  Serial.begin(115200);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.start("M5Paper Music Display");
}

void loop()
{
  M5.update();

  // G38 버튼 (가운데) 처리
  if (M5.BtnP.wasPressed())
  {
    g38PressStartTime = millis(); // 버튼 누른 시간 기록
  }

  if (M5.BtnP.isPressed())
  {
    // 길게 누르기 감지
    if (millis() - g38PressStartTime >= LONG_PRESS_TIME)
    {
      // 전원 끄기
      M5.EPD.Clear(true);
      delay(100);
      M5.shutdown(); // 전원 끄기
    }
  }
  else if (M5.BtnP.wasReleased())
  {
    // 짧게 누르기 감지
    if (millis() - g38PressStartTime < LONG_PRESS_TIME)
    {
      // 재생/일시정지 토글
      isPaused = !isPaused;
      if (isPaused)
      {
        a2dp_sink.pause();
      }
      else
      {
        a2dp_sink.play();
      }
      update_display(); // 화면 갱신
    }
  }

  // G37 버튼 (왼쪽) - 이전곡
  if (M5.BtnL.wasPressed())
  {
    a2dp_sink.previous();
    update_display();
  }

  // G39 버튼 (오른쪽) - 다음곡
  if (M5.BtnR.wasPressed())
  {
    a2dp_sink.next();
    update_display();
  }

  delay(100);
}
