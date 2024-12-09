#include <M5EPD.h>
#include <BluetoothA2DPSink.h>
#include "efont.h"
#include "efontEnableAll.h"

#define G37_PIN 37 // G37 버튼 핀 번호
#define G38_PIN 38 // G38 버튼 핀 번호
#define G39_PIN 39 // G39 버튼 핀 번호

M5EPD_Canvas canvas(&M5.EPD);
M5EPD_Canvas infoCanvas(&M5.EPD);
BluetoothA2DPSink a2dp_sink;

int point[2][2];

// 버튼 크기와 간격 조정
int btnSize = 120;
int btnPitch = 130;
int xOffset = 40;
int yOffset = 100;
int PrtOffsetx = 14;
int PrtOffsety = -7;
int btn0 = yOffset + 0 * btnPitch;
int btn1 = yOffset + 1 * btnPitch;

// 음악 정보 저장 변수
String track_title = "제목: 없음";
String track_artist = "가수: 없음";
String track_album = "앨범: 없음";

bool isPaused = false;
unsigned long g38PressTime = 0;
bool g38LongPressed = false;

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

    int width = 16 * textsize;
    if (strUTF16 < 0x0100)
    {
      width = 8 * textsize;
    }

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

void updateMusicInfo()
{
  const int TEXT_Y_START = 50;
  const int LINE_SPACING = 50;
  const int INFO_HEIGHT = TEXT_Y_START + LINE_SPACING * 3; // 음악 정보 영역의 높이

  // 음악 정보 전용 캔버스 생성
  infoCanvas.createCanvas(540, INFO_HEIGHT);
  infoCanvas.fillCanvas(15); // 검은색 배경

  // 음악 정보 표시
  printEfont(&infoCanvas, track_title.c_str(), xOffset, TEXT_Y_START, 2, 0);
  printEfont(&infoCanvas, track_artist.c_str(), xOffset, TEXT_Y_START + LINE_SPACING, 2, 0);
  printEfont(&infoCanvas, track_album.c_str(), xOffset, TEXT_Y_START + LINE_SPACING * 2, 2, 0);

  // 음악 정보 영역만 부분 업데이트
  infoCanvas.pushCanvas(0, 0, UPDATE_MODE_DU4); // DU4 모드로 변경하여 더 빠른 업데이트
}

void drawMusicControls()
{
  canvas.createCanvas(540, 960);
  canvas.fillCanvas(15);

  // 텍스트 정보 표시 (상단)
  const int TEXT_Y_START = 50;
  const int LINE_SPACING = 50;

  printEfont(&canvas, track_title.c_str(), xOffset, TEXT_Y_START, 2, 0);
  printEfont(&canvas, track_artist.c_str(), xOffset, TEXT_Y_START + LINE_SPACING, 2, 0);
  printEfont(&canvas, track_album.c_str(), xOffset, TEXT_Y_START + LINE_SPACING * 2, 2, 0);

  // 버튼 위치 조정
  const int BUTTON_Y_START = TEXT_Y_START + LINE_SPACING * 3 + 50;
  btn0 = BUTTON_Y_START;
  btn1 = BUTTON_Y_START + btnSize + 20; // 두 번째 줄 버튼의 Y 좌표

  // 첫 번째 줄: 이전, 일시정지/재생, 다음
  for (int i = 0; i < 3; i++)
  {
    canvas.drawRoundRect(xOffset + i * btnPitch, btn0, btnSize, btnSize, 10, 15);
  }
  printEfont(&canvas, "이전", xOffset + 0 * btnPitch + PrtOffsetx, btn0 + btnSize / 2 + PrtOffsety, 2, 0);
  printEfont(&canvas, isPaused ? "재생" : "일시정지", xOffset + 1 * btnPitch + PrtOffsetx - 10, btn0 + btnSize / 2 + PrtOffsety, 2, 0);
  printEfont(&canvas, "다음", xOffset + 2 * btnPitch + PrtOffsetx, btn0 + btnSize / 2 + PrtOffsety, 2, 0);

  // 두 번째 줄: 볼륨 -, 음소거, 볼륨 +
  for (int i = 0; i < 3; i++)
  {
    canvas.drawRoundRect(xOffset + i * btnPitch, btn1, btnSize, btnSize, 10, 15);
  }
  printEfont(&canvas, "볼륨 -", xOffset + 0 * btnPitch + PrtOffsetx - 5, btn1 + btnSize / 2 + PrtOffsety, 2, 0);
  printEfont(&canvas, "음소거", xOffset + 1 * btnPitch + PrtOffsetx - 5, btn1 + btnSize / 2 + PrtOffsety, 2, 0);
  printEfont(&canvas, "볼륨 +", xOffset + 2 * btnPitch + PrtOffsetx - 5, btn1 + btnSize / 2 + PrtOffsety, 2, 0);

  canvas.pushCanvas(0, 0, UPDATE_MODE_GC16);
}

void handleTouch()
{
  if (M5.TP.available())
  {
    if (!M5.TP.isFingerUp())
    {
      M5.TP.update();
      for (int i = 0; i < 2; i++)
      {
        tp_finger_t FingerItem = M5.TP.readFinger(i);
        if ((point[i][0] != FingerItem.x) || (point[i][1] != FingerItem.y))
        {
          point[i][0] = FingerItem.x;
          point[i][1] = FingerItem.y;

          int controlYOffset = 280;
          int btnY;

          // 왼쪽 열 버튼 (이전, 재생, 다음)
          for (int j = 0; j < 3; j++)
          {
            btnY = controlYOffset + 30 + j * btnPitch;
            if (point[i][0] >= xOffset && point[i][0] <= xOffset + btnSize &&
                point[i][1] >= btnY && point[i][1] <= btnY + btnSize)
            {
              switch (j)
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
            }
          }

          // 오른쪽 열 버튼 (볼륨+, 음소거, 볼륨-)
          for (int j = 0; j < 3; j++)
          {
            btnY = controlYOffset + 30 + j * btnPitch;
            if (point[i][0] >= xOffset + btnPitch && point[i][0] <= xOffset + btnPitch + btnSize &&
                point[i][1] >= btnY && point[i][1] <= btnY + btnSize)
            {
              switch (j)
              {
              case 0: // 볼륨+
                a2dp_sink.set_volume(a2dp_sink.get_volume() + 5);
                break;
              case 1: // 음소거
                a2dp_sink.set_volume(0);
                break;
              case 2: // 볼륨-
                a2dp_sink.set_volume(a2dp_sink.get_volume() - 5);
                break;
              }
            }
          }
          drawMusicControls();
        }
      }
    }
  }
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

  updateMusicInfo();
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

  pinMode(G37_PIN, INPUT_PULLUP); // G37 버튼 입력 설정
  pinMode(G38_PIN, INPUT_PULLUP); // G38 버튼 입력 설정
  pinMode(G39_PIN, INPUT_PULLUP); // G39 버튼 입력 설정

  canvas.createCanvas(540, 960);
  a2dp_sink.set_avrc_metadata_callback(avrc_metadata_callback);
  a2dp_sink.start("M5Paper Music Display");

  drawMusicControls();
}

void loop()
{
  M5.update();
  handleTouch();

  // G37 버튼: 이전곡
  if (digitalRead(G37_PIN) == LOW)
  {
    a2dp_sink.previous(); // 이전곡 함수 호출
    delay(200);           // 디바운싱을 위한 지연
  }

  // G38 버튼: 재생/일시정지 및 전원 종료
  if (digitalRead(G38_PIN) == LOW)
  {
    if (g38PressTime == 0)
    {
      g38PressTime = millis(); // 눌린 시간 기록 시작
      g38LongPressed = false;
    }
    else if (!g38LongPressed && (millis() - g38PressTime > 3000))
    {
      powerOff(); // 전원 종료
      g38LongPressed = true;
    }
  }
  else
  {
    if (g38PressTime != 0 && !g38LongPressed)
    {
      // 짧게 눌렀을 때 재생/일시정지 토글
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
    }
    g38PressTime = 0;
    g38LongPressed = false;
  }

  // G39 버튼: 다음곡
  if (digitalRead(G39_PIN) == LOW)
  {
    a2dp_sink.next(); // 다음곡 함수 호출
    delay(200);       // 디바운싱을 위한 지연
  }

  delay(10);
}