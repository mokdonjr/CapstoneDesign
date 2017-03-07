/*
 Name:		SmartCigarCase170223_2.ino
 Created:	2/23/2017 4:31:58 PM
 Author:	백승찬

 담배 충전시 digitalWrite회로 추가 필요
 모듈화. 라이브러리화 필요

*/

#include <DS1302.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

/* Number of Notifiers = Number of Cigarrtes + 1 */
#define MAX_CIGARRETE 7 // 담배 개수
#define MAX_NOTIFIER (MAX_CIGARRETE + 3) // 측정기 개수 (notifiers 8개, counter 1개, aker 1개)
/* Digital Input Pin Number for Notifiers ( 2 pin ~ 9 pin ) */
#define PIN_REMAIN_0 2
#define PIN_REMAIN_1 3
#define PIN_REMAIN_2 4
#define PIN_REMAIN_3 5
#define PIN_REMAIN_4 6
#define PIN_REMAIN_5 7
#define PIN_REMAIN_6 8
#define PIN_REMAIN_7 9
/* 담배를 꺼내는 버튼. 카운터 */
#define PIN_COUNTER 10
/* 담배를 채울때 차단하는 버튼. 퍼즈 */
#define PIN_TAKER 11
/* Digital Output (HIGH, 5V) Pin Number for pusher */
#define PIN_PUSHER 12
/* RTC모듈 - 아날로그 핀을 디지털 핀으로 이용 시작 */
#define PIN_DS1302_RST A0
#define PIN_DS1302_DAT A1
#define PIN_DS1302_CLK A2
/* SoftwareSerial Pin Number for Bluetooth */
#define PIN_BLUETOOTH_RX A3
#define PIN_BLUETOOTH_TX A4

/* 5V 담배 측정막 PIN 정의 */
int pusher;

/* 5V 담배측정막이 닿는 PIN 정의 */
typedef struct {
  int pinNum;
  int isTouched = HIGH;
  int state;
  int lastState = LOW;
  long debounceDelay = 50;
  long lastDebounceTime = 0; // Pusher와 가장최근 닿았던 시각
  int reading = 0;
} Notifier;

/* Fields */
Notifier notifier[MAX_NOTIFIER];

/* 카운터 */
Notifier counter;

/* 담배 채우기 버튼 */
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
unsigned long start; // 시작시간

/* 각 notifier는 비지니스로직이 다르다 */
//typedef void(*notifyFuncPtr)();


void setup(){
  
  /* 1. Pin번호 초기화 */
  initPinNum();
  
  /* 2. PinMode 설정 */
  initPinMode();

  /* 3. 블루투스 통신 설정 */
  bluetooth.begin(9600);
  
  /* 4. EEPROM 주소 가져오기 */
  addr = EEPROM.read(0);
  start = millis();

  /* 5. RTC 모듈에 현재시간 설정 <- 추후에 안드로이드로부터 최초 디바이스 블루투스 연결시 설정하도록
   * DS1302 RTC, int DOW, int HOUR, int MIN, int SEC, int DATE, int MONTH, int YEAR */
  setCurrentTime(rtc, THURSDAY, 11, 56, 00, 23, 2, 2017);

  timeSet = rtc.getTime();
}

void loop(){
  
  notifyCurrentCigarrete();
  
}

/* Methods called by loop() */
void bluetoothEEPROM() { // 재혁이 - from 안드로이드 to 아두이노 // ??
  
  if(bluetooth.available()){
    char toSend = (char)bluetooth.read();
    Serial.print(toSend);
    // EEPROM 에 저장된 기록을 불러옴
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
    // EEPROM 클리어 ( 초기화 )
    for(int n = 0; n < tmp*7 ; n++){
      EEPROM.write(n,0);
    }
    // EEPROM 주소 초기화
    addr=0;
  }
}

/* 디바운싱 */
void notifyCurrentCigarrete(){
  
  int which_pin = 0;
  
  for(int i=0; i<MAX_NOTIFIER; i++){
    /* 1. notifier[i]의 상태를 읽음 */
    notifier[i].reading = digitalRead(notifier[i].pinNum);

    /* 2. Pusher와 notifier[i]가 닿아 state에 변화가 생겼을 때 lastDebounceTime 설정 
     * 즉, 맞닿지 않을 경우 lastDebounceTime은 최신화 되지 않음 */
    if(notifier[i].reading != notifier[i].lastState) {
      notifier[i].lastDebounceTime = millis();
    }

    /* 3. Pusher와 notifier[i]가 닿은 시간이 debounceDelay보다 길며
     * 즉, 바로 맞닿고 debounceDelay 시간이 지나기 전에는 이 조건문을 거치지 않음 */
    if( (millis() - notifier[i].lastDebounceTime) > notifier[i].debounceDelay ) {

      /* 4. Pusher와 notifier[i]가 닿아 변화가 있을 때
       * 즉, 맞닿은 후 또는 떨어진 후 변화가 없으면 이 조건문을 거치지 않음 */
      if( notifier[i].reading != notifier[i].state ) {
        notifier[i].state = notifier[i].reading;

        /* 5. Pusher와 notifier[i]가 닿아서 발생한 것 이라면
         * 즉, Pusher와 notifier[i]가 떨어질때 발생한 것은 이 조건문을 거치지 않음 */
        if( notifier[i].state == HIGH ) {
          which_pin = notifier[i].pinNum;
          notifier[i].isTouched = !notifier[i].isTouched;
          
          /* 이 곳에 코드를 작성!! */
          Serial.println(which_pin); // --> 블루투스 연결후 지우기
          int current_cigarrete = getCurrentCigarrete(which_pin); // --> 블루투스로 전송할 데이터
          /* 6. 이벤트가 발생한 현재 시간을 구분자포함 블루투스로 출력
           * SoftwareSerial BLUETOOTH, DS1302 RTC, char DELIMITER */
          printCurrentCigarreteToBluetooth(bluetooth, rtc, '-', current_cigarrete);
          /* 7. EEPROM 에 로그 기록 */
          recordEEPROM(count, timeSet);

        }
      }
    }
    /* 6. Pusher와 notifier[i]가 닿아서 발생한 state변화가 한번 이루어진 후, lastState를 바꿔준다
     * Pusher와 notifier[i]가 닿고 debounceDelay보다 시간이 짧다면 위 조건문 진행 없이 아래 명령 수행 */
    notifier[i].lastState = notifier[i].reading;
  }

}



void setCurrentTime(DS1302 RTC, int DOW, int HOUR, int MIN, int SEC, int DATE, int MONTH, int YEAR) {
  
  RTC.halt(false); // 동작 모드로 설정
  RTC.writeProtect(false); // 시간 변경이 가능하도록 설정
  
  RTC.setDOW(DOW); // SUNDAY 로 설정
  RTC.setTime(HOUR, MIN, SEC); // 시간을 12:00:00로 설정 (24시간 형식)
  RTC.setDate(DATE, MONTH, YEAR); // 2015년 8월 16일로 설정
}

/* EEPROM 에 로그 기록 */
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

/* notifier 핀과 담배 개수를 매핑
 * notifier 핀에 대한 담배 개수를 리턴 */
int getCurrentCigarrete(int pin) {
  switch(pin) {
    case PIN_REMAIN_0: return 0; // 2번핀(PIN_REMAIN_0)일 경우 남은 담배 개수 0개
    case PIN_REMAIN_1: return 1;
    case PIN_REMAIN_2: return 2;
    case PIN_REMAIN_3: return 3;
    case PIN_REMAIN_4: return 4;
    case PIN_REMAIN_5: return 5;
    case PIN_REMAIN_6: return 6;
    case PIN_REMAIN_7: return 7;
	default: return -1; // pinNum이 0 ~ 7 사이 값이 아님
  }
}

/* 담배개수를 시리얼모니터에 출력할 문자열로 매핑
 * 담배개수를 문자열로 변환하여 출력 */
void printCurrentCigarreteToSerial(int num) {
  switch(num) {
    case 0: Serial.println("EMPTY 0"); break; // 담배개수가 0개일때
    case 1: Serial.println("REMAIN 1"); break;
    case 2: Serial.println("REMAIN 2"); break;
    case 3: Serial.println("REMAIN 3"); break;
    case 4: Serial.println("REMAIN 4"); break;
    case 5: Serial.println("REMAIN 5"); break;
    case 6: Serial.println("REMAIN 6"); break;
    case 7: Serial.println("FULL 7"); break; // 담배가 7개 꽉찬 상태
    default: Serial.println("Cannot estimate number of current cigarretes");// 5V막과 핀의 접합 불량으로 남은 개수 측정 불가능
  }
}

void printCurrentCigarreteToBluetooth(SoftwareSerial BLUETOOTH, DS1302 RTC, char DELIMITER, int REMAIN) {
  // 요일 출력
  BLUETOOTH.print(RTC.getDOWStr(FORMAT_SHORT));
  BLUETOOTH.print(DELIMITER);
  // 날짜 출력
  BLUETOOTH.print(RTC.getDateStr(FORMAT_LONG,FORMAT_MIDDLEENDIAN,'/'));
  BLUETOOTH.print(DELIMITER);
  // 시간 출력
  BLUETOOTH.print(RTC.getTimeStr());
  BLUETOOTH.print(DELIMITER);
  // 남은 담배 개수
  BLUETOOTH.print(REMAIN);
  BLUETOOTH.println();
}

/* Methods called by setup() */
/* initPinNum 변수와 핀넘버를 매핑하는 함수 */
void initPinNum() {
  // 0번 핀
  
  // 1번 핀

  notifier[0].pinNum = PIN_REMAIN_0; // 2번 핀
  
  notifier[1].pinNum = PIN_REMAIN_1; // 3번 핀
  
  notifier[2].pinNum = PIN_REMAIN_2; // 4번 핀

  notifier[3].pinNum = PIN_REMAIN_3; // 5번 핀

  notifier[4].pinNum = PIN_REMAIN_4; // 6번 핀

  notifier[5].pinNum = PIN_REMAIN_5; // 7번 핀

  notifier[6].pinNum = PIN_REMAIN_6; // 8번 핀

  notifier[7].pinNum = PIN_REMAIN_7; // 9번 핀

  counter.pinNum = PIN_COUNTER; // 10번 핀

  // 사용안함 pusher = PIN_PUSHER; // 11번 핀

  // 블루투스 RX // 12번 핀

  // 블루투스 TX // 13번 핀

  // RTC RST // A0번 핀

  // RTC DAT // A1번 핀

  // RTC CLK // A2번 핀
}

/* initPinMode 핀넘버가 매핑된 변수로 핀모드 설정 */
void initPinMode() {
  // 0번 핀

  // 1번 핀

  pinMode(notifier[0].pinNum, INPUT_PULLUP); // 2번 핀

  pinMode(notifier[1].pinNum, INPUT_PULLUP); // 3번 핀

  pinMode(notifier[2].pinNum, INPUT_PULLUP); // 4번 핀

  pinMode(notifier[3].pinNum, INPUT_PULLUP); // 5번 핀

  pinMode(notifier[4].pinNum, INPUT_PULLUP); // 6번 핀

  pinMode(notifier[5].pinNum, INPUT_PULLUP); // 7번 핀

  pinMode(notifier[6].pinNum, INPUT_PULLUP); // 8번 핀

  pinMode(notifier[7].pinNum, INPUT_PULLUP); // 9번 핀

  pinMode(counter.pinNum, INPUT_PULLUP); // 10번 핀

  pinMode(taker.pinNum, INPUT_PULLUP); // 11번 핀

  pinMode(pusher, OUTPUT); digitalWrite(pusher, HIGH); // 12번 핀

  // 13번 핀

  // RTC RST // A0번 핀

  // RTC DAT // A1번 핀

  // RTC CLK // A2번 핀

  // Bluetooth RX // A3번 핀

  // Bluetooth TX // A4번 핀

  // A5번 핀
}

//void debounceController(Notifier noti) {
//  /* 1. notifier의 상태를 읽음 */
//  noti.reading = digitalRead(noti.pinNum);
//
//  /* 2. 5V와 notifier가 닿아 state에 변화가 생겼을 때 lastDebounceTime 설정 
//   * 즉, 맞닿지 않을 경우 lastDebounceTime은 최신화 되지 않음 */
//  if(noti.reading != noti.lastState) {
//    noti.lastDebounceTime = millis();
//  }
//
//  /* 3. 5V와 notifier가 닿은 시간이 debounceDelay보다 길며
//   * 즉, 바로 맞닿고 debounceDelay 시간이 지나기 전에는 이 조건문을 거치지 않음 */
//  if( (millis() - noti.lastDebounceTime) > noti.debounceDelay ) {
//
//    /* 4. 5V와 notifier가 닿아 변화가 있을 때
//     * 즉, 맞닿은 후 또는 떨어진 후 변화가 없으면 이 조건문을 거치지 않음 */
//    if( noti.reading != noti.state ) {
//      noti.state = noti.reading;
//
//      /* 5. 5V와 notifier가 닿아서 발생한 것 이라면
//       * 즉, 5V와 notifier가 떨어질때 발생한 것은 이 조건문을 거치지 않음 */
//      if( noti.state == HIGH ) {
//        noti.isTouched = !noti.isTouched;
//        
//        /* 이 곳에 코드를 작성!! */
//        
//      }
//    }
//  }
//  /* 6. 5V와 notifier가 닿아서 발생한 state변화가 한번 이루어진 후, lastState를 바꿔준다
//   * 5V와 notifier가 닿고 debounceDelay보다 시간이 짧다면 위 조건문 진행 없이 아래 명령 수행 */
//  noti.lastState = noti.reading;
//}