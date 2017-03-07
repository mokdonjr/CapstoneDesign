/*
 Name:		SmartCigarCase170223_2.ino
 Created:	2/23/2017 4:31:58 PM
 Author:	�����

 ��� ������ digitalWriteȸ�� �߰� �ʿ�
 ���ȭ. ���̺귯��ȭ �ʿ�

*/

#include <DS1302.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

/* Number of Notifiers = Number of Cigarrtes + 1 */
#define MAX_CIGARRETE 7 // ��� ����
#define MAX_NOTIFIER (MAX_CIGARRETE + 3) // ������ ���� (notifiers 8��, counter 1��, aker 1��)
/* Digital Input Pin Number for Notifiers ( 2 pin ~ 9 pin ) */
#define PIN_REMAIN_0 2
#define PIN_REMAIN_1 3
#define PIN_REMAIN_2 4
#define PIN_REMAIN_3 5
#define PIN_REMAIN_4 6
#define PIN_REMAIN_5 7
#define PIN_REMAIN_6 8
#define PIN_REMAIN_7 9
/* ��踦 ������ ��ư. ī���� */
#define PIN_COUNTER 10
/* ��踦 ä�ﶧ �����ϴ� ��ư. ���� */
#define PIN_TAKER 11
/* Digital Output (HIGH, 5V) Pin Number for pusher */
#define PIN_PUSHER 12
/* RTC��� - �Ƴ��α� ���� ������ ������ �̿� ���� */
#define PIN_DS1302_RST A0
#define PIN_DS1302_DAT A1
#define PIN_DS1302_CLK A2
/* SoftwareSerial Pin Number for Bluetooth */
#define PIN_BLUETOOTH_RX A3
#define PIN_BLUETOOTH_TX A4

/* 5V ��� ������ PIN ���� */
int pusher;

/* 5V ����������� ��� PIN ���� */
typedef struct {
  int pinNum;
  int isTouched = HIGH;
  int state;
  int lastState = LOW;
  long debounceDelay = 50;
  long lastDebounceTime = 0; // Pusher�� �����ֱ� ��Ҵ� �ð�
  int reading = 0;
} Notifier;

/* Fields */
Notifier notifier[MAX_NOTIFIER];

/* ī���� */
Notifier counter;

/* ��� ä��� ��ư */
Notifier taker;

/* Fields - RealTimeClock module */
DS1302 rtc( PIN_DS1302_RST, PIN_DS1302_DAT, PIN_DS1302_CLK );
Time timeSet;

/* Fields - Bluetooth module */
SoftwareSerial bluetooth(PIN_BLUETOOTH_TX, PIN_BLUETOOTH_RX);
char toSend;

/* Fields - EEPROM recording */
int addr;
int tmp = 0;
int count = 0;
unsigned long start; // ���۽ð�

/* �� notifier�� �����Ͻ������� �ٸ��� */
//typedef void(*notifyFuncPtr)();


void setup(){
  
  /* 1. Pin��ȣ �ʱ�ȭ */
  initPinNum();
  
  /* 2. PinMode ���� */
  initPinMode();

  /* 3. ������� ��� ���� */
  bluetooth.begin(9600);
  
  /* 4. EEPROM �ּ� �������� */
  addr = EEPROM.read(0);
  start = millis();

  /* 5. RTC ��⿡ ����ð� ���� <- ���Ŀ� �ȵ���̵�κ��� ���� ����̽� ������� ����� �����ϵ���
   * DS1302 RTC, int DOW, int HOUR, int MIN, int SEC, int DATE, int MONTH, int YEAR */
  setCurrentTime(rtc, THURSDAY, 11, 56, 00, 23, 2, 2017);

  timeSet = rtc.getTime();
}

void loop(){
  
  notifyCurrentCigarrete();
  
}

/* Methods called by loop() */
void bluetoothEEPROM() { // ������ - from �ȵ���̵� to �Ƶ��̳� // ??
  
  if(bluetooth.available()){
    char toSend = (char)bluetooth.read();
    Serial.print(toSend);
    // EEPROM �� ����� ����� �ҷ���
    tmp = EEPROM.read(0);
    Serial.print(tmp);
    bluetooth.print("\n");    
    for(int n = 1; n <= tmp*7 ; n++){
      bluetooth.print(String(EEPROM.read(n)));   
      bluetooth.print("\t");   
      if(n%7 == 0 && n>0){
          bluetooth.print("\n"); 
        }
    }
    // EEPROM Ŭ���� ( �ʱ�ȭ )
    for(int n = 0; n < tmp*7 ; n++){
      EEPROM.write(n,0);
    }
    // EEPROM �ּ� �ʱ�ȭ
    addr=0;
  }
}

/* ��ٿ�� */
void notifyCurrentCigarrete(){
  
  int which_pin = 0;
  
  for(int i=0; i<MAX_NOTIFIER; i++){
    /* 1. notifier[i]�� ���¸� ���� */
    notifier[i].reading = digitalRead(notifier[i].pinNum);

    /* 2. Pusher�� notifier[i]�� ��� state�� ��ȭ�� ������ �� lastDebounceTime ���� 
     * ��, �´��� ���� ��� lastDebounceTime�� �ֽ�ȭ ���� ���� */
    if(notifier[i].reading != notifier[i].lastState) {
      notifier[i].lastDebounceTime = millis();
    }

    /* 3. Pusher�� notifier[i]�� ���� �ð��� debounceDelay���� ���
     * ��, �ٷ� �´�� debounceDelay �ð��� ������ ������ �� ���ǹ��� ��ġ�� ���� */
    if( (millis() - notifier[i].lastDebounceTime) > notifier[i].debounceDelay ) {

      /* 4. Pusher�� notifier[i]�� ��� ��ȭ�� ���� ��
       * ��, �´��� �� �Ǵ� ������ �� ��ȭ�� ������ �� ���ǹ��� ��ġ�� ���� */
      if( notifier[i].reading != notifier[i].state ) {
        notifier[i].state = notifier[i].reading;

        /* 5. Pusher�� notifier[i]�� ��Ƽ� �߻��� �� �̶��
         * ��, Pusher�� notifier[i]�� �������� �߻��� ���� �� ���ǹ��� ��ġ�� ���� */
        if( notifier[i].state == HIGH ) {
          which_pin = notifier[i].pinNum;
          notifier[i].isTouched = !notifier[i].isTouched;
          
          /* �� ���� �ڵ带 �ۼ�!! */
          Serial.println(which_pin); // --> ������� ������ �����
          int current_cigarrete = getCurrentCigarrete(which_pin); // --> ��������� ������ ������
          /* 6. �̺�Ʈ�� �߻��� ���� �ð��� ���������� ��������� ���
           * SoftwareSerial BLUETOOTH, DS1302 RTC, char DELIMITER */
          printCurrentCigarreteToBluetooth(bluetooth, rtc, '-', current_cigarrete);
          /* 7. EEPROM �� �α� ��� */
          recordEEPROM(count, timeSet);

        }
      }
    }
    /* 6. Pusher�� notifier[i]�� ��Ƽ� �߻��� state��ȭ�� �ѹ� �̷���� ��, lastState�� �ٲ��ش�
     * Pusher�� notifier[i]�� ��� debounceDelay���� �ð��� ª�ٸ� �� ���ǹ� ���� ���� �Ʒ� ��� ���� */
    notifier[i].lastState = notifier[i].reading;
  }

}



void setCurrentTime(DS1302 RTC, int DOW, int HOUR, int MIN, int SEC, int DATE, int MONTH, int YEAR) {
  
  RTC.halt(false); // ���� ���� ����
  RTC.writeProtect(false); // �ð� ������ �����ϵ��� ����
  
  RTC.setDOW(DOW); // SUNDAY �� ����
  RTC.setTime(HOUR, MIN, SEC); // �ð��� 12:00:00�� ���� (24�ð� ����)
  RTC.setDate(DATE, MONTH, YEAR); // 2015�� 8�� 16�Ϸ� ����
}

/* EEPROM �� �α� ��� */
void recordEEPROM(int COUNT, Time TT) {
  addr++;
    EEPROM.write(0,addr);
    Serial.println("addr=" + addr);

    for(int index = (addr-1)*7+1 ; index <= addr*7 ; index++) {
      if(index%7 == 1){
        EEPROM.write(index, COUNT);
      }else if(index%7 == 2){
        EEPROM.write(index, TT.year-2000);
      }else if(index%7 == 3){
        EEPROM.write(index, TT.mon);
      }else if(index%7 == 4){
        EEPROM.write(index, TT.date);
      }else if(index%7 == 5){
        EEPROM.write(index, TT.hour);
      }else if(index%7 == 6){
        EEPROM.write(index, TT.min);
      }else if(index%7 == 0 && index != 0){
        EEPROM.write(index, TT.sec);
      }
    }
}

/* notifier �ɰ� ��� ������ ����
 * notifier �ɿ� ���� ��� ������ ���� */
int getCurrentCigarrete(int pin) {
  switch(pin) {
    case PIN_REMAIN_0: return 0; // 2����(PIN_REMAIN_0)�� ��� ���� ��� ���� 0��
    case PIN_REMAIN_1: return 1;
    case PIN_REMAIN_2: return 2;
    case PIN_REMAIN_3: return 3;
    case PIN_REMAIN_4: return 4;
    case PIN_REMAIN_5: return 5;
    case PIN_REMAIN_6: return 6;
    case PIN_REMAIN_7: return 7;
	default: return -1; // pinNum�� 0 ~ 7 ���� ���� �ƴ�
  }
}

/* ��谳���� �ø������Ϳ� ����� ���ڿ��� ����
 * ��谳���� ���ڿ��� ��ȯ�Ͽ� ��� */
void printCurrentCigarreteToSerial(int num) {
  switch(num) {
    case 0: Serial.println("EMPTY 0"); break; // ��谳���� 0���϶�
    case 1: Serial.println("REMAIN 1"); break;
    case 2: Serial.println("REMAIN 2"); break;
    case 3: Serial.println("REMAIN 3"); break;
    case 4: Serial.println("REMAIN 4"); break;
    case 5: Serial.println("REMAIN 5"); break;
    case 6: Serial.println("REMAIN 6"); break;
    case 7: Serial.println("FULL 7"); break; // ��谡 7�� ���� ����
    default: Serial.println("Cannot estimate number of current cigarretes");// 5V���� ���� ���� �ҷ����� ���� ���� ���� �Ұ���
  }
}

void printCurrentCigarreteToBluetooth(SoftwareSerial BLUETOOTH, DS1302 RTC, char DELIMITER, int REMAIN) {
  // ���� ���
  BLUETOOTH.print(RTC.getDOWStr(FORMAT_SHORT));
  BLUETOOTH.print(DELIMITER);
  // ��¥ ���
  BLUETOOTH.print(RTC.getDateStr(FORMAT_LONG,FORMAT_MIDDLEENDIAN,'/'));
  BLUETOOTH.print(DELIMITER);
  // �ð� ���
  BLUETOOTH.print(RTC.getTimeStr());
  BLUETOOTH.print(DELIMITER);
  // ���� ��� ����
  BLUETOOTH.print(REMAIN);
  BLUETOOTH.println();
}

/* Methods called by setup() */
/* initPinNum ������ �ɳѹ��� �����ϴ� �Լ� */
void initPinNum() {
  // 0�� ��
  
  // 1�� ��

  notifier[0].pinNum = PIN_REMAIN_0; // 2�� ��
  
  notifier[1].pinNum = PIN_REMAIN_1; // 3�� ��
  
  notifier[2].pinNum = PIN_REMAIN_2; // 4�� ��

  notifier[3].pinNum = PIN_REMAIN_3; // 5�� ��

  notifier[4].pinNum = PIN_REMAIN_4; // 6�� ��

  notifier[5].pinNum = PIN_REMAIN_5; // 7�� ��

  notifier[6].pinNum = PIN_REMAIN_6; // 8�� ��

  notifier[7].pinNum = PIN_REMAIN_7; // 9�� ��

  counter.pinNum = PIN_COUNTER; // 10�� ��

  // ������ pusher = PIN_PUSHER; // 11�� ��

  // ������� RX // 12�� ��

  // ������� TX // 13�� ��

  // RTC RST // A0�� ��

  // RTC DAT // A1�� ��

  // RTC CLK // A2�� ��
}

/* initPinMode �ɳѹ��� ���ε� ������ �ɸ�� ���� */
void initPinMode() {
  // 0�� ��

  // 1�� ��

  pinMode(notifier[0].pinNum, INPUT_PULLUP); // 2�� ��

  pinMode(notifier[1].pinNum, INPUT_PULLUP); // 3�� ��

  pinMode(notifier[2].pinNum, INPUT_PULLUP); // 4�� ��

  pinMode(notifier[3].pinNum, INPUT_PULLUP); // 5�� ��

  pinMode(notifier[4].pinNum, INPUT_PULLUP); // 6�� ��

  pinMode(notifier[5].pinNum, INPUT_PULLUP); // 7�� ��

  pinMode(notifier[6].pinNum, INPUT_PULLUP); // 8�� ��

  pinMode(notifier[7].pinNum, INPUT_PULLUP); // 9�� ��

  pinMode(counter.pinNum, INPUT_PULLUP); // 10�� ��

  pinMode(taker.pinNum, INPUT_PULLUP); // 11�� ��

  pinMode(pusher, OUTPUT); digitalWrite(pusher, HIGH); // 12�� ��

  // 13�� ��

  // RTC RST // A0�� ��

  // RTC DAT // A1�� ��

  // RTC CLK // A2�� ��

  // Bluetooth RX // A3�� ��

  // Bluetooth TX // A4�� ��

  // A5�� ��
}

//void debounceController(Notifier noti) {
//  /* 1. notifier�� ���¸� ���� */
//  noti.reading = digitalRead(noti.pinNum);
//
//  /* 2. 5V�� notifier�� ��� state�� ��ȭ�� ������ �� lastDebounceTime ���� 
//   * ��, �´��� ���� ��� lastDebounceTime�� �ֽ�ȭ ���� ���� */
//  if(noti.reading != noti.lastState) {
//    noti.lastDebounceTime = millis();
//  }
//
//  /* 3. 5V�� notifier�� ���� �ð��� debounceDelay���� ���
//   * ��, �ٷ� �´�� debounceDelay �ð��� ������ ������ �� ���ǹ��� ��ġ�� ���� */
//  if( (millis() - noti.lastDebounceTime) > noti.debounceDelay ) {
//
//    /* 4. 5V�� notifier�� ��� ��ȭ�� ���� ��
//     * ��, �´��� �� �Ǵ� ������ �� ��ȭ�� ������ �� ���ǹ��� ��ġ�� ���� */
//    if( noti.reading != noti.state ) {
//      noti.state = noti.reading;
//
//      /* 5. 5V�� notifier�� ��Ƽ� �߻��� �� �̶��
//       * ��, 5V�� notifier�� �������� �߻��� ���� �� ���ǹ��� ��ġ�� ���� */
//      if( noti.state == HIGH ) {
//        noti.isTouched = !noti.isTouched;
//        
//        /* �� ���� �ڵ带 �ۼ�!! */
//        
//      }
//    }
//  }
//  /* 6. 5V�� notifier�� ��Ƽ� �߻��� state��ȭ�� �ѹ� �̷���� ��, lastState�� �ٲ��ش�
//   * 5V�� notifier�� ��� debounceDelay���� �ð��� ª�ٸ� �� ���ǹ� ���� ���� �Ʒ� ��� ���� */
//  noti.lastState = noti.reading;
//}