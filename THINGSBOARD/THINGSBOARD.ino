#include <WiFi.h>
#include <esp_now.h>
#include <LiquidCrystal_I2C.h>
#include <ThingsBoard.h>

// Wi-Fi credentials
const char* ssid = "YourNetworkName"; // Ganti dengan nama SSID WiFi Anda
const char* password = "YourPassword"; // Ganti dengan password WiFi Anda

// ThingsBoard credentials
#define TOKEN "Kxj1j8cP6iDLeg7ZvQx5" // Ganti dengan Device Token dari ThingsBoard
const char* thingsboardServer = "thingsboard.cloud"; // atau IP server ThingsBoard Anda

// Inisialisasi ThingsBoard client
WiFiClient espClient;
ThingsBoard tb(espClient);

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

// Struktur data yang diterima
typedef struct struct_message {
  char a[32];
  float b;
  float c;
} struct_message;
struct_message data_ku;

int totalSuhu = 0;
int totalKelembapan = 0;
int totalDataDiterima = 0;

int relayPin1 = 26;
int relayPin2 = 32;

// Callback jika menerima data
void cb_terima(const uint8_t *mac_addr, const uint8_t *dataDiterima, int panjang) {
  memcpy(&data_ku, dataDiterima, sizeof(data_ku));
  Serial.print("Bytes diterima: ");
  Serial.println(panjang);
  Serial.print("Char: ");
  Serial.println(data_ku.a);
  Serial.print("Kelembapan: ");
  Serial.println(data_ku.b);
  Serial.print("Suhu: ");
  Serial.println(data_ku.c);

  totalSuhu += data_ku.c;
  totalKelembapan += data_ku.b;
  totalDataDiterima++;
  Serial.print("total Data Diterima: ");
  Serial.println(totalDataDiterima);
  
  if (totalDataDiterima == 2) {
    float rataRataSuhu = totalSuhu / 2;
    float rataRataKelembapan = totalKelembapan / 2;

    LCD.setCursor(0, 0);
    LCD.print("Temp = " + String(rataRataSuhu));
    LCD.setCursor(0, 1);
    LCD.print("Klmbp = " + String(rataRataKelembapan));
    delay(5000);
    LCD.clear();

    // Mengirim data ke ThingsBoard
    if (tb.connected()) {
      tb.sendTelemetryFloat("temperature", rataRataSuhu);
      tb.sendTelemetryFloat("humidity", rataRataKelembapan);
    }

    if (rataRataSuhu > 28) {
      Serial.print("Sensor Suhu: ");
      Serial.println(rataRataSuhu);
      digitalWrite(relayPin1, LOW);
      LCD.setCursor(0, 0);
      LCD.print("Kipas Nyala");
    } 
    if (rataRataSuhu < 28) {
      Serial.print("Sensor Suhu: ");
      Serial.println(rataRataSuhu);
      digitalWrite(relayPin1, HIGH);
      LCD.setCursor(0, 0);
      LCD.print("Kipas Mati");
    } 
    if (rataRataKelembapan < 80) {
      Serial.print("Sensor Suhu: ");
      Serial.println(rataRataKelembapan);
      digitalWrite(relayPin2, LOW);
      LCD.setCursor(0, 1);
      LCD.print("Pompa Nyala");
      delay(5000);
    } 
    if (rataRataKelembapan > 80) {
      Serial.print("Sensor Suhu: ");
      Serial.println(rataRataKelembapan);
      digitalWrite(relayPin2, HIGH);
      LCD.setCursor(0, 1);
      LCD.print("Pompa Mati");
      delay(5000);
    }

    totalSuhu = 0;
    totalKelembapan = 0;
    totalDataDiterima = 0;
    LCD.clear();
  }
}

void setup() {
  Serial.begin(115200);

  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  
  // Mengatur ESP ke mode station
  WiFi.mode(WIFI_STA);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  // Inisialisasi ThingsBoard
  if (!tb.connect(thingsboardServer, TOKEN)) {
    Serial.println("Failed to connect to ThingsBoard");
  } else {
    Serial.println("Connected to ThingsBoard");
  }

  // Inisialisasi espnow
  if (esp_now_init() != ESP_OK) {
    Serial.println("Gagal Inisialisasi espnow");
    return;
  }
  
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  
  digitalWrite(relayPin1, LOW);
  digitalWrite(relayPin2, LOW);

  // Mendaftarkan fungsi callback
  esp_now_register_recv_cb(cb_terima);
}

void loop() {
  // Reconnect to Wi-Fi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("WiFi connected");
  }

  // Reconnect to ThingsBoard if disconnected
  if (!tb.connected()) {
    if (!tb.connect(thingsboardServer, TOKEN)) {
      Serial.println("Failed to connect to ThingsBoard");
      return;
    }
  }

  // Process messages
  tb.loop();
}
