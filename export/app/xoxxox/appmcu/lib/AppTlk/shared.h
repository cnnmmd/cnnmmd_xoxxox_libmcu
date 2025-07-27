//--------------------------------------------------------------------------

#ifndef SHARED
#define SHARED

//--------------------------------------------------------------------------
// 参照

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <driver/i2s.h>
#include <SPIFFS.h>
#include <M5Stack.h>

//--------------------------------------------------------------------------
// 定義

// 定義：通信（WiFi）
extern WiFiClient clitcp;
// 定義：通信（HTTP）
extern HTTPClient cliweb;
extern String urlrcv;
// 定義：音声
extern const int ratsmp;
extern const int lenbff_snd;
extern const int lenbff_rcv;

//--------------------------------------------------------------------------
// 定義：個別

// 通信
extern const char* wifiid;
extern const char* wifipw;
extern const char* srvadr;
extern const uint16_t srvprt;
extern const char* pthsnd;
extern const char* pthrcv;

// 音声
extern const char* pthaud; // 音声ファイルの格納場所（一時的）
extern const int secrec; // 録音時間（秒）
extern const float volume; // 調整：音量

//--------------------------------------------------------------------------
// 機能（個別）

// 機能（個別）：環境
void setmsg(char* string);
// 機能（個別）：音声
void modvol(uint8_t* buffer, size_t length, float volume);

// 機能（全体）

// 環境
void inienv();
// 通信
void cnnnet();
void sndvce();
// 音声
void inii2s_snd();
void recvce();
void inii2s_rcv();
void endi2s();

//--------------------------------------------------------------------------

#endif // SHARED
