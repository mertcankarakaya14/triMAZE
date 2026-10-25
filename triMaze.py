import cv2
import mediapipe as mp
import socket
import time

# --- WI-FI HEDEF AYARLARI ---
# ESP32 Seri Monitöründe görünen IP adresini buraya yazın:
ESP_IP = "10.220.161.113"  # Örnek IP: ESP32'nizin IP adresiyle değiştirin
UDP_PORT = 4210

# UDP Soketi (Sıfır gecikmeli kablosuz aktarım)
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# MediaPipe El Takibi
mp_hands = mp.solutions.hands
hands = mp_hands.Hands(
    static_image_mode=False,
    max_num_hands=1,
    min_detection_confidence=0.7,
    min_tracking_confidence=0.6
)
mp_draw = mp.solutions.drawing_utils

cap = cv2.VideoCapture(0)

# Parmak uçları ve bir alt eklem indeksleri
TIP_IDS = [4, 8, 12, 16, 20]
PIP_IDS = [3, 6, 10, 14, 18]

son_komut = ""
son_gonderim = 0

def udp_komut_gonder(komut):
    global son_komut, son_gonderim
    suan = time.time()
    # Komut değiştiğinde veya her 0.25 saniyede bir paket tazele
    if komut != son_komut or (suan - son_gonderim > 0.25):
        try:
            sock.sendto(komut.encode(), (ESP_IP, UDP_PORT))
        except Exception as e:
            print("UDP Gönderim Hatası:", e)
        son_komut = komut
        son_gonderim = suan

print(f"Kamera başlatıldı. Hedef: {ESP_IP}:{UDP_PORT}")
print("Çıkış için 'q' tuşuna basınız.")

while cap.isOpened():
    success, img = cap.read()
    if not success:
        break

    img = cv2.flip(img, 1) # Aynalama
    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    results = hands.process(img_rgb)

    komut = 'S'
    durum_metni = "DUR (El Yok)"
    renk = (0, 0, 255)

    if results.multi_hand_landmarks:
        for hand_landmarks in results.multi_hand_landmarks:
            mp_draw.draw_landmarks(img, hand_landmarks, mp_hands.HAND_CONNECTIONS)
            lm = hand_landmarks.landmark

            parmaklar = []
            for i in range(1, 5):
                parmaklar.append(1 if lm[TIP_IDS[i]].y < lm[PIP_IDS[i]].y else 0)

            isaret, orta, yuzuk, serce = parmaklar

            # 1 Parmak -> İLERİ
            if isaret == 1 and orta == 0 and yuzuk == 0 and serce == 0:
                komut = 'F'
                durum_metni = "ILERI (1 Parmak)"
                renk = (0, 255, 0)
            # 2 Parmak -> GERİ
            elif isaret == 1 and orta == 1 and yuzuk == 0 and serce == 0:
                komut = 'B'
                durum_metni = "GERI (2 Parmak)"
                renk = (0, 255, 255)
            # Yumruk veya El Yok -> DUR
            else:
                komut = 'S'
                durum_metni = "DUR (Yumruk)"
                renk = (0, 0, 255)

    udp_komut_gonder(komut)

    # Ekran Bilgi Paneli
    cv2.rectangle(img, (20, 20), (410, 105), (0, 0, 0), -1)
    cv2.putText(img, f"Hedef: {ESP_IP}:{UDP_PORT}", (30, 45), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (180, 180, 180), 1)
    cv2.putText(img, f"DURUM: {durum_metni}", (30, 75), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, renk, 2)
    cv2.putText(img, f"Komut: '{komut}'", (30, 98), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.55, (220, 220, 220), 1)

    cv2.imshow("ESP32-S3 Wi-Fi MediaPipe Kontrol", img)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

udp_komut_gonder('S')
sock.close()
cap.release()
cv2.destroyAllWindows()
