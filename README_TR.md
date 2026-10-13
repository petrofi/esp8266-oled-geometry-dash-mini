# Geometry Dash Mini — ESP8266 + SH1106 OLED — Telefondan Oynama

## Malzemeler
- ESP8266 NodeMCU veya Wemos D1 Mini (ESP8266)
- 1.3 inç 128×64 SH1106 I2C OLED (4 pinli)
- 4 jumper kablosu, USB veri kablosu ve telefon

**Harici sensör, buton veya joystick gerekmiyor.** Telefonda ekrana dokunarak zıplarsın. OLED oyunu gösterir.

## Kablolar
| OLED | ESP8266 | GPIO |
|------|---------|------|
| GND | GND | - |
| VCC | 3V3 | - |
| SCL | D1 | GPIO5 |
| SDA | D2 | GPIO4 |

ÖNEMLİ: Ekranı 3.3 V ile besle; OLED VCC pinini 5 V hattına bağlama. Bu bağlantı 4 pinli I2C SH1106 içindir, SPI ekran değil.

## Arduino IDE
1. Arduino IDE Preferences > Additional Boards Manager URLs:
   https://arduino.esp8266.com/stable/package_esp8266com_index.json
2. Boards Manager'dan ESP8266 by ESP8266 Community yükle.
3. Library Manager'dan `Adafruit SH110X` ve `Adafruit GFX Library` yükle. `Adafruit BusIO` bağımlılığını da yükle.
4. `GeometryDashMini_ESP8266_Telefon.ino` dosyasını aç.
5. Tools > Board'dan `NodeMCU 1.0 (ESP-12E Module)` veya kullandığın Wemos D1 mini'yi seç, doğru COM portunu belirle.
6. Upload'a bas. OLED'de GEOMETRY DASH MINI yazısı çıkmalı.

## Telefon ile oyun
- Telefondan Wi-Fi ağına bağlan: `GeometryDash-ESP8266`
- Şifre: `geomdash123`
- Telefon tarayıcısında aç: `http://192.168.4.1`
- "OYUNU BAŞLAT" ve "ZIPLA!" düğmelerini kullan.
- İnternet bağlantısı gerekmiyor. "İnternet yok" uyarısında bu Wi-Fi'da kal, gerekirse mobil veriyi geçici kapat.

## Oyun içeriği
- Zıplayan küçük kare karakter
- Hareketli sivri engeller ve ilerleyen seviyelerde bloklar
- Hayatta kalma skoru ve geçilen engel bonusu
- Skor arttıkça hız ve zorluk artışı
- En yüksek skor EEPROM'da saklanır (güç kesilince silinmez)
- Çarpışınca Game Over ve yeniden başlatma
- Ekran yalnızca OLED'de; telefonda kontrol arayüzü ve skor görünür.

## Sorun giderme
- OLED açılmıyorsa SCL/SDA yerleşimini ve ekranın SH1106 olduğunu kontrol et. Gerekirse `OLED_ADDRESS = 0x3C` satırını `0x3D` yap.
- Görüntü kararsızsa `Wire.setClock(400000)` yerine `Wire.setClock(100000)` dene.
- Telefondan sayfa açılmıyorsa ESP8266 Wi-Fi ağına bağlı olduğundan emin ol, adresi `http://192.168.4.1` olarak yaz ve mobil veriyi kapatmayı dene.
- COM portu görünmüyorsa USB veri kablosu ve CH340/CP210x sürücüsünü kontrol et.
- 1.3 inç ekranın pin sayısı 7 ise bu dosya o SPI modele uygun değildir.

## Teknik not
Bu paket yazılım kontrolü için hazırlanmıştır. Gerçek ESP8266/SH1106 üzerinde donanımsal oynanış ve Arduino kart paketiyle tam derleme testi kullanıcının cihazında yapılmalıdır.
