/*
* Snake game based on display driver Lcdxxx()
* functions found from the internet
*/
#include <TimerOne.h>

#define CE    13  //chip enable
#define RST   11  //reset input
#define DC     9  //data/command
#define DIN    7  //serial data input
#define CLK    5  //serial clock input
#define L_BUTTON  2  //button
#define R_BUTTON  3  //button


#define TEST_DISPLAY 0  // 1 for testing, 0 otherwise
#define TEST_PRINT_SNAKE 0 // print snake's body part coordinates to console
#define TEST_GAMESTATUS 1 // print gamestatus

#define LCD_C     LOW01
#define LCD_D     HIGH
#define LCD_X     84
#define LCD_Y     48

#define LEN(x) ((sizeof x)/(sizeof x[0]))
#define FPS       30    // not used
#define GAMESPEED 150*1000 // game cycling rate in microseconds

volatile unsigned long time_last_pressed = 0;
volatile byte ready_to_go                = 0;

//#include "ascii_table.h"
//#include "scr_buffer.h"


void LcdWriteData(byte cmd){
  digitalWrite(DC, HIGH);  //Data
  digitalWrite(CE, LOW);
  shiftOut(DIN, CLK, MSBFIRST, cmd);
  digitalWrite(CE, HIGH);
}

void LcdWriteCmd(byte cmd){
  digitalWrite(DC, LOW);    //Commands
  digitalWrite(CE, LOW);    //Enable control
  shiftOut(DIN, CLK, MSBFIRST, cmd); //Write 'cmd' to DIN
  digitalWrite(CE, HIGH);   //Disable control
}


typedef enum {
  GAME_ON,
  GAME_OVER,
  GAME_PAUSE
} GAMESTATE;

/*HERE ARE IMPLEMENTATIONS FOR SNAKE*/
typedef enum {    //directions for snake
  LEFT,
  RIGHT,
  UP,
  DOWN
} Dir;

typedef struct coordinates{
  byte x;
  byte y;
} CRD;

struct SnakeObject{
  CRD parts[35];
  Dir dir;
  Dir new_dir;
  byte tail;
  byte head;
};

struct SnakeObject worm;

struct one_byte{
  byte b0:1;
  byte b1:1;
  byte b2:1;
  byte b3:1;
  byte b4:1;
  byte b5:1;
  byte b6:1;
  byte b7:1;
};
struct one_byte scr_buffer[LCD_Y/8][LCD_X] = {0};


/* Writes to SCREEN BUFFER */
void turnPixelOn(byte y, byte x){
  byte pixel = y%8;
  if (pixel==0) scr_buffer[y/8][x].b0 = 1;
  if (pixel==1) scr_buffer[y/8][x].b1 = 1;
  if (pixel==2) scr_buffer[y/8][x].b2 = 1;
  if (pixel==3) scr_buffer[y/8][x].b3 = 1;
  if (pixel==4) scr_buffer[y/8][x].b4 = 1;
  if (pixel==5) scr_buffer[y/8][x].b5 = 1;
  if (pixel==6) scr_buffer[y/8][x].b6 = 1;
  if (pixel==7) scr_buffer[y/8][x].b7 = 1;
}


void ClearScreen(void){

  LcdWriteCmd(0x40); //y
  LcdWriteCmd(0x80); //x
  for(int i=0; i<504; i++){
    LcdWriteData(0x00);}

  //LcdWriteCmd(0x0C);  //LCD all segments off
}

/* ONKO TAMA TARPEELLLINEN? */
void Draw(byte x, byte y){  //func to turn single pixel on
  byte ybit = y%8;
  byte row = y/8;
  LcdWriteCmd((0x40|row));  //set y address
  LcdWriteCmd((0x80|x));    //set x address
  LcdWriteData((0x01 << ybit ));  //set bit
}

/* Writes SCREEN BUFFER to output, erases SCREEN BUFFER sametime */
void DrawScreen(void){

  //ClearScreen();
  LcdWriteCmd((0x40));  //set y address
  LcdWriteCmd((0x80));    //set x address

  byte byte_buffer = 0x00;
  for (byte x=0; x<84; x++){
    for (byte current_byte=0; current_byte<6; current_byte++){  //struct
        for (byte pixel=0; pixel<8; pixel++){
          if (pixel == 0)
            byte_buffer |= scr_buffer[current_byte][x].b0;
          if (pixel == 1)
            byte_buffer |= scr_buffer[current_byte][x].b1 <<1;
          if (pixel == 2)
            byte_buffer |= scr_buffer[current_byte][x].b2 <<2;
          if (pixel == 3)
            byte_buffer |= scr_buffer[current_byte][x].b3 <<3;
          if (pixel == 4)
            byte_buffer |= scr_buffer[current_byte][x].b4 <<4;
          if (pixel == 5)
            byte_buffer |= scr_buffer[current_byte][x].b5 <<5;
          if (pixel == 6)
            byte_buffer |= scr_buffer[current_byte][x].b6 <<6;
          if (pixel == 7)
            byte_buffer |= scr_buffer[current_byte][x].b7 <<7;
        }

        LcdWriteData(byte_buffer);
        memset(&scr_buffer[current_byte][x], 0, sizeof(scr_buffer[current_byte][x]));
        byte_buffer = 0x00;
    }
  }
}

void createSnake(void){
    worm.head = 0;
    worm.tail = LEN(worm.parts)-1;
    worm.dir = UP;
  for(byte i=0; i<LEN(worm.parts); i++){
    worm.parts[i].x = 20;
    worm.parts[i].y = 20+i;
  }
}
void drawSnake(void){
  for (byte i=0; i<LEN(worm.parts); i++){
    turnPixelOn(worm.parts[i].y, worm.parts[i].x);
  }
}
void moveSnake(void){
  for (byte i=LEN(worm.parts)-1; i>0; i--){
      worm.parts[i] = worm.parts[i-1];
      //Serial.println(i);
    }

  if (worm.dir ==UP){
    worm.parts[0].y -=1;
  }
  if (worm.dir ==DOWN){
     worm.parts[0].y +=1;
  }
  if (worm.dir ==RIGHT){
    worm.parts[0].x +=1;
  }
  if (worm.dir ==LEFT){
    worm.parts[0].x -=1;
  }
}
bool isSelfCollision(void){
  for (byte i = worm.head+1; i<=worm.tail; i++){
    if (worm.parts[worm.head].x == worm.parts[i].x && worm.parts[worm.head].y == worm.parts[i].y){
      return true;
    }
  }
  return false;
}

void turnToLeft(){
  unsigned long time_interrupt_start = millis();
  if ((time_interrupt_start - time_last_pressed) > 200){
    time_last_pressed = time_interrupt_start;
    if (worm.dir == UP){
       worm.new_dir = LEFT;
    }
    if (worm.dir == LEFT){
       worm.new_dir = DOWN;
    }
    if (worm.dir == DOWN){
       worm.new_dir = RIGHT;
    }
    if (worm.dir == RIGHT){
       worm.new_dir = UP;
    }
  }
}
void turnToRight(){
  unsigned long time_interrupt_start = millis();
  if ((time_interrupt_start - time_last_pressed) > 200){
    time_last_pressed = time_interrupt_start;
    if (worm.dir == UP){
       worm.new_dir = RIGHT;
    }
    if (worm.dir == LEFT){
       worm.new_dir = UP;
    }
    if (worm.dir == DOWN){
       worm.new_dir = LEFT;
    }
    if (worm.dir == RIGHT){
       worm.new_dir = DOWN;
    }
  }
}

void gameCycle(void){
  ready_to_go = 1;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Begin Testing:");
  pinMode(RST, OUTPUT);
  pinMode(CE, OUTPUT);
  pinMode(DC, OUTPUT);
  pinMode(DIN, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(L_BUTTON, INPUT_PULLUP);
  pinMode(R_BUTTON, INPUT_PULLUP);
  //digitalWrite(L_BUTTON, HIGH);
  //digitalWrite(R_BUTTON, HIGH);
  digitalWrite(RST, LOW);
  digitalWrite(RST, HIGH);

  Timer1.initialize(GAMESPEED);
  Timer1.attachInterrupt(gameCycle);
  attachInterrupt(digitalPinToInterrupt(L_BUTTON), turnToLeft, FALLING);
  attachInterrupt(digitalPinToInterrupt(R_BUTTON), turnToRight, FALLING);

    LcdWriteCmd(0x21);  //LCD extended commands
    LcdWriteCmd(0xB8);  //set LCD Vop (contrast)
    LcdWriteCmd(0x04);  //set temp coefficient
    LcdWriteCmd(0x14);  //LCD bias mode 1:40
    LcdWriteCmd(0x22);  //LCD basic commands
    LcdWriteCmd(0x0C);  //LCD all segments off
    LcdWriteCmd(0x09);  //LCD all segments on

    ClearScreen();

    Serial.println(sizeof scr_buffer);
    createSnake();
}

void loop(){
    while (TEST_DISPLAY) {
      LcdWriteCmd(0x09);
      delay(500);
      LcdWriteCmd(0x0c);
      delay(500);
    }
    if (TEST_PRINT_SNAKE){
      char info[16];
      for (int i=0; i<LEN(worm.parts); i++){
         sprintf(info, "Part # %d  X: %d  Y:  %d", i, worm.parts[i].x, worm.parts[i].y);
         Serial.println(info);
      }

    }

    static GAMESTATE  gamestate = GAME_PAUSE;
    static byte cycles = 0 ;
    byte scr_buffer[LCD_Y][LCD_X] = {0};


    //LcdWriteCmd(0x09);
    LcdWriteCmd(0x0c);

    if (gamestate == GAME_ON){
      if (ready_to_go){
        ready_to_go = 0;
        worm.dir = worm.new_dir;
        drawSnake();
        moveSnake();
        DrawScreen();
        if (isSelfCollision()){gamestate = GAME_OVER;}
        if (worm.parts[worm.head].x > LCD_X || worm.parts[worm.head].y > LCD_Y){gamestate = GAME_OVER;}
      }
    }
    if (gamestate == GAME_OVER){
      if(TEST_GAMESTATUS){Serial.println("GAME OVER");}
      delay(250);
      gamestate = GAME_PAUSE;
      setup();
    }
    if (gamestate == GAME_PAUSE){
      if(TEST_GAMESTATUS){Serial.println("GAME PAUSED");}
      drawSnake();
      DrawScreen();
       if ( (millis() - time_last_pressed) < 200){Serial.println("VITTTU"); gamestate = GAME_ON;}
       delay(50);
    }

}
