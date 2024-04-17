/***
 This code Created by AGEAB project group
 This code created for car track application and also gives the G data to the AGEAB application


*/

#include <SoftwareSerial.h>
#include "Wire.h"
#include <MPU6050_light.h>

// Initialize software serial - Pin 2 (Arduino RX) is connected to TX on the GSM Module
// Pin 3 (Arduino TX) is connected to RX on the GSM Module
// MPU6050 SDA pin A4 pin (analog) and SCL pin to A5 (analog) pin 
SoftwareSerial gsmSerial(2, 3); // RX, TX
String gpsInfo = "";
MPU6050 mpu(Wire);
void setup() {
  // Begin serial communication with the computer
  Serial.begin(115200);
  // Begin serial communication with the SIM7600E-H module
  gsmSerial.begin(115200);
  // set the module and arduino baud rates 9600 for clear communication
  sendATCommand("AT+IPR=9600",200); // Check communication
  Serial.begin(9600);
   gsmSerial.begin(9600);
  delay(1000);

  Serial.println("Initializing...");
  initializeGyro();
  // Initialize GSM and GPS modules
  initializeGSM();
 
  initializeGPS();

  // Now moving to loop() for continuous operation.
}

void loop() {
  // Fetch GPS data
  String latitude, longitude;
  if (fetchGPSData(latitude, longitude)) {
    // If GPS data fetched successfully, send it to the database
    sendGPSDataToDatabase(latitude, longitude);
  } else {
    Serial.println("Failed to fetch GPS data.");
  }

  // Wait for a specified time span before sending the location again
  delay(500); // 30 seconds delay; adjust as per your requirement
}

/**
*This fuction initilize and calibrate the MPU6050 Gyroscope
*/
void initializeGyro() {
    Wire.begin();

    byte status = mpu.begin();
    Serial.print(F("MPU6050 status: "));
    Serial.println(status);
    while(status!=0){ } // stop everything if could not connect to MPU6050

    Serial.println(F("Calculating offsets, do not move MPU6050"));
    delay(1000);
    mpu.calcOffsets(true,true); // gyro and accelero
    Serial.println("Done!\n");
     delay(5000);
}

/**
* This function initilize the GPRS part of the Sim7600E-H
* So it can connect internet to send a http post request
*
*/
void initializeGSM() {
  
 
  sendATCommand("AT",200); // Check communication
  sendATCommand("AT+CREG=1",200); // Full functionality
  delay(3000); // Wait for network registration
  sendATCommand("AT+CGATT=1",200); // Open GPRS context
   sendATCommand("AT+CGACT=1,1",200); // Query GPRS context
  // Setup bearer profile for internet
  sendATCommand("AT+CGDCONT=1,\"IP\",\"internet\"",2000); // Replace "internet" with your APN
 
 
}

/**
* This function allows open the GPS in the module 
* So we can get the Namea info  from the module
*/
void initializeGPS() {

  sendATCommand("AT+CGPS=1", 2000); // Turn on GPS
}

bool fetchGPSData(String &latitude, String &longitude) {
  String response =  sendATCommand("AT+CGPSINFO", 500); // Request GPS data;
  latitude = response;
  int index = response.indexOf("+CGPSINFO: ");
  if (index != -1) {
     

    if (!latitude.startsWith("0") && !longitude.startsWith("0")) {
      return true; // Assume valid data if lat & long don't start with '0'
    }
  }
  return false;
}
/**
* This function send the fetched GPS data to data base via http request
*/
void sendGPSDataToDatabase(String latitude, String longitude) {

      //Serial.println("gyrooooo: "+ b); // Debugging: print the full URL to check correctness
      // Construct the URL with GPS data

      // fullUrl = url + a; // Assuming 'a' contains query parameters or path extensions
      // char fullUrlChar[fullUrl.length() + 1];

      // Copy the contents of the String object to the char array
      //fullUrl.toCharArray(fullUrlChar, sizeof(fullUrlChar));
      //Serial.println("Full URL: "+  fullUrl); // Debugging: print the full URL to check correctness
      // Initialize HTTP service
  sendATCommand("AT+HTTPINIT",100);
  sendATCommand("AT+HTTPPARA=\"CID\",1",100);
  latitude.replace(" ", "");

  latitude.replace("\n", "");
   String b,a, fullUrl = "";
   a = extractGPSData(latitude); 
   b = getGyro();

  String url = "http://64.227.114.152/test_connection/"+String(a+b);
  delay(500);
  sendATCommand("AT+HTTPPARA=\"URL\",\"" +url+"\"",500);

  // Start GET session
  sendATCommand("AT+HTTPACTION=0",200);

  // Optionally, read the HTTP result and data
  //sendATCommand("AT+HTTPREAD",500);

  // End the HTTP session
  sendATCommand("AT+HTTPTERM",100);

  // Log to Serial Monitor
  Serial.println("GPS data sent to database.");
}

/**
* This function allows us to send a AT command to the module with specified delay time (ms)
*/
String sendATCommand(String command, unsigned long timeout) {
    String response = "";
    gsmSerial.println(command); // Send the AT command

    unsigned long startTime = millis();
    while (millis() - startTime < timeout) {
        if (gsmSerial.available()) {
            char c = gsmSerial.read();
            response += c; // Append each read character into the response string
        }
    }
    Serial.println(response); // Optional: Echo the response to the Serial Monitor
    return response;
}

/*
* This function extract the gps data interms of latitude longtitude and speed (knots)
*
*/
String extractGPSData(String response) {
   String noSpaces,cleanedGPSData ="";
  
    int index_of = response.indexOf(":") +1;
    response = response.substring(index_of);
    
       for (char c : response) {
        if (c != ' ') {
            noSpaces += c;
        }
    }
    response = noSpaces;
    noSpaces="";
    int comma_position = 0;
    int i = 0;
    for (char c : response){
        
        if(comma_position == 0 || comma_position == 2 || comma_position > 6){
             noSpaces += c;
        }
        if(c == ','){comma_position++;}
    }
    response = noSpaces;
    Serial.println("response gps data : " + response);
       // Iterate over the extracted GPS data
    for (int i = 0; i < response.length(); i++) {
      // Check if the current character is not a letter
      if (!isAlpha(response.charAt(i))) {
        // Append non-letter characters to the cleanedGPSData string
        cleanedGPSData += response.charAt(i);
      }
    }
    cleanedGPSData = cleanedGPSData.substring(0,cleanedGPSData.length()-4);
    
     Serial.println("Cleaned gps data : " + cleanedGPSData);

    return cleanedGPSData;
}

/*
* get the x and y axisis g forces
* + X axis left return | -X axis right return
* + Y axis brake | -Y axis is acceleration
*/
String getGyro (){
  
  mpu.update(); // MPU'nun değerlerini güncelle

  // Ham ivme verileri (m/s^2)
  float ax = mpu.getAccX();
  float ay = mpu.getAccY();
  float az = mpu.getAccZ();


  // Z ekseninde genellikle bu tür bir düzeltme yapmaya gerek yoktur, çünkü az zaten yerçekimi ile aynı yönde
  
  // Gerçek g-kuvvetleri
  String modifiedGyro = ","+String(ax) + "," + String(ay);
  
  return modifiedGyro;

   
   /* mpu.update();
    
    Serial.print(F("TEMPERATURE: "));Serial.println(mpu.getTemp());
    Serial.print(F("ACCELERO  X: "));Serial.print(mpu.getAccX());
    Serial.print("\tY: ");Serial.print(mpu.getAccY());
    Serial.print("\tZ: ");Serial.println(mpu.getAccZ());

    modifiedGyro =","+ String( mpu.getAccX() )  + ","+mpu.getAccY() +","+ mpu.getAccZ();

    Serial.print(F("GYRO      X: "));Serial.print(mpu.getGyroX());
    Serial.print("\tY: ");Serial.print(mpu.getGyroY());
    Serial.print("\tZ: ");Serial.println(mpu.getGyroZ());

    Serial.print(F("ACC ANGLE X: "));Serial.print(mpu.getAccAngleX());
    Serial.print("\tY: ");Serial.println(mpu.getAccAngleY());

    Serial.print(F("ANGLE     X: "));Serial.print(mpu.getAngleX());
    Serial.print("\tY: ");Serial.print(mpu.getAngleY());
    Serial.print("\tZ: ");Serial.println(mpu.getAngleZ());


   // modifiedGyro = modifiedGyro  +","+ String( mpu.getAngleX() ) +","+ mpu.getAngleY()  +","+ mpu.getAngleZ();
   
    return String(modifiedGyro)*/


}
