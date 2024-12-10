#include <M5EPD.h>
#include <BluetoothA2DPSink.h>
#include "efont.h"
#include "efontEnableAll.h"

#define G37_PIN 37 // 이전 곡 버튼 핀
#define G38_PIN 38 // 재생/일시정지/전원 버튼 핀
#define G39_PIN 39 // 다음 곡 버튼 핀

M5EPD_Canvas canvas(&M5.EPD);
M5EPD_Canvas infoCanvas(&M5.EPD);
BluetoothA2DPSink a2dp_sink;

// 음악 정보
String track_title = "제목: 없음";
String track_artist = "가수: 없음";
String track_album = "앨범: 없음";

// 상태 변수
bool isPaused = false;
unsigned long g38PressTime = 0;
bool g38LongPressed = false;

// UI 레이아웃 변수
int btnSize = 120;
int btnPitch = 130;
int xOffset = 40;
int yOffset = 100;
int PrtOffsetx = 14;
int PrtOffsety = -7;

// mediaBtnY, volumeBtnY를 전역 변수로 선언하여 drawMusicControls와 handleTouch에서 동일하게 사용
int mediaBtnY;
int volumeBtnY;

// 터치 포인트
int point[2][2];

void printEfont(M5EPD_Canvas *canvas, const char *str, int x = -1, int y = -1, int textsize = 1, int color = 15)
{
  static int posX = 0;
  static int posY = 0;
  uint32_t textcolor = 15;
  uint32_t textbgcolor = 0;

  if (color != 15)
  {
    textcolor = 0;
    textbgcolor = 15;
  }

  if (x != -1)
    posX = x;
  if (y != -1)
    posY = y;

  byte font[32];
  while (*str != 0x00)
  {
    if (*str == '\n')
    {
      posY += 16 * textsize;
      posX = x != -1 ? x : 0;
      str++;
      continue;
    }

    uint16_t strUTF16;
    str = efontUFT8toUTF16(&strUTF16, (char *)str);
    getefontData(font, strUTF16);

    int width = (strUTF16 < 0x0100) ? (8 * textsize) : (16 * textsize);

    canvas->fillRect(posX, posY, width, 16 * textsize, textbgcolor);

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

    posX += width;
    if (canvas->width() <= posX)
    {
      posX = x != -1 ? x : 0;
      posY += 16 * textsize;
    }
  }
}

// 음악 정보 부분 갱신
void updateMusicInfo()
{
  const int TEXT_Y_START = 50;
  const int LINE_SPACING = 50;
  const int INFO_HEIGHT = TEXT_Y_START + LINE_SPACING * 3;

  infoCanvas.createCanvas(540, INFO_HEIGHT);
  infoCanvas.fillCanvas(15); // 검정색 배경
  printEfont(&infoCanvas, track_title.c_str(), xOffset, TEXT_Y_START, 2, 0);
  printEfont(&infoCanvas, track_artist.c_str(), xOffset, TEXT_Y_START + LINE_SPACING, 2, 0);
  printEfont(&infoCanvas, track_album.c_str(), xOffset, TEXT_Y_START + LINE_SPACING * 2, 2, 0);

  infoCanvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

// 전체 음악 컨트롤 UI 갱신
void drawMusicControls()
{
  canvas.createCanvas(540, 960);
  canvas.fillCanvas(15); // 검정 배경

  const int TEXT_Y_START = 50;
  const int LINE_SPACING = 50;

  // 상단 텍스트 정보
  printEfont(&canvas, track_title.c_str(), xOffset, TEXT_Y_START, 2, 0);
  printEfont(&canvas, track_artist.c_str(), xOffset, TEXT_Y_START + LINE_SPACING, 2, 0);
  printEfont(&canvas, track_album.c_str(), xOffset, TEXT_Y_START + LINE_SPACING * 2, 2, 0);

  // 미디어 컨트롤 버튼 위치 정의
  mediaBtnY = TEXT_Y_START + LINE_SPACING * 3 + 50;
  for (int i = 0; i < 3; i++)
  {
    canvas.drawRoundRect(xOffset + i * btnPitch, mediaBtnY, btnSize, btnSize, 10, 0);
  }
  printEfont(&canvas, "이전", xOffset + 0 * btnPitch + 10, mediaBtnY + btnSize / 2 - 10, 2, 0);
  printEfont(&canvas, isPaused ? "재생" : "일시정지", xOffset + 1 * btnPitch + 10, mediaBtnY + btnSize / 2 - 10, 2, 0);
  printEfont(&canvas, "다음", xOffset + 2 * btnPitch + 10, mediaBtnY + btnSize / 2 - 10, 2, 0);

  // 볼륨 컨트롤 버튼 위치 정의
  volumeBtnY = mediaBtnY + btnSize + 50;
  for (int i = 0; i < 3; i++)
  {
    canvas.drawRoundRect(xOffset + i * btnPitch, volumeBtnY, btnSize, btnSize, 10, 0);
  }
  printEfont(&canvas, "Vol-", xOffset + 0 * btnPitch + 10, volumeBtnY + btnSize / 2 - 10, 2, 0);
  printEfont(&canvas, "음소거", xOffset + 1 * btnPitch + 10, volumeBtnY + btnSize / 2 - 10, 2, 0);
  printEfont(&canvas, "Vol+", xOffset + 2 * btnPitch + 10, volumeBtnY + btnSize / 2 - 10, 2, 0);

  canvas.pushCanvas(0, 0, UPDATE_MODE_DU4);
}

// 터치 처리
void handleTouch()
{
  if (M5.TP.available())
  {
    M5.TP.update();
    if (!M5.TP.isFingerUp())
    {
      for (int i = 0; i < 2; i++)
      {
        tp_finger_t FingerItem = M5.TP.readFinger(i);
        if (FingerItem.size > 0)
        {
          int touchX = FingerItem.x;
          int touchY = FingerItem.y;

          // 첫 번째 줄 버튼 (이전, 재생/일시정지, 다음) -> mediaBtnY 사용
          if (touchY >= mediaBtnY && touchY <= mediaBtnY + btnSize)
          {
            for (int col = 0; col < 3; col++)
            {
              int btnXStart = xOffset + col * btnPitch;
              if (touchX >= btnXStart && touchX <= btnXStart + btnSize)
              {
                switch (col)
                {
                case 0: // 이전
                  a2dp_sink.previous();
                  break;
                case 1: // 재생/일시정지
                  isPaused = !isPaused;
                  if (isPaused)
                    a2dp_sink.pause();
                  else
                    a2dp_sink.play();
                  break;
                case 2: // 다음
                  a2dp_sink.next();
                  break;
                }
                drawMusicControls();
              }
            }
          }

          // 두 번째 줄 버튼 (Vol-, 음소거, Vol+) -> volumeBtnY 사용
          if (touchY >= volumeBtnY && touchY <= volumeBtnY + btnSize)
          {
            for (int col = 0; col < 3; col++)
            {
              int btnXStart = xOffset + col * btnPitch;
              if (touchX >= btnXStart && touchX <= btnXStart + btnSize)
              {
                switch (col)
                {
                case 0: // 볼륨 -
                  a2dp_sink.set_volume(a2dp_sink.get_volume() - 5);
                  break;
                case 1: // 음소거
                  a2dp_sink.set_volume(0);
                  break;
                case 2: // 볼륨 +
                  a2dp_sink.set_volume(a2dp_sink.get_volume() + 5);
                  break;
                }
                drawMusicControls();
              }
            }
          }
        }
      }
    }
  }
}

void avrc_metadata_callback(uint8_t attribute_id, const uint8_t *value)
{
  String attribute_value = String((const char *)value);
  bool changed = false;

  switch (attribute_id)
  {
  case ESP_AVRC_MD_ATTR_TITLE:
    if (("제목: " + attribute_value) != track_title)
    {
      track_title = "제목: " + attribute_value;
      changed = true;
    }
    break;
  case ESP_AVRC_MD_ATTR_ARTIST:
    if (("가수: " + attribute_value) != track_artist)
    {
      track_artist = "가수: " + attribute_value;
      changed = true;
    }
    break;
  case ESP_AVRC_MD_ATTR_ALBUM:
    if (("앨범: " + attribute_value) != track_album)
    {
      track_album = "앨범: " + attribute_value;
      changed = true;
    }
    break;
  default:
    break;
  }

  if (changed)
  {
    updateMusicInfo();
  }
}

void powerOff()
{
  M5.EPD.Clear(true);
  M5.shutdown();
}

void setup()
{
  M5.begin();
  M5.EPD.SetRotation(1);
  M5.EPD.Clear(true);

  pinMode(G37_PIN, INPUT_PULLUP);
  pinMode(G38_PIN, INPUT_PULLUP);
  pinMode(G39_PIN, INPUT_PULLUP);

  canvas.createCanvas(540, 960);

  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.start("M5Paper Music Display");

  drawMusicControls();
  updateMusicInfo();
}

void loop()
{
  M5.update();
  handleTouch();

  // G37 버튼: 이전곡
  if (digitalRead(G37_PIN) == LOW)
  {
    a2dp_sink.previous();
    drawMusicControls();
    delay(200);
  }

  // G38 버튼: 재생/일시정지 및 전원 종료
  if (digitalRead(G38_PIN) == LOW)
  {
    if (g38PressTime == 0)
    {
      g38PressTime = millis();
      g38LongPressed = false;
    }
    else if (!g38LongPressed && (millis() - g38PressTime > 3000))
    {
      powerOff();
      g38LongPressed = true;
    }
  }
  else
  {
    if (g38PressTime != 0 && !g38LongPressed)
    {
      static bool isPlaying = true;
      if (isPlaying)
      {
        a2dp_sink.pause();
      }
      else
      {
        a2dp_sink.play();
      }
      isPlaying = !isPlaying;
      drawMusicControls();
    }
    g38PressTime = 0;
    g38LongPressed = false;
  }

  // G39 버튼: 다음곡
  if (digitalRead(G39_PIN) == LOW)
  {
    a2dp_sink.next();
    drawMusicControls();
    delay(200);
  }

  delay(10);
}
