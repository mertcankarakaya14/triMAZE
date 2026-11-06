import cv2
import mediapipe as mp
import socket
import time

# --- WI-FI HEDEF AYARLARI ---
ESP_IP = "10.69.10.113"  # ESP32 IP adresi
UDP_PORT = 4210

# UDP Soketi
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

# Kamerayı başlat
cap = cv2.VideoCapture(0)

# Parmak Uçları ve Eklem Noktaları
# [Basparmak, Isaret, Orta, Yuzuk, Serce]
TIP_IDS = [4, 8, 12, 16, 20]
PIP_IDS = [2, 6, 10, 14, 18]

son_komut = ""
son_gonderim = 0

def udp_komut_gonder(komut):
    global son_komut, son_gonderim
    suan = time.time()
    if komut != son_komut or (suan - son_gonderim > 0.25):
        try:
            sock.sendto(komut.encode(), (ESP_IP, UDP_PORT))
        except Exception as e:
            print("UDP Hata:", e)
        son_komut = komut
        son_gonderim = suan

print("Sistem baslatiliyor...")

while True:
    success, img = cap.read()
    if not success or img is None:
        print("[HATA] Kameradan goruntu alinamadi, bekleniyor...")
        time.sleep(0.1)
        continue

    img = cv2.flip(img, 1)
    h, w, _ = img.shape
    img_rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    
    komut = 'S'
    durum_metni = "DUR (El Yok)"
    renk = (0, 0, 255)

    try:
        results = hands.process(img_rgb)
        
        if results.multi_hand_landmarks:
            for hand_landmarks in results.multi_hand_landmarks:
                mp_draw.draw_landmarks(img, hand_landmarks, mp_hands.HAND_CONNECTIONS)
                lm = hand_landmarks.landmark

                # 4 parmak kontrolü (İşaret, Orta, Yüzük, Serçe)
                parmaklar = []
                for i in range(1, 5):
                    # Y koordinatında uç nokta eklemden yukarıdaysa (ekranda Y aşağı doğru artar)
                    if lm[TIP_IDS[i]].y < lm[PIP_IDS[i]].y:
                        parmaklar.append(1)
                    else:
                        parmaklar.append(0)

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

                # 3 Parmak -> SAĞA
                elif isaret == 1 and orta == 1 and yuzuk == 1 and serce == 0:
                    komut = 'R'
                    durum_metni = "SAGA (3 Parmak)"
                    renk = (255, 165, 0)

                # 4 Parmak -> SOLA
                elif isaret == 1 and orta == 1 and yuzuk == 1 and serce == 1:
                    komut = 'L'
                    durum_metni = "SOLA (4 Parmak)"
                    renk = (255, 0, 255)

                # Diğer durumlar veya Yumruk -> DUR
                else:
                    komut = 'S'
                    durum_metni = "DUR (Yumruk)"
                    renk = (0, 0, 255)

    except Exception as e:
        print("MediaPipe Hata:", e)

    udp_komut_gonder(komut)

    # Arayüz Paneli
    cv2.rectangle(img, (20, 20), (380, 105), (0, 0, 0), -1)
    cv2.putText(img, f"Hedef: {ESP_IP}:{UDP_PORT}", (30, 45), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.5, (180, 180, 180), 1)
    cv2.putText(img, f"DURUM: {durum_metni}", (30, 75), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.65, renk, 2)
    cv2.putText(img, f"Komut: '{komut}'", (30, 98), 
                cv2.FONT_HERSHEY_SIMPLEX, 0.55, (220, 220, 220), 1)

    cv2.imshow("ESP32 Kontrol Paneli", img)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

udp_komut_gonder('S')
sock.close()
cap.release()
cv2.destroyAllWindows()
