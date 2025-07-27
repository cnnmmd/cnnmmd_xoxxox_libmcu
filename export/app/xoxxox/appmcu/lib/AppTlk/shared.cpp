//--------------------------------------------------------------------------
// 参照

#include "shared.h"

//--------------------------------------------------------------------------
// 設定

// 設定：通信（WiFi）
WiFiClient clitcp;
// 設定：通信（HTTP）
HTTPClient cliweb;
String urlrcv = String("http://") + srvadr + ":" + String(srvprt) + pthrcv;
// 設定：音声
const int ratsmp = 16000;
const int lenbff_snd = ratsmp * secrec * 2;
const int lenbff_rcv = 1024;

//--------------------------------------------------------------------------
// 機能（個別）：環境

// 状態を表示
void setmsg(char* string) {
  M5.Lcd.setCursor(0, 0); 
  M5.Lcd.println(string);
}

//--------------------------------------------------------------------------
// 機能（個別）：音声

// 音量を調整
void modvol(uint8_t* buffer, size_t length, float volume) {
  for (size_t i = 0; i < length; i += 4) { // 処理単位：4 bytes (32 bits)
    int32_t sample = (buffer[i+3] << 24) | (buffer[i+2] << 16) | (buffer[i+1] << 8) | buffer[i]; // データを格納（32 bits）
    sample = static_cast<int32_t>(sample * volume); // 音量調整（volume: 0.0 - 1.0）
    int8_t smpscl = sample >> 24; // スケーリング（符号なし／0-255 ）：PCM (32 bits) -> DAC (8 bits)
    memset(buffer + i, 0, 3); // リセット（0 ）：buffer[i], buffer[i+1], buffer[i+2]
    buffer[i + 3] = static_cast<uint8_t>(smpscl + 128); // 加算（128 ）※ＤＡＣの範囲に対応
  }
}

//--------------------------------------------------------------------------
// 機能（全体）

// 環境
void inienv() {
  M5.begin();
  M5.Power.begin();
  SPIFFS.begin(true);
  Serial.begin(115200);
}

// 通信：接続（WiFi）
void cnnnet() {
  WiFi.begin(wifiid, wifipw);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("bgn: connect wifi"); // DBG
  }
  Serial.println("end: connect wifi"); // DBG
}

// 通信：送信：音声（HTTP）
void sndvce() {
  // 読み出し：ファイル
  File filrec = SPIFFS.open(pthaud, FILE_READ);
  if (!filrec) {
    Serial.println("err: open spiffs"); // DBG
    return;
  }
  size_t lenfil = filrec.size();
  // 接続：サーバ
  if (clitcp.connect(srvadr, srvprt)) {
    Serial.println("bgn: send data to server"); // DBG
    // 送信：ヘッダ
    clitcp.print("POST ");
    clitcp.print(pthsnd);
    clitcp.println(" HTTP/1.1");
    clitcp.print("Host: ");
    clitcp.println(srvadr);
    clitcp.println("Content-Type: application/octet-stream");
    clitcp.print("Content-Length: ");
    clitcp.println(lenfil);
    clitcp.println();
    // 送信：データ
    uint8_t buffer[512];
    size_t difbff;
    while ((difbff = filrec.read(buffer, sizeof(buffer))) > 0) {
      clitcp.write(buffer, difbff);
    }
    Serial.println("end: send data to server"); // DBG
  }
  else {
    Serial.println("err: connect server"); // DBG
  }
  filrec.close();
  clitcp.stop();
}

// 音声：初期化：送信用：ＤＡＣ（I2S ）
void inii2s_snd() {
  i2s_config_t cfgi2s = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
    .sample_rate = ratsmp,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 512,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };
  i2s_pin_config_t cfgpin = {
    .bck_io_num = I2S_PIN_NO_CHANGE,
    .ws_io_num = 22,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = 21,
  };
  i2s_driver_install(I2S_NUM_0, &cfgi2s, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &cfgpin);
}

// 音声：録音
void recvce() {
  File filrec = SPIFFS.open(pthaud, FILE_WRITE);
  if (!filrec) {
    Serial.println("err: open spiffs"); // DBG
    return;
  }
  size_t difbff;
  uint8_t bffi2s[512]; // 一時バッファ

  Serial.println("bgn: record sound"); // DBG
  for (int i = 0; i < lenbff_snd; i += difbff) {
    i2s_read(I2S_NUM_0, (void*)bffi2s, 512, &difbff, portMAX_DELAY);
    filrec.write(bffi2s, difbff);
  }
  filrec.close();
  Serial.println("end: record sound"); // DBG
}

// 音声：初期化：受信用：ＤＡＣ（I2S ）
void inii2s_rcv() {
  i2s_driver_config_t cfgi2s = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN), // ＤＡＣビルトインモード
    .sample_rate = 16000, // サンプリングレート
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, 
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, 
    .dma_buf_count = 16,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true
  };
  i2s_driver_install(I2S_NUM_0, &cfgi2s, 0, NULL);
  i2s_set_pin(I2S_NUM_0, NULL);
  i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);
  i2s_zero_dma_buffer(I2S_NUM_0);
}

// 音声：停止〜破棄
void endi2s() {
  i2s_stop(I2S_NUM_0);
  i2s_driver_uninstall(I2S_NUM_0);
}
