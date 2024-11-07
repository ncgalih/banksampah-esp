#include <EasyNextionLibrary.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Wire.h>

// Pin definisi
#define SS_PIN 21
#define RST_PIN 22

#define SDA_PIN 15  // Pin SDA baru
#define SCL_PIN 14  // Pin SCL baru
#define I2C_ADDRESS 8  // Alamat I2C untuk slave ESP32

#define TRIG_PIN_A 4
#define ECHO_PIN_A 5

MFRC522 rfid(SS_PIN, RST_PIN);

EasyNex myNex(Serial2);  // Gunakan Serial2 jika menggunakan ESP32

const char* ssid = "Niken";           // Replace with your Wi-Fi SSID
const char* password = "12345678";

String currentUser = "";
String currentAlamat = "";
int currentSaldo = 0;
String cardID = "";
boolean accessGranted = false;
String currentType = "";
String currentSubType = "";
int countItem = 0;

// ============== WEIGHT STORAGE VALUES =================
float previousWeightA = 0, previousWeightB = 0, previousWeightC = 0, previousWeightD = 0;
float currentWeightA = 0, currentWeightB = 0, currentWeightC = 0, currentWeightD = 0;
float newWeightA = 0, newWeightB = 0, newWeightC = 0, newWeightD = 0;
// Dari ESP32 Slave
float storagePlastic = 0.0;
float storagePaper = 0.0;
float storageMetal = 0.0;
float storageGlass = 0.0;

// Data rata-rata berat jenis
float rerataBeratJenis = 0.01;

boolean loginRFID(String rfid){
    HTTPClient http;

    String serverUrl = "https://banksampah-be.vercel.app/user/login/";

    http.begin(serverUrl + rfid); // Initialize HTTP connection
    int httpResponseCode = http.GET(); // Send the request

    if (httpResponseCode == 200) { // Check if request was successful
      String payload = http.getString();
      Serial.println("HTTP Response Code: " + String(httpResponseCode));
      Serial.println("Response payload: " + payload);

      // Parse JSON payload
      StaticJsonDocument<1024> jsonDoc; // Adjust the size based on JSON response size
      DeserializationError error = deserializeJson(jsonDoc, payload);

      if (error) {
        Serial.print("JSON Parsing Error: ");
        Serial.println(error.c_str());
        return false;
      }

      // Access JSON data (Assuming JSON format is {"key1": "value1", "key2": "value2"})
      currentUser = jsonDoc["nama"].as<String>();;
      currentAlamat = jsonDoc["alamat"].as<String>();;
      currentSaldo = jsonDoc["saldo"].as<int>();;

      return true;
      http.end();
    } 
    else {
      Serial.println("Error on HTTP request: " + String(httpResponseCode));
      return false;
      http.end();
    }
    http.end(); // Close the connection  
}

// Fungsi untuk cek kartu RFID
bool checkRFID() {
  if (!rfid.PICC_IsNewCardPresent()) return false;
  if (!rfid.PICC_ReadCardSerial()) return false;

  cardID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    cardID += String(rfid.uid.uidByte[i], HEX);
  }
  cardID.toUpperCase();

  Serial.print("Kartu ditemukan dengan ID: ");
  Serial.println(cardID);

  return true;
}

int currentPage = 0;
void changePage(int page){
  currentPage = page;
  myNex.writeStr("page " + String(page));
}

void Logout(){
  accessGranted = false;
  currentUser = "";
  currentAlamat = "";
  currentSaldo = 0;
  myNex.writeStr("t1.txt", "");
  myNex.writeStr("t2.txt", "");
}

void InfoSaldo(){
  myNex.writeStr("t1.txt", String(currentSaldo));
}

void tarikTunai(int amount){
  Serial.println("tarik tunai");
  if (amount > currentSaldo) {
    Serial.println("Saldo tidak cukup untuk tarik tunai");
    // Tambahkan kode untuk menampilkan pesan error di HMI jika saldo tidak cukup
    return;
  }
  HTTPClient http;

  String serverUrl = "https://banksampah-be.vercel.app/user/tarik";

  http.begin(serverUrl); // Initialize HTTP connection
  http.addHeader("Content-Type", "application/json"); // Specify JSON content type

    // Create JSON object
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["rfid_user"] = cardID; // Replace with actual RFID data
  jsonDoc["nominal"] = amount;       // Replace with actual saldo

    // Serialize JSON to string
  String requestBody;
  serializeJson(jsonDoc, requestBody);

    // Send HTTP POST request
  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode == 201) { // Check if request was successful
    String payload = http.getString();
    Serial.println("HTTP Response Code: " + String(httpResponseCode));
    Serial.println("Response payload: " + payload);
    changePage(4);
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);     // Pastikan baud rate sesuai dengan pengaturan di Nextion Editor
  myNex.begin();

  changePage(0);

  Wire.begin(SDA_PIN, SCL_PIN);
  
  // Atur teks pada komponen Nextion
  myNex.writeStr("t0", "tes nama nasabah");

  SPI.begin();
  rfid.PCD_Init();

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (!accessGranted) {
    if (checkRFID()) {
      if (loginRFID(cardID)){
        accessGranted = true;

        Serial.print("Nama: ");
        Serial.println(currentUser);
        Serial.print("Alamat: ");
        Serial.println(currentAlamat);
        Serial.print("Saldo: ");
        Serial.println(currentSaldo);

        myNex.writeStr("t1.txt", currentUser);
        myNex.writeStr("t2.txt", currentAlamat);
        delay(1000);
        changePage(1);           
      }
      else {
        Serial.println("kartu tidak terdaftar");
        myNex.writeStr("t1.txt", cardID);
        myNex.writeStr("t2.txt", "Kartu tidak terdaftar");
        delay(2000);
      }
    }
  }
  int newPage = myNex.readNumber("dp");
  Serial.print(newPage);
  Serial.print(" ");
  bool changedPage = currentPage != newPage;
  delay(50);
  if(true){
    switch(newPage){
      case 0:
        if (changedPage) Logout();
        break;
      case 2:
        if (changedPage) InfoSaldo();
        break;
      case 3:
        if(myNex.readNumber("b4.val") == 1){
          tarikTunai(myNex.readNumber("n0.val"));
          myNex.writeNum("b4.val", 0);
        }
        break;
      case 5:
        if (changedPage) mutasiRekening();
        break;
      case 7:
        currentType = "plastik";
        if (myNex.readNumber("setor_plastik.b0.val") == 1) {  // Botol Bening
            Serial.println("Gambar Botol Bening Ditekan");
            currentSubType = "PET Botol Bening";
            Wire.beginTransmission(I2C_ADDRESS);
            Serial.println("Buka servo plastik");
            Wire.write(1);
            Wire.endTransmission();  
            myNex.writeNum("setor_plastik.b0.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_plastik.b1.val") == 1) {  // Botol Warna
            Serial.println("Gambar Botol Warna Ditekan");
            currentSubType = "PET Botol Warna";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(1);
            Wire.endTransmission();  
            myNex.writeNum("setor_plastik.b1.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_plastik.b2.val") == 1) {  // Plastik Kemasan
            Serial.println("Gambar Plastik Kemasan Ditekan");
            currentSubType = "Plastik Kemasan";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(1);
            Wire.endTransmission();  
            myNex.writeNum("setor_plastik.b2.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_plastik.b3.val") == 1) {  // Tutup Botol
            Serial.println("Gambar Tutup Botol Ditekan");
            currentSubType = "Tutup Botol";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(1);
            Wire.endTransmission();  
            myNex.writeNum("setor_plastik.b3.val", 0);
            changePage(11);
        }
        break;
      case 8:
        currentType = "kertas";
        if (myNex.readNumber("setor_kertas.b0.val") == 1) {  // Arsip
            Serial.println("Gambar Arsip Ditekan");
            currentSubType = "Arsip (hvs, buku)";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(2);
            Wire.endTransmission();
            myNex.writeNum("setor_kertas.b0.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_kertas.b1.val") == 1) {  // Tetra Pack
            Serial.println("Gambar Tetra Pack Ditekan");
            currentSubType = "Tetra Pack";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(2);
            Wire.endTransmission();
            myNex.writeNum("setor_kertas.b1.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_kertas.b2.val") == 1) {  // Kardus
            Serial.println("Gambar Kardus Ditekan");
            currentSubType = "Kardus";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(2);
            Wire.endTransmission();
            myNex.writeNum("setor_kertas.b2.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_kertas.b3.val") == 1) {  // Majalah
            Serial.println("Gambar Majalah Ditekan");
            currentSubType = "Majalah/Koran";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(2);
            Wire.endTransmission();
            myNex.writeNum("setor_kertas.b3.val", 0);
            changePage(11);
        }
        break;
      case 9:
        currentType = "logam";
        if (myNex.readNumber("setor_logam.b0.val") == 1) {  // Seng
            Serial.println("Gambar Seng Ditekan");
            currentSubType = "Seng (kaleng)";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(3);
            Wire.endTransmission();
            myNex.writeNum("setor_logam.b0.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_logam.b1.val") == 1) {  // Besi
            Serial.println("Gambar Besi Ditekan");
            currentSubType = "Besi";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(3);
            Wire.endTransmission();
            myNex.writeNum("setor_logam.b1.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_logam.b2.val") == 1) {  // Aluminium
            Serial.println("Gambar Aluminium Ditekan");
            currentSubType = "Aluminium";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(3);
            Wire.endTransmission();
            myNex.writeNum("setor_logam.b2.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_logam.b3.val") == 1) {  // Tembaga
            Serial.println("Gambar Tembaga Ditekan");
            currentSubType = "Tembaga";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(3);
            Wire.endTransmission();
            myNex.writeNum("setor_logam.b3.val", 0);
            changePage(11);
        }
        break;
      case 10:
        currentType = "kaca";
        if (myNex.readNumber("setor_kaca.b0.val") == 1) {  // Beling
            Serial.println("Gambar Beling Ditekan");
            currentSubType = "Tembaga";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(4);
            Wire.endTransmission();
            myNex.writeNum("setor_kaca.b0.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_kaca.b1.val") == 1) {  // Botol Kecap
            Serial.println("Gambar Botol Kecap Ditekan");
            currentSubType = "Tembaga";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(4);
            Wire.endTransmission();
            myNex.writeNum("setor_kaca.b1.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_kaca.b2.val") == 1) {  // Botol Utuh
            Serial.println("Gambar Botol Utuh Ditekan");
            currentSubType = "Tembaga";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(4);
            Wire.endTransmission();
            myNex.writeNum("setor_kaca.b2.val", 0);
            changePage(11);
        }

        if (myNex.readNumber("setor_kaca.b3.val") == 1) {  // Botol Hijau
            Serial.println("Gambar Botol Hijau Ditekan");
            currentSubType = "Tembaga";
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(4);
            Wire.endTransmission();
            myNex.writeNum("setor_kaca.b3.val", 0);
            changePage(11);
        }
        break;
      case 11:
        // Set nilai awal jumlah item
        if (changedPage) {
          countItem = 0;
          // update read weight sensor
          updateStorageValues();
          previousWeightA = storagePlastic;
          previousWeightB = storageMetal;
          previousWeightC = storagePaper;
          previousWeightD = storageGlass;
        }

        // Deteksi item dan akumulasi jumlah item
        if (detectItems(TRIG_PIN_A, ECHO_PIN_A)){
          countItem ++;
          Serial.print("Ada item, jumlah item masuk :  " + countItem);
          // Foto ESP CAM
        }

        // Selesai setor
        if (myNex.readNumber("konfirm_setor.b4.val") == 1){
          updateStorageValues();
          
          if(currentType == "plastik"){
            processStorageA(currentSubType);
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(5);
            Wire.endTransmission();
          }
          else if(currentType == "kertas"){
            processStorageB(currentSubType);
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(6);
            Wire.endTransmission();
          }
          else if(currentType == "logam"){
            processStorageC(currentSubType);
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(7);
            Wire.endTransmission();
          }
          else if(currentType == "kaca"){
            processStorageD(currentSubType);
            Wire.beginTransmission(I2C_ADDRESS);
            Wire.write(8);
            Wire.endTransmission();
          }
          changePage(12);
          delay(250);
          myNex.writeNum("konfirm_setor.b4.val", 0);
        }
    }
    currentPage = newPage;
  }

  // cek koneksi
  //Wire.beginTransmission(I2C_ADDRESS);
  //Wire.write(1);
  //Wire.endTransmission();  
  //delay(1000);
}

bool cekBeratWajar(float beratJenis, float rerataBeratJenis){
  float selisih = rerataBeratJenis - beratJenis;
  float toleransi = 0.1 * rerataBeratJenis;
  previousWeightA = currentWeightA;
  if(selisih > toleransi || selisih < -toleransi)
    return false;
  else
    return true;
}

// Functions to handle waste type selection
void processStorageA(const String &subtype) {
  delay(500);
  // update read weight sensor
  currentWeightA = storagePlastic;
  newWeightA = currentWeightA - previousWeightA;
  processResultPlastic(newWeightA, subtype);
}

void processStorageB(const String &subtype) {
  delay(500);
  // update read weight sensor
  currentWeightB = storagePaper;
  newWeightB = currentWeightB - previousWeightB;
  processResultPaper(newWeightB, subtype);
}

void processStorageC(const String &subtype) {
  delay(500);
  // update read weight sensor
  currentWeightC = storageMetal;
  newWeightC = currentWeightC - previousWeightC;
  processResultMetal(newWeightC, subtype);
}

void processStorageD(const String &subtype) {
  delay(500);
  // update read weight sensor
  currentWeightD = storageGlass;
  newWeightD = currentWeightD - previousWeightD;
  processResultGlass(newWeightD, subtype);
}

// Result processing functions for each waste type
void processResultPlastic(float newWeight, const String &subtype) {
  float pricePerKg = 0;
  float weightPerItem = 0;

  if (subtype == "PET Botol Bening") pricePerKg = 5500;
  else if (subtype == "PET Botol Warna") pricePerKg = 1500;
  else if (subtype == "Plastic Kemasan") pricePerKg = 400;
  else if (subtype == "Tutup Botol") pricePerKg = 3000;

  float totalPrice = newWeight * pricePerKg;
  
  createTransaction(subtype, newWeight, pricePerKg, totalPrice);
}

void processResultPaper(float newWeight, const String &subtype) {
  float pricePerKg = 0;
  float weightPerItem = 0;

  if (subtype == "Arsip (hvs, buku)") pricePerKg = 1800;
  else if (subtype == "Tetra Pack") pricePerKg = 300; 
  else if (subtype == "Kardus") pricePerKg = 1600;
  else if (subtype == "Majalah/koran") pricePerKg = 500;
  
  float totalPrice = newWeight * pricePerKg;

  createTransaction(subtype, newWeight, pricePerKg, totalPrice);
}

void processResultMetal(float newWeight, const String &subtype) {
  float pricePerKg = 0;
  float weightPerItem = 0;

  if (subtype == "Seng (kaleng)") pricePerKg = 2000;
  else if (subtype == "Besi (paku, dll)") pricePerKg = 3500;
  else if (subtype == "Aluminium") pricePerKg = 10000;
  else if (subtype == "Tembaga (kawat)") pricePerKg = 40000;

  float totalPrice = newWeight * pricePerKg;

  createTransaction(subtype, newWeight, pricePerKg, totalPrice);
}

void processResultGlass(float newWeight, const String &subtype) {
  float pricePerKg = 0;

  if (subtype == "Beling (pecahan)") pricePerKg = 200;
  else if (subtype == "Botol Kecap") pricePerKg = 300;
  else if (subtype == "Botol Utuh") pricePerKg = 400;
  else if (subtype == "Botol Hijau") pricePerKg = 500;

  float totalPrice = newWeight * pricePerKg;

  createTransaction(subtype, newWeight, pricePerKg, totalPrice);
}

// belum diedit
void mutasiRekening() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://banksampah-be.vercel.app/user/mutasi/" + cardID;
    http.begin(url);
    int httpResponseCode = http.GET();
    Serial.print("Status " + String(httpResponseCode) + " ");

    if (httpResponseCode == 200) {
      String payload = http.getString();
      StaticJsonDocument<2048> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {
        
        if (doc.is<JsonArray>()) {
          JsonArray array = doc.as<JsonArray>();
        
          int row = 1;
          String cell = "";
          String tanggal = "";
          
          int size = array.size();
          int start = (size > 6) ? size - 6 : 0;
          for (int i = start; i < size; i++) {
            JsonObject item = array[i];
            cell = "r" + String(row) + "c";
            tanggal = item["tanggal"].as<String>();
            myNex.writeStr(cell + "1.txt", tanggal.substring(0, tanggal.indexOf(',')));
            myNex.writeStr(cell + "2.txt", item["tarik"].as<String>());
            myNex.writeStr(cell + "3.txt", item["setor"].as<String>());
            myNex.writeStr(cell + "4.txt", item["status"].as<String>());
            myNex.writeStr(cell + "5.txt", item["saldo"].as<String>());
            row ++;
          }
        } else { 
          myNex.writeStr("r1c1.txt", "Error: Expected a JSON array.");
        }
      } else {
        Serial.print("JSON Deserialization failed: ");
        Serial.println(error.c_str());
      }      
    }
    http.end();
  } else {
    myNex.writeStr("r1c1", "WiFi Error");
  }
}

boolean createTransaction(String jenis_sampah, float weight, float pricePerKg, float kredit){
  HTTPClient http;

  String serverUrl = "https://banksampah-be.vercel.app/user/setor";

  http.begin(serverUrl); // Initialize HTTP connection
  http.addHeader("Content-Type", "application/json"); // Specify JSON content type

    // Create JSON object
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["rfid_user"] = cardID;
  jsonDoc["jenis_sampah"] = jenis_sampah;
  jsonDoc["berat"] = weight;
  jsonDoc["harga_jenis"] = pricePerKg;
  jsonDoc["kredit"] = kredit; 

    // Serialize JSON to string
  String requestBody;
  serializeJson(jsonDoc, requestBody);

    // Send HTTP POST request
  int httpResponseCode = http.POST(requestBody);

  if (httpResponseCode == 200) { // Check if request was successful
    String payload = http.getString();
    Serial.println("HTTP Response Code: " + String(httpResponseCode));
    Serial.println("Response payload: " + payload);
    
    changePage(12);
    currentSaldo += kredit;
    myNex.writeStr("info_setor.t3", jenis_sampah);
    myNex.writeStr("info_setor.t4", String(weight, 2) + " kg");
    myNex.writeStr("info_setor.t5", "Rp " + String(kredit, 2));

    return true;
  }
  return false;
}

int detectItems(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 20000);
  int distance = duration * 0.034 / 2;  // Distance in cm

  return (distance < 10) ? 1 : 0;
}

int updateStorageValues(){
  Wire.requestFrom(SLAVE_ADDRESS, 8);
  
  int loadCellData[4] = {0, 0, 0, 0};

  int i = 0;
  while (Wire.available() && i < 4) {
    // Baca high byte dan low byte dari setiap wadah
    uint8_t highByte = Wire.read();
    uint8_t lowByte = Wire.read();
    // Gabungkan menjadi int16 dan simpan di array
    loadCellData[i] = (highByte << 8) | lowByte;
    i++;
  }
  storagePlastic = loadCellData[0];
  storageMetal = loadCellData[1];
  storagePaper = loadCellData[2];
  storageGlass = loadCellData[3];
}
