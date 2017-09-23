#include <MCP3008.h>
#include <ESP8266WiFi.h>
#include <MyCommonFun.h>
#include <PubSubClient.h>
//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>

#define CS_PIN    D8
#define CLOCK_PIN D5
#define MOSI_PIN  D7
#define MISO_PIN  D6

#define THREEPHASE

#define IND 5
#define IND_SEND 4


MCP3008 adc(CLOCK_PIN, MOSI_PIN, MISO_PIN, CS_PIN);
WiFiClient espClient;

const char* ssidRpi = "RaspberryPiEnergyLogger";
const char* passRpi = "raspberryraspberry";
const byte serverRpi[] = {172, 24, 1, 1};

const char* subTopic = "SET";

const float ab_correction = 0.840645846575558;
const float bc_correction = 0.87330169361856;
const float ca_correction = 0.840645846575558;

//const float CT_RES = 20.6;
const float CT_RES = 21.6;

float a_current = 0;
float ab_voltage = 0;
float ab_active_power = 0;
float ab_energy = 0;
float a_current_offset = 3.00870365303179;
//float a_current_offset = 3.01240202322535;
float ab_voltage_offset = 2.8129809081812;
float ab_net_energy = 0;
float ab_power = 0;
char ab_powerStr[15];
char ab_energyStr[15];

#ifdef THREEPHASE
  float b_current = 0;
  float bc_voltage = 0;
  float bc_active_power = 0;
  float bc_energy = 0;
  float b_current_offset = 3.01755482585503;
  float bc_voltage_offset = 2.81814090028871;

  //float b_current_offset = 3.01580370633;
  //float bc_voltage_offset = 2.81770086003634;
  
  float bc_net_energy = 0;
  float bc_power = 0;
  char bc_powerStr[15];
  char bc_energyStr[15];
  
  float c_current = 0;
  float ca_voltage = 0;
  float ca_active_power = 0;
  float ca_energy = 0;
  //float c_current_offset = 3.01240202322535;
  //float ca_voltage_offset = 2.81383347994153;
  
  float c_current_offset = 3.00899875616206;
  float ca_voltage_offset = 2.81398369179471;
  float ca_net_energy = 0;
  float ca_power = 0;
  char ca_powerStr[15];
  char ca_energyStr[15];
  
#endif

float TOTAL_POWER = 0.0;
float TOTAL_ENERGY = 0.0;
char TOTAL_powerStr[15];
char TOTAL_energyStr[15];

char accu_DATA[300];
char messageSet[15];

int n = 0;
int send_data = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  int k = 0; int c = 0;
  for (k = 0; k < length; k++){
    messageSet[k] = payload[k];
  }
  for (k = 0; k < length; k++) {
    if (strcmp(messageSet, "RESET") == 0) {
      c++;
    }
  }
  Serial.println(c);
  if (c == length) {
    ab_net_energy = 0;
    bc_net_energy = 0;
    ca_net_energy = 0;
  }
  else {
    Serial.println("Wrong setting indicated");
  }
  Serial.println(messageSet);
  messageReset(messageSet, sizeof(messageSet));
}

PubSubClient clientPi(serverRpi, PORTPI, callback, espClient);

void setup() {
  Serial.begin(115200);
  pinMode(IND, OUTPUT);
  pinMode(IND_SEND, OUTPUT);
  
  digitalWrite(IND_SEND, LOW);
  digitalWrite(IND, LOW);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidRpi, passRpi);
  conBuffer(ssidRpi,passRpi, IND);
  clientPi.connect("ESP8266Client");
  clientPi.subscribe(subTopic);
}

void loop() {

  if (WiFi.status() != WL_CONNECTED) {
    conBuffer(ssidRpi, passRpi, IND);  //Add buffer if connection is lost
    clientPi.connect("ESP8266Client");
    clientPi.subscribe(subTopic);
  }
  
  double time_micro = micros();
  double time_perSample = 0;
  while ((micros() - time_micro) < 1000000) {
    n += 1;
    time_perSample = micros();
     a_current = (adc.readADC(0))*(4.04/1023.0);
    ab_voltage = (adc.readADC(1))*(4.04/1023.0);
     a_current = 1470.0*(a_current-a_current_offset)/(CT_RES);
    ab_voltage = (ab_voltage-ab_voltage_offset)*(110000.0/10000.0)*18.333333333;
    ab_active_power = ab_voltage*a_current;
    
    if (ab_active_power < 0.0) {
      ab_active_power = 0.0;
    }
    ab_energy = ab_energy + (ab_active_power);
    #ifdef THREEPHASE
      b_current = (adc.readADC(2))*(4.04/1023.0);
      bc_voltage = (adc.readADC(3))*(4.04/1023.0);
       b_current = 1470.0*(b_current-b_current_offset)/(CT_RES);
      bc_voltage = (bc_voltage-bc_voltage_offset)*(110000.0/10000.0)*18.333333333;
      bc_active_power = bc_voltage*b_current;
  
      if (bc_active_power < 0.0) {
        bc_active_power = 0.0;
      }
      bc_energy = bc_energy + (bc_active_power);
      
       c_current = (adc.readADC(4))*(4.04/1023.0);
      ca_voltage = (adc.readADC(5))*(4.04/1023.0);
       c_current = 1470.0*(c_current-c_current_offset)/(CT_RES);
      ca_voltage = (ca_voltage-ca_voltage_offset)*(110000.0/10000.0)*18.333333333;
      ca_active_power = ca_voltage*c_current;
      
      if (ca_active_power < 0.0) {
        ca_active_power = 0.0;
      }
      ca_energy = ca_energy + (ca_active_power);
     #endif
  }
  ab_net_energy += ab_energy/(1000.0*n*3600.0);
  ab_power = (ab_energy/(1000.0*n))*ab_correction;
  if (ab_power < 0.025) {
      ab_power = 0.0;
    }
  Serial.print(millis()/1000, 10);
  Serial.print('\t');
  Serial.print(ab_net_energy, 10);
  Serial.print("  kwH");
  Serial.print('\t');
  Serial.print(ab_power, 10);
  #ifdef SINGLEPHASE
    Serial.println("  kw");
  #endif

  #ifdef THREEPHASE
    //Serial.print("  kw");
  #endif
  ab_energy = 0.0;

  #ifdef THREEPHASE
    bc_net_energy += bc_energy/(1000.0*n*3600.0);
    bc_power = (bc_energy/(1000.0*n))*bc_correction;
    if (bc_power < 0.025) {
      bc_power = 0.0;
    }
    Serial.print('\t');
    Serial.print(bc_net_energy, 10);
    Serial.print("  kwH");
    Serial.print('\t');
    Serial.print(bc_power, 10);
    //Serial.print("  kw");
    bc_energy = 0.0;
  
    ca_net_energy += ca_energy/(1000.0*n*3600.0);
    ca_power = (ca_energy/(1000.0*n))*ca_correction;
    if (ca_power < 0.025) {
      ca_power = 0.0;
    }
    Serial.print('\t');
    Serial.print(ca_net_energy, 10);
    Serial.print("  kwH");
    Serial.print('\t');
    Serial.print(ca_power, 10);
    //Serial.print("  kw");
    ca_energy = 0.0;
   #endif

  TOTAL_POWER = ab_power + bc_power + ca_power;
  TOTAL_ENERGY = ab_net_energy + bc_net_energy + ca_net_energy;
  Serial.print('\t');
  Serial.print("Total:   ");
  Serial.print(TOTAL_POWER, 10);
  Serial.print("  KW");
  Serial.print('\t');
  Serial.print(TOTAL_ENERGY, 10);
  Serial.println("  KWH");

  if (send_data == 10) {
    digitalWrite(IND, HIGH);
    dtostrf(ab_power, 4, 10, ab_powerStr);
    dtostrf(bc_power, 4, 10, bc_powerStr);
    dtostrf(ca_power, 4, 10, ca_powerStr);
    dtostrf(TOTAL_POWER, 4, 10, TOTAL_powerStr);
    
    dtostrf(ab_net_energy, 4, 10, ab_energyStr);
    dtostrf(bc_net_energy, 4, 10, bc_energyStr);
    dtostrf(ca_net_energy, 4, 10, ca_energyStr);
    dtostrf(TOTAL_ENERGY, 4, 10, TOTAL_energyStr);

    sprintf(accu_DATA, "%s,%s,%s,%s,%s,%s,%s,%s", ab_powerStr, bc_powerStr, ca_powerStr, TOTAL_powerStr, ab_energyStr, bc_energyStr, ca_energyStr, TOTAL_energyStr);

    clientPi.connect("ESP8266Client");
    clientPi.publish("DATA", accu_DATA);

    send_data = 0;
    delay(30);
  }  
  digitalWrite(IND, LOW);
  send_data += 1;
  n = 0;
  clientPi.loop();
  delayMicroseconds(100);
}

void publishToRpi (char* topicToPub, float messageToPub) {
  //clientPi.connect(ESP8266Client);
  //Serial.print("Look here: ");
  //Serial.println(topicToPub);
  //clientPi.publish(topicToPub, messageToPub);
}
