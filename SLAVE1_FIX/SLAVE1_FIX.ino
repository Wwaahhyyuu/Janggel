#include "DHT.h"
#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal_I2C.h>

// Definisikan tipe sensor DHT
#define DHTTYPE DHT22 // Atau ganti dengan DHT22 jika menggunakan DHT22

// Pin untuk sensor DHT
#define DHTPIN1 26
#define DHTPIN2 32

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

// Inisialisasi sensor DHT
DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

// mac address tujuan
uint8_t mac_addr_tujuan[] = {0xE4, 0x65, 0xB8, 0xE7, 0x05, 0x1C};

// struktur data yang diterima
typedef struct struct_message {
  char a[32];
  float b;
  float c;
} struct_message;
struct_message data_ku;

// variabel untuk jeda kirim
unsigned long waktu_sebelum, interval=2000;

// variabel peerInfo
esp_now_peer_info_t peerInfo;

// callback Jika data terkirim
void cb_terkirim(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nStatus Pengiriman: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sukses" : "Gagal");
}
 
void setup() {
  Serial.begin(9600);
  Serial.println("Membaca dua sensor DHT!");

  // Memulai sensor
  dht1.begin();
  dht2.begin();
  
  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);

// mengatur esp ke mode station
  WiFi.mode(WIFI_STA);

  // inisialisasi espnow
  if (esp_now_init() != ESP_OK) {
    Serial.println("Gagal Inisialisasi espnow");
    return;
  }

  // mendaftarkan fungsi callback  
  esp_now_register_send_cb(cb_terkirim);
  
  // Mendata Peer / Kawan
  memcpy(peerInfo.peer_addr, mac_addr_tujuan, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Menambahkan Peer / Kawan
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Gagal Menambah Peer");
    return;
  }

}

void loop() {
  // Membaca kelembaban dan suhu dari sensor pertama
  float humidity1 = dht1.readHumidity();
  float temperature1 = dht1.readTemperature();

  // Membaca kelembaban dan suhu dari sensor kedua
  float humidity2 = dht2.readHumidity();
  float temperature2 = dht2.readTemperature();

  // Memeriksa apakah ada pembacaan yang gagal dan mencoba lagi
  if (isnan(humidity1) || isnan(temperature1) || isnan(humidity2) || isnan(temperature2)) {
    Serial.println("Gagal membaca dari sensor DHT!");
    delay(2000); // Tunggu sebelum mencoba lagi
    return;
  }

  // Tampilkan hasil pembacaan dari sensor pertama
  Serial.print("Sensor 1 - Kelembaban: ");
  Serial.print(humidity1);
  Serial.print(" %\t");
  Serial.print("Suhu: ");
  Serial.print(temperature1);
  Serial.println(" *C");

  // Tampilkan hasil pembacaan dari sensor kedua
  Serial.print("Sensor 2 - Kelembaban: ");
  Serial.print(humidity2);
  Serial.print(" %\t");
  Serial.print("Suhu: ");
  Serial.print(temperature2);
  Serial.println(" *C");
  
    data_ku.b = humidity1+humidity2;

    data_ku.c = temperature1+temperature2;
  
    // Mengirim Data
    esp_now_send(mac_addr_tujuan, (uint8_t *) &data_ku, sizeof(data_ku));
  
    // update waktu_sebelum
    waktu_sebelum = millis();
    LCD.setCursor(0, 0);
    LCD.print("Temp = " + String(data_ku.c/2) + " 'C");
    LCD.setCursor(0, 1);
    LCD.print("Hum = " + String(data_ku.b/2) + " %");
      
    delay(10000);
    LCD.clear();
     //delay 30 menit

}
