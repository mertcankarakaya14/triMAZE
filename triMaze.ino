/*
 * ESP32-S3 + TB6612FNG Wi-Fi (UDP) Motor Kontrol Kodu
 * 
 * Pin Bağlantıları:
 *   AIN1 -> GPIO 4
 *   AIN2 -> GPIO 5
 *   BIN1 -> GPIO 6
 *   BIN2 -> GPIO 7
 *   STBY -> GPIO 21
 */

#include <WiFi.h>
#include <WiFiUdp.h>

// --- WI-FI AĞ BİLGİLERİNİZ ---
const char* ssid = "Wifi Adınız";         // Wi-Fi adınızı buraya yazın
const char* password = "Şifreniz";   // Wi-Fi şifrenizi buraya yazın

const unsigned int UDP_PORT = 4210;
WiFiUDP udp;
char packetBuffer[32];

// Pin Tanımları
#define PIN_AIN1 4
#define PIN_AIN2 5
#define PIN_BIN1 15
#define PIN_BIN2 7
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

void motorlariDurdur() {
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_BIN1, LOW);
  digitalWrite(PIN_BIN2, LOW);
}

void setup() {
  Serial.begin(115200);
  
  // Bilgisayarın Seri Portu tanımasını bekle (Maksimum 4 saniye)
  unsigned long baslangic = millis();
  while (!Serial && (millis() - baslangic < 4000)) {
    delay(10);
  }
  delay(1000);

  Serial.println("\n\n====================================");
  Serial.println(">>> ESP32-S3 CALISMAYA BASLADI <<<");
  Serial.println("====================================");

  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);
  pinMode(PIN_STBY, OUTPUT);

  digitalWrite(PIN_STBY, HIGH);
  motorlariDurdur();

  // Wi-Fi Bağlantısı
  Serial.printf("Wi-Fi agina baglaniliyor: %s\n", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int deneme = 0;
  while (WiFi.status() != WL_CONNECTED && deneme < 40) {
    delay(500);
    Serial.println("Baglanti bekleniyor..."); // Alt satıra geçerek yazdır
    deneme++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[BASARILI] Wi-Fi Baglandi!");
    Serial.print("ESP32 IP Adresi: ");
    Serial.println(WiFi.localIP());
    Serial.printf("UDP Port: %d\n", UDP_PORT);
    udp.begin(UDP_PORT);
  } else {
    Serial.println("\n[HATA] Wi-Fi Baglanamadi! Sifre veya 2.4GHz agi kontrol edin.");
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
        default:
          motorlariDurdur();
          break;
      }
    }
  }

  // Güvenlik zaman aşımı: Sinyal kesilirse motorları durdur
  if (millis() - sonPaketZamani > TIMEOUT_SURESI) {
    motorlariDurdur();
  }
}
