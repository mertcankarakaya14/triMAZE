/*
 * ESP32-S3 + TB6612FNG Wi-Fi (UDP) Motor Kontrol Kodu
 * PWMA ve PWMB -> 3.3V donanımsal bağlı
 */

#include <WiFi.h>
#include <WiFiUdp.h>

// --- WI-FI AĞ BİLGİLERİ ---
const char* ssid = "Mertcan";
const char* password = "MertcanK14";

const unsigned int UDP_PORT = 4210;
WiFiUDP udp;
char packetBuffer[32];

// Pin Tanımları (Kartındaki fiziksel bağlantılara göre ayarla)
#define PIN_AIN1 5
#define PIN_AIN2 4
#define PIN_BIN1 7
#define PIN_BIN2 15
#define PIN_STBY 6

unsigned long sonPaketZamani = 0;
const unsigned long TIMEOUT_SURESI = 800; // 800 ms veri gelmezse güvenlik için durdur

void ileriGit() {
  digitalWrite(PIN_STBY, HIGH);
  digitalWrite(PIN_AIN1, HIGH);
  digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_BIN1, HIGH);
  digitalWrite(PIN_BIN2, LOW);
}

void geriGit() {
  digitalWrite(PIN_STBY, HIGH);
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, HIGH);
  digitalWrite(PIN_BIN1, LOW);
  digitalWrite(PIN_BIN2, HIGH);
}

void solaGit() {
  digitalWrite(PIN_STBY, HIGH);
  digitalWrite(PIN_AIN1, HIGH);
  digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_BIN1, LOW);
  digitalWrite(PIN_BIN2, LOW);
}

void sagaGit() {
  digitalWrite(PIN_STBY, HIGH);
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_BIN1, HIGH);
  digitalWrite(PIN_BIN2, LOW);
}

void motorlariDurdur() {
  // Motorları anında frenlemek / durdurmak için girişleri LOW yap
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_BIN1, LOW);
  digitalWrite(PIN_BIN2, LOW);
}

void setup() {
  Serial.begin(115200);
  
  unsigned long baslangic = millis();
  while (!Serial && (millis() - baslangic < 4000)) {
    delay(10);
  }
  delay(1000);

  Serial.println("\n>>> ESP32-S3 BASLATILDI <<<");

  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);

  // Başlangıçta standby açık, motorlar durur vaziyette
  digitalWrite(PIN_STBY, HIGH);
  motorlariDurdur();

  // Wi-Fi Bağlantısı
  Serial.printf("Ag baglantisi kuruluyor: %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int deneme = 0;
  while (WiFi.status() != WL_CONNECTED && deneme < 40) {
    delay(500);
    Serial.print(".");
    deneme++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[BASARILI] Baglandi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.printf("UDP Port: %d\n", UDP_PORT);
    udp.begin(UDP_PORT);
  } else {
    Serial.println("\n[HATA] Wi-Fi Baglanamadi!");
  }
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);
    if (len > 0) {
      packetBuffer[len] = '\0';
      char komut = packetBuffer[0];
      sonPaketZamani = millis();

      switch (komut) {
        case 'F':
        case 'f':
          ileriGit();
          break;

        case 'B':
        case 'b':
          geriGit();
          break;

        case 'S':
        case 's':
          motorlariDurdur();
          break;

        case 'R':
        case 'r':
         sagaGit();
          break;

        case 'L':
        case 'l':
         solaGit();
          break;

        default:
        motorlariDurdur();
          break;
      }
    }
  }

  // Güvenlik zaman aşımı
  if (millis() - sonPaketZamani > TIMEOUT_SURESI) {
    motorlariDurdur();
  }
}
