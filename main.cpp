#include "MicroBit.h"
#include <string>

#define JOYSTICK_LEFT_BOUND 1023
#define JOYSTICK_RIGHT_BOUND 0
// Enables the accessory pin.
//#define ACCESSORY



// Declare globals
MicroBit uBit;
//BOAT CODE
////BOAT
//MicroBitPin servo(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ANALOG);
//MicroBitPin fanRelay(MICROBIT_ID_IO_P1, MICROBIT_PIN_P1, PIN_CAPABILITY_DIGITAL);
//MicroBitPin accessoryRelay(MICROBIT_ID_IO_P2, MICROBIT_PIN_P2, PIN_CAPABILITY_DIGITAL);
//
//// REMOTE 
//MicroBitPin joystickX(MICROBIT_ID_IO_P10, MICROBIT_PIN_P10, PIN_CAPABILITY_ANALOG);
//MicroBitPin fanControl(MICROBIT_ID_IO_P16, MICROBIT_PIN_P16, PIN_CAPABILITY_DIGITAL);
//MicroBitPin accessoryControl(MICROBIT_ID_IO_P15, MICROBIT_PIN_P15, PIN_CAPABILITY_DIGITAL);
//MicroBitSerial serial(USBTX, USBRX); 
//
//MicroBitPin modePin(MICROBIT_ID_IO_P9, MICROBIT_PIN_P9, PIN_CAPABILITY_DIGITAL);
//bool isController = modePin.getDigitalValue();
// Will be initalized in main()
bool isController = false;

//BOAT
MicroBitPin servo(MICROBIT_ID_IO_P0, MICROBIT_PIN_P0, PIN_CAPABILITY_ANALOG);
MicroBitPin fanRelay(MICROBIT_ID_IO_P2, MICROBIT_PIN_P2, PIN_CAPABILITY_DIGITAL);
#ifdef ACCESSORY
MicroBitPin accessoryRelay(MICROBIT_ID_IO_P1, MICROBIT_PIN_P1, PIN_CAPABILITY_DIGITAL);
#endif

// REMOTE 
MicroBitPin joystickX(MICROBIT_ID_IO_P1, MICROBIT_PIN_P1, PIN_CAPABILITY_ANALOG);
MicroBitPin fanControl(MICROBIT_ID_IO_P16, MICROBIT_PIN_P16, PIN_CAPABILITY_DIGITAL);
#ifdef ACCESSORY
MicroBitPin accessoryControl(MICROBIT_ID_IO_P15, MICROBIT_PIN_P15, PIN_CAPABILITY_DIGITAL);
#endif
// TODO is this needed?
MicroBitSerial serial(USBTX, USBRX);

int isFanOn = 0;


// Create LED arrow bit maps
MicroBitImage east("0,0,255,0,0\n0,0,0,255,0\n255,255,255,255,1\n0,0,0,255,0\n0,0,255,0,0\n");
MicroBitImage northeast("0,255,255,255,255\n0,0,0,255,255\n0,0,255,0,255\n0,255,0,0,255\n255,0,0,0,0\n");
MicroBitImage north("0,0,255,0,0\n0,255,255,255,0\n255,0,255,0,255\n0,0,255,0,0\n0,0,255,0,0\n");
MicroBitImage northwest("255,255,255,255,0\n255,255,0,0,0\n255,0,255,0,0\n255,0,0,255,0\n0,0,0,0,0\n");
MicroBitImage west("0,0,255,0,0\n0,255,0,0,0\n255,255,255,255,255\n0,255,0,0,0\n0,0,255,0,0\n");

string directions[5] = { "east", "northeast", "north", "northwest", "west" };

// Turn boat fans on/off
void toggleFans(){
    isFanOn = !isFanOn;
    fanRelay.setDigitalValue(isFanOn);
}

// Display a directional arrow on the microbit led display
void displayLEDArrow(string direction){
    if(direction == "east"){
        uBit.display.print(east);
    } else if(direction == "northeast"){
        uBit.display.print(northeast);
    } else if(direction == "north"){
        uBit.display.print(north);
    } else if(direction == "northwest"){
        uBit.display.print(northwest);
    } else if(direction == "west"){
        uBit.display.print(west);
    }      
}

// Event handler for when data is received by the microbit
void onData(MicroBitEvent e)
{   
    PacketBuffer buffer = uBit.radio.datagram.recv();
    int fan = buffer[0];
    #ifdef ACCESSORY
    int accessory = buffer[1];
    #endif
    int unsigned direction = buffer[2] << 8 | buffer[3];
    int servoValue = ((direction / (float) 1024) - 0.5) * 90 + 90;
    
    printf("Fan: %d\r\n", fan);
    #ifdef ACCESSORY
    printf("Accessory: %d\r\n", accessory);
    #endif
    printf("Servo value: %d\r\n", servoValue);
    printf("Direction: %d\r\n", direction);


    if (fan){
        toggleFans();
        uBit.display.print("F");
    }
    #ifdef ACCESSORY
    accessoryRelay.setDigitalValue(accessory);
    #endif
    
    // Set rudder position.
    // Specs from http://www.ee.ic.ac.uk/pcheung/teaching/DE1_EE/stores/sg90_datasheet.pdf
    servo.setServoValue(servoValue/*, 1000, 1500*/);
    int arrowIndex = servoValue / (1024 / (float) 5);
    displayLEDArrow(directions[arrowIndex]);
}


int main()
{
    // Initialise the micro:bit runtime.
    uBit.init();
    
    // Initialize radio runtime
    uBit.radio.enable(); 
    
    // Set output power level to max value
    uBit.radio.setTransmitPower(7);
    
    // Set channel that microbits will communicate on
    uBit.radio.setGroup(1);
    
    isController = uBit.buttonA.isPressed();
    
    if (!isController) {
       // Calls onData function when MICROBIT_RADIO_EVT_DATAGRAM (datagram received) event is raised by MICROBIT_ID_RADIO
        uBit.messageBus.listen(MICROBIT_ID_RADIO, MICROBIT_RADIO_EVT_DATAGRAM, onData);
    }
    
    uBit.display.print(isController ? "C" : "B");
    uBit.sleep(3000);
    
    while(1){
        if (isController) {
            uint8_t motor = uBit.buttonA.isPressed();
            // fanControl.getDigitalValue();
            #ifdef ACCESSORY
            uint8_t accessory = uBit.buttonB.isPressed();
            // accessoryControl.getDigitalValue();
            #else
            uint8_t accessory = 0;
            #endif
    
            
            // Normalize the joystick input on the controller because it's easier to reprogram than the boat.
            float range = JOYSTICK_LEFT_BOUND - JOYSTICK_RIGHT_BOUND;
    //        int center = (JOYSTICK_LEFT_BOUND + JOYSTICK_RIGHT_BOUND) / 2;
    //        // TODO reverse operands if necessary.
    //        int offset = joystickX.getAnalogValue() - center;
    //        float relativeOffset = (float) offset / (range / 2);
    //        if (relativeOffset > 1) {
    //            relativeOffset = 1;
    //        } else if (relativeOffset < -1) {
    //            relativeOffset = -1;
    //        }
            int analogValue = joystickX.getAnalogValue();
    //        serial.send("analogValue: ");
    //        serial.send(analogValue);
    //        serial.send("\n");
            
            int normalizedInput = ((analogValue - JOYSTICK_RIGHT_BOUND) / range) * 1024;
            if (normalizedInput > 1023) {
                normalizedInput = 1023;
            } else if (normalizedInput < 0) {
                normalizedInput = 0;
            }

            
            int arrowIndex = normalizedInput / (1024 / (float) 5);

            displayLEDArrow(directions[arrowIndex]);
            
            int unsigned normalizedInputUnsigned = normalizedInput;
            
            // TODO set rudder values
            uint8_t packet[4] = {motor, accessory, normalizedInputUnsigned >> 8, normalizedInputUnsigned & 255};
            uBit.radio.datagram.send(&packet[0], 4);


        }
        uBit.sleep(100); 
    }

}