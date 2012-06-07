#include <EEPROM.h>
// ATmega168 and ATmega8 has 4096 bytes of EEPROM

// first 2 bytes for length, last 1 byte for checksum
#define MAX_EEPROM_LEN (4096 - 2 - 1)

unsigned int banner_buffer_len = 0;
byte banner_buffer[MAX_EEPROM_LEN] = {
  /*
     0xFF, 0xFF,	//OOOOOOOOOOOOOOOO
     0xFF, 0xFF,	//OOOOOOOOOOOOOOOO
     0xC0, 0x03,	//OO............OO
     0xC0, 0x03,	//OO............OO
     0xDF, 0x7F,	//OO.OOOOO.OOOOOOO
     0xDF, 0x7F,	//OO.OOOOO.OOOOOOO
     0xDF, 0x7F,	//OO.OOOOO.OOOOOOO
     0xDF, 0x7F,	//OO.OOOOO.OOOOOOO
     0xCE, 0x3F,	//OO..OOO...OOOOOO
     0xE0, 0x83,	//OOO.....O.....OO
     0xF1, 0xC3,	//OOOO...OOO....OO
     0xFF, 0xFF,	//OOOOOOOOOOOOOOOO
     0xFF, 0xFF,	//OOOOOOOOOOOOOOOO
   */
};


unsigned long time;
unsigned long new_time;
unsigned long old_time;
unsigned long dp_time;

unsigned long wait_time;
unsigned long data_end;
unsigned long data_start;

void dummy_banner();
bool load_banner();
void store_banner();

void setup()
{
  PORTB = 0x3F;
  PORTC = 0x3F;
  PORTD = 0xF0;

  DDRB = 0x3F;
  DDRC = 0x3F;
  DDRD = 0xF0;

  Serial.begin(9600);

  Serial.println("Hello led_pov");

  if (false == load_banner())
    dummy_banner();
}

void loop()
{
  if (Serial.available() > 1 && Serial.read() == '#') {
    delay(10000);
    banner_buffer_len = 0;
    banner_buffer_len |= Serial.read() << 8;
    banner_buffer_len |= Serial.read();

    if (banner_buffer_len > MAX_EEPROM_LEN) {
      dummy_banner();
    } else {
      for (int i = 0; i < banner_buffer_len; i++)
        banner_buffer[i] = Serial.read();

      store_banner();
    }
  }

  if ((PIND & 0x0C) == 0x08) {
    delay(40);
    new_time = micros();
    time = new_time - old_time;
    old_time = new_time;
    wait_time = time / 2 - ((data_end - data_start) / 2) ;

    while (micros() < (new_time + wait_time)) if ((PIND & 0x0C) == 0x04)  break ;

    data_start = micros();
    DP_L();
    data_end = micros();
    while (!((PIND & 0x0C) == 0x04)) ;
  } else if ((PIND & 0x0C) == 0x04) {
    delay(40);
    new_time = micros();
    time = new_time - old_time;
    old_time = new_time;
    wait_time = time / 2 - ((data_end - data_start) / 2) ;

    while (micros() < (new_time + wait_time)) if ((PIND & 0x0C) == 0x08)  break ;

    data_start = micros();
    DP_R();
    data_end = micros();
    while (!((PIND & 0x0C) == 0x08)) ;
  }

}

void clean_line(void)
{
  PORTD = ~0;
  PORTB = ~0;
  PORTC = ~0;
}

void draw_line(byte msb, byte lsb)
{
  PORTD = lsb << 4;         // PD4 : lsb
  PORTB = ((msb << 4) & 0x30)  | ((lsb >> 4) & 0x0F);
  PORTC = (msb >> 2) & 0x3F;     // PC5 : msb
}

void DP_R()
{
  for (int i = 0; i < banner_buffer_len; i += 2) {
    if ((PIND & 0x0C) == 0x08)
      break;

    draw_line(banner_buffer[i], banner_buffer[i + 1]);
    delay(1);
  }
  clean_line();
}

void DP_L()
{
  for (int i = banner_buffer_len - 2; i > 0; i -= 2) {
    if ((PIND & 0x0C) == 0x04)
      break;

    draw_line(banner_buffer[i], banner_buffer[i + 1]);
    delay(1);
  }
  clean_line();
}


bool load_banner()
{
  Serial.println("Loading banner...");

  banner_buffer_len = 0;
  banner_buffer_len = EEPROM.read(0) << 8;
  banner_buffer_len |= EEPROM.read(1) << 8;

  if (banner_buffer_len == 0 || banner_buffer_len > MAX_EEPROM_LEN)
    return false;

  byte checksum = 0;
  for (int i = 0; i < banner_buffer_len; i++) {
    banner_buffer[i] = EEPROM.read(i + 2);
    checksum += banner_buffer[i];
  }

  if (checksum != EEPROM.read(2 + banner_buffer_len + 1))
    return false;

  return true;
  Serial.println("OK");
}

void store_banner()
{
  Serial.println("Storing banner...");

  EEPROM.write(0, (banner_buffer_len >> 8) & 0xff);
  EEPROM.write(1, (banner_buffer_len) & 0xff);

  byte checksum = 0;
  for (int i = 0; i < banner_buffer_len; i++) {
    EEPROM.write(i + 2, banner_buffer[i]);
    checksum += banner_buffer[i];
  }

  EEPROM.write(2 + banner_buffer_len + 1, checksum);

  Serial.println("OK");
}

void dummy_banner()
{
  Serial.println("Loading dummy banner...");

  banner_buffer_len = MAX_EEPROM_LEN;
  for (int i = 0; i < banner_buffer_len; i++)
    banner_buffer[i] = 1 << (i % 8);

  Serial.println("OK");
}
