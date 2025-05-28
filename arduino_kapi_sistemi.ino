// =============================================
// KÜTÜPHANELER
// =============================================
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <Servo.h>

// =============================================
// PIN TANIMLAMALARI
// =============================================
// LCD ve Keypad
#define LCD_I2C_ADRES 0x20
#define LCD_SUTUN 16
#define LCD_SATIR 2

// Motorlar ve Sensörler
#define DC_MOTOR_PIN 11          // DC motor PWM pini
#define POT_PIN A3               // Potansiyometre analog pini
#define BUTON_PIN 12             // Ana kontrol butonu pini
#define SICAKLIK_PIN A0          // Sıcaklık sensörü analog pini
#define TURNIKE_PIN 13           // Turnike kontrol pini

// Keypad Pinleri
#define KEYPAD_SATIR 4
#define KEYPAD_SUTUN 4
byte satir_pinleri[KEYPAD_SATIR] = {10, 9, 8, 7};
byte sutun_pinleri[KEYPAD_SUTUN] = {6, 5, 4, 3};

// =============================================
// GLOBAL DEĞİŞKENLER
// =============================================
// LCD ve Keypad Nesneleri
LiquidCrystal_I2C lcd(LCD_I2C_ADRES, LCD_SUTUN, LCD_SATIR);
Servo turnike_servo;

// Keypad Tuş Haritası
char tuslar[KEYPAD_SATIR][KEYPAD_SUTUN] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
Keypad klavye = Keypad(makeKeymap(tuslar), satir_pinleri, sutun_pinleri, KEYPAD_SATIR, KEYPAD_SUTUN);

// Sistem Durum Değişkenleri
int buton_yeni;
int buton_onceki = 1;
int sistem_durumu = 0;          // 0=kapalı, 1=şifre bekleme, 2=turnike açık, 3=fan çalışıyor
int buton_sayisi = 0;           // Kaç kez butona basıldığını takip eder

// Şifre Sistemi Değişkenleri
String dogru_sifre = "1234";    // Doğru şifre
String girilen_sifre = "";      // Kullanıcının girdiği şifre
bool sifre_dogru = false;

// Fan ve Sıcaklık Değişkenleri
int sicaklik_analog;
float sicaklik_celsius;
int fan_hizi;
int onceki_fan_hizi = -1;

// =============================================
// ANA FONKSİYONLAR
// =============================================
void setup() {
  // Seri Haberleşme Başlatma
  Serial.begin(9600);
  
  // LCD Başlatma
  lcd.init();
  lcd.setBacklight(HIGH);
  
  // Başlangıç Mesajı
  lcd.clear();
  lcd.print("HOSGELDINIZ");
  delay(1000);
  
  // Servo Motor Başlatma
  turnike_servo.attach(TURNIKE_PIN);
  turnike_servo.write(0);
  
  // Pin Modları
  pinMode(BUTON_PIN, INPUT);
  pinMode(DC_MOTOR_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);
  pinMode(SICAKLIK_PIN, INPUT);
  
  // Başlangıç Durumları
  analogWrite(DC_MOTOR_PIN, 0);
  
  // Ana Ekran
  lcd.clear();
  lcd.print("GIRIS ICIN");
  lcd.setCursor(0, 1);
  lcd.print("KARTINIZI OKUTUN");
  
  Serial.println("Sistem baslatildi");
}

void loop() {
  // Buton Durumunu Oku
  buton_yeni = digitalRead(BUTON_PIN);
  
  // Buton Basma Kontrolü
  if (buton_onceki == 0 && buton_yeni == 1) {
    buton_sayisi++;
    buton_islemi();
  }
  
  // Sistem Durumuna Göre İşlemler
  switch (sistem_durumu) {
    case 1: // Şifre bekleme
      sifre_kontrol();
      break;
    case 2: // Turnike açık
      turnike_bekle();
      break;
    case 3: // Fan çalışıyor
      fan_kontrol();
      break;
  }
  
  buton_onceki = buton_yeni;
  delay(50); // Debounce
}

// =============================================
// YARDIMCI FONKSİYONLAR
// =============================================
void buton_islemi() {
  if (buton_sayisi == 1) {
    lcd.clear();
    lcd.print("KAPI ACILIYOR...");
    lcd.setCursor(0, 1);
    lcd.print("KAPI ACILDI");
    delay(2000);
    lcd.clear();
    lcd.print("SIFRE GIRIN:");
    sistem_durumu = 1;
    girilen_sifre = "";
  }
  else if (buton_sayisi == 2 && sifre_dogru) {
    lcd.clear();
    lcd.print("TURNIKE ACILIYOR");
    lcd.setCursor(0, 1);
    lcd.print("LUTFEN GECIN...");
    for (int aci = 0; aci <= 90; aci += 5) {
      turnike_servo.write(aci);
      delay(50);
    }
    delay(1500);
    for (int aci = 90; aci >= 0; aci -= 5) {
      turnike_servo.write(aci);
      delay(50);
    }
    sistem_durumu = 2;
  }
}

void sifre_kontrol() {
  char basilan_tus = klavye.getKey();
  if (basilan_tus != NO_KEY) {
    if (basilan_tus == '#') {
      if (girilen_sifre == dogru_sifre) {
        sifre_dogru = true;
        lcd.clear();
        lcd.print("SIFRE DOGRU");
        lcd.setCursor(0, 1);
        lcd.print("KAPI ACILIYOR");
        delay(2000);
        lcd.clear();
        lcd.print("LUTEN PARMAGINIZI");
        lcd.setCursor(0, 1);
        lcd.print("OKUTUN...");
      } else {
        sifre_dogru = false;
        lcd.clear();
        lcd.print("YANLIS SIFRE!");
        lcd.setCursor(0, 1);
        lcd.print("TEKRAR DENEYIN");
        delay(2000);
        lcd.clear();
        lcd.print("SIFRE GIRIN:");
      }
      girilen_sifre = "";
    }
    else if (basilan_tus == '*') {
      girilen_sifre = "";
      lcd.setCursor(0, 1);
      lcd.print("               ");
      lcd.setCursor(0, 1);
      lcd.print("Sifrelendi");
      delay(500);
      lcd.setCursor(0, 1);
      lcd.print("               ");
    }
    else {
      girilen_sifre += basilan_tus;
      lcd.setCursor(0, 1);
      String yildizlar = "";
      for (int i = 0; i < girilen_sifre.length(); i++) {
        yildizlar += "*";
      }
      lcd.print(yildizlar + "               ");
    }
  }
}

void turnike_bekle() {
  delay(3000);
  lcd.clear();
  lcd.print("FAN SISTEMI");
  lcd.setCursor(0, 1);
  lcd.print("BASLATILIYOR...");
  delay(2000);
  sistem_durumu = 3;
}

void fan_kontrol() {
  sicaklik_analog = analogRead(SICAKLIK_PIN);
  sicaklik_celsius = (float)(sicaklik_analog - 105) / 2;
  fan_hizi = map(analogRead(POT_PIN), 0, 1023, 0, 100);
  analogWrite(DC_MOTOR_PIN, (9 + analogRead(POT_PIN) / 5));
  
  if (fan_hizi != onceki_fan_hizi) {
    lcd.clear();
    lcd.print("SICAKLIK:");
    lcd.print((int)sicaklik_celsius);
    lcd.print("C");
    lcd.setCursor(0, 1);
    lcd.print("FAN HIZI:");
    lcd.print(fan_hizi);
    lcd.print("%");
    
    onceki_fan_hizi = fan_hizi;
    
    Serial.print("Sicaklik: ");
    Serial.print((int)sicaklik_celsius);
    Serial.print("C, Fan hizi: ");
    Serial.print(fan_hizi);
    Serial.println("%");
  }
} 