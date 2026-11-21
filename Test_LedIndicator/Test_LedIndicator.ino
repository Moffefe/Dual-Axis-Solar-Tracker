#include <RemoteCodes.h>
#include <IRremote.h>
#include <LedIndicator.h>


// Zmienne globalne
int RECV_PIN = 3;
IRrecv irrecv(RECV_PIN);
decode_results results;
LedIndicator* pIndicator;
PCF8574* pExpander;
byte mode = 0b00000000;






void setup()
{
  Serial.begin(9600);
  irrecv.enableIRIn(); // Start the receiver

  pExpander = new PCF8574();
  pExpander->begin(0x20);
  pIndicator = new LedIndicator(pExpander, 2, 3, 4, 5);
}

void loop()
{
  if (irrecv.decode(&results))
  {
    switch(results.value)
    {
      case BUTTON_ON:
        pIndicator->SetState(LedIndicator::State::off);
        break;

      case BUTTON_1:
        pIndicator->SetState(LedIndicator::State::foldDown);
        break;

      case BUTTON_2:
        pIndicator->SetState(LedIndicator::State::sleep);
        break;

      case BUTTON_3:
        pIndicator->SetState(LedIndicator::State::flashLed, 0b00001011 | mode);     // Zeby miec pewnosc, ze dla kilku diod tez zadziala
        break;

      case BUTTON_UP:
        pIndicator->SetState(LedIndicator::State::flashLed, 0b00000001 | mode);
        break;

      case BUTTON_DOWN:
        pIndicator->SetState(LedIndicator::State::flashLed, 0b00000010 | mode);
        break;

      case BUTTON_LEFT:
        pIndicator->SetState(LedIndicator::State::flashLed, 0b00000100 | mode);
        break;

      case BUTTON_RIGHT:
        pIndicator->SetState(LedIndicator::State::flashLed, 0b00001000 | mode);
        break;

      case BUTTON_OK:
      {
        if(mode == 0b00000000)
        {
          mode = 0b01000000;
          Serial.println("Tryb ogolny");
          break;
        }
        if(mode == 0b01000000)
        {
          mode = 0b00100000;
          Serial.println("Tryb ON");
          break;
        }
        if(mode == 0b00100000)
        {
          mode = 0b00000000;
          Serial.println("Tryb OFF");
          break;
        }
      }

      case BUTTON_4:
      {
        pIndicator->SetState(LedIndicator::State::blink, 0, 254);
      }
    }
    
    irrecv.resume(); // Receive the next value
  }

  updateManager.UpdateAll();
  delay(20);
}
