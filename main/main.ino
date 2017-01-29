#define INCREMENT 128
/*
    Analog sensor pins.
*/
int lightInPin = A0; // Light sensor pin.
int tempInPin = A1; // Temperature sensor pin.

/*
    Digital output pins.
*/
int ledPin = 8; // LED that will vary in blinking rate.
int pumpPin = 9; // Pump that will activate the relay to turn the pump on.
int lightOutPin = 11; // Light output pin to activate the relay.

/*
    Analog output pins.
*/
int tempOutPin = A2;

/*
bool processed = false;
bool receivedData = false;
*/

uint8_t data[1024];
int length;

// TODO: Setup the rest of the pins.
void setup()
{
    pinMode(lightInPin, INPUT);
    pinMode(tempInPin, INPUT);

    pinMode(ledPin, OUTPUT);
    pinMode(pumpPin, OUTPUT);
    pinMode(lightOutPin, OUTPUT);

    pinMode(tempOutPin, OUTPUT);

    Serial.begin(9600);
}

// Return argument length of data received from
// master.
short argLength(uint8_t *recvData)
{
    uint8_t theBytes[2];
    theBytes[0] = recvData[2];
    theBytes[1] = recvData[3];
    short ret = *(short*) theBytes;
    return ret;
}

/*
void requestHandler()
{
    // Handle case where we receive a read
    // request when a command was never received.
    if (!receivedData)
    {
        Wire.beginTransmission(ADDRESS);
        Wire.write(-1);
        Wire.endTransmission();
        return;
    }
    // If data has not been processed, and a
    // command was sent by the master, then
    // wait until the data has been processed.
    while (!processed)
    {
        delay(100);
    }

    Wire.beginTransmission(ADDRESS);
    Wire.write(data, length);
    Wire.endTransmission();

    // Finished processing data, so
    // now we can receive another command.
    processed = false;
    receivedData = false;
}
*/

// Set status code for failure.
void fail()
{
    data[0] = -1;
}

// Set status code for done.
void done()
{
    data[0] = 0;
}

/*
// Pretty print data received from master.
void prettyPrintData(uint8_t *recvData)
{
    short len = argLength(recvData);
    int devCode = recvData[0];
    int actCode = recvData[1];
    byte args[len];
    for (int i = 0; i < len; i++)
    {
        args[i] = recvData[i+4];
    }

    Serial.print("Received data from master. Device code: ");
    Serial.print(devCode);
    Serial.print(". Action code: ");
    Serial.print(actCode);
    Serial.print(". Arguments:");
    for (int i = 0; i < len; i++)
    {
      Serial.print(args[i], HEX);
    }
    Serial.print('\n');
}
*/

// Handler for data received from master.
short receiveHandler(int numBytes, uint8_t *recvData)
{
    if (numBytes == 0)
    {
        return;
    }

    short dataLength = 0;

    //prettyPrintData(recvData);

    switch (recvData[0])
    {
        // Pump device.
        case 0:
            // Serial.println("Pump action.");
            dataLength = pump(recvData);
            break;
        // Light output device.
        case 1:
            // Serial.println("Light out action.");
            dataLength = lightOut(recvData);
            break;
        // Light sensor device.
        case 2:
            // Serial.println("Light in action.");
            dataLength = lightIn(recvData);
            break;
        // Temperature output device.
        case 3:
            // Serial.println("Temp out action.");
            dataLength = tempOut(recvData);
            break;
        // Temperature sensor device.
        case 4:
            // Serial.println("Temp in action.");
            dataLength = tempIn(recvData);
            break;
        // Bad device code.
        case 255:
            // Serial.println("Echo action.");
            dataLength = echo(recvData);
            break;
        default:
            // Serial.print("Did not recognize device code ");
            // Serial.print(recvData[0]);
            // Serial.println(".");
            fail();
            break;
    }
    // Set lengths.
    uint8_t *shortBytes = (uint8_t*) &dataLength;
    // TODO: Check endianness.
    data[1] = shortBytes[0];
    data[2] = shortBytes[1];
    return dataLength;
}

// Pump device action handler. Return number of data
// bytes.
short pump(uint8_t *recvData)
{
    switch(recvData[1])
    {
        // Info.
        case 0:
            data[3] = readPinState(pumpPin);
            done();
            return 1;
        // Set value.
        case 1:
            int setVal = recvData[4];

            // Set pump pin.
            if(writePinState(pumpPin, setVal))
            {
                done();
            }
            else
            {
                fail();
            }

            return 0;
        default:
            fail();
            return 0;
    }
}

// Write to a pin, checking the value provided.
// Return false if value is not 1 or 0.
bool writePinState(int pin, int val)
{
    // Check if we have illegal value.
    if (val != 1 && val != 0)
    {
        return false;
    }
    switch (val)
    {
        case 1:
            digitalWrite(pin, HIGH);
            break;
        case 0:
            digitalWrite(pin, LOW);
    }
    return true;
}

// Convenience function to read pin digital
// state and return 1 or 0 for HIGH and LOW.
// Only reads digitally.
int readPinState(int pin)
{
    int pinStatus = digitalRead(pin);
    if (pinStatus == HIGH)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

// Light output action handler.
short lightOut(uint8_t *recvData)
{
    switch (recvData[1])
    {
        // Info.
        case 0:
            data[3] = readPinState(lightOutPin);
            done();
            return 1;
        // Set value.
        case 1:
            if (writePinState(lightOutPin, recvData[2]))
            {
                done();
            }
            else
            {
                fail();
            }
            return 0;
    }
}

// Test echo action handler.
short echo(uint8_t *recvData)
{
    switch (recvData[1])
    {
        case 0:
            short theShort = argLength(recvData);
            if (theShort != 1)
            {
                // Serial.println(theShort);
                // Serial.println(theShort);
                
                fail();
                return 0;
            }
            // Serial.print("Data received was: ");
            // Serial.println(recvData[4], HEX);
            // Copy data back.
            data[3] = recvData[4];
            // data[3] = 5;
            done();
            return 1;
        default:
            // Serial.println("wtf mate");
            fail();
            return 0;
    }
}

// Function to read voltage of an analog pin.
double voltageAt(int pin)
{
    return 0.0049 * analogRead(pin);
}

// Light sensor action handler. Returns number
// of bytes of data.
short lightIn(uint8_t *recvData)
{
    switch (recvData[1])
    {
        // Info. And only info.
        case 0:
            double voltage = voltageAt(lightInPin);
            uint8_t *voltagePtr = (uint8_t*) &voltage;
            for (int i = 3; i < 11; i++)
            {
                data[i] = voltagePtr[i-3];
            }
            done();
            return 8;
        default:
            fail();
            return 0;
    }
}

// Handler function for temperature setting.
short tempOut(uint8_t *recvData)
{
    switch (recvData[1])
    {
        // Info.
        case 0:
            // Check bad argument length.
            if (argLength(recvData) != 0)
            {
                fail();
                return 0;
            }
            double voltage = voltageAt(tempOutPin);
            uint8_t *temperature = (uint8_t*) &voltage;
            for (int i = 3; i < 11; i++)
            {
                data[i] = temperature[i-3];
            }
            done();
            return 8;
        // Set temperature level.
        case 1:
            // Expect float as argument.
            if (argLength(recvData) != 4)
            {
                fail();
                return 0;
            }

            uint8_t tempBytes[4];
            for (int i = 0; i < 4; i++)
            {
                tempBytes[i] = recvData[i+4];
            }

            float setTemp = *(float*) tempBytes;

            analogWrite(tempOutPin, setTemp);
            done();
            return 0;
        default:
            fail();
            return 0;
    }
}

// Handler action for temperature sensor.
short tempIn(uint8_t *recvData)
{
    switch (recvData[1])
    {
        case 0:
            double voltage = voltageAt(tempInPin);
            uint8_t *voltagePtr = (uint8_t*) &voltage;
            for (int i = 3; i < 11; i++)
            {
                data[i] = voltagePtr[i-3];
            }
            done();
            return 8;
        default:
            fail();
            return 0;
    }
}

int MAX_SIZE = 128;
uint8_t *recvData = malloc(MAX_SIZE);

int currPos = 0;

void blockingRead(int amount)
{
    int completed = 0;
    while (completed < amount)
    {
        int temp = Serial.read();
        if (temp == -1)
        {
            continue;
        }
        
        recvData[currPos] = temp;
        currPos++;
        completed++;
    }
}

void loop()
{
    /*
        All serial reading puts data into the 
        recvData global variable.

        Another global variable, currPos,
        keeps track of the index that we currently
        are at in recvData.

        After we've read all our data, we can process it
        and return synchronously. The loops handles
        poorly timed serial input too, allowing us
        to read partial serial input.

        There is no way to recover from data loss.
    */
    // Read device id, action id, and length of args.
    blockingRead(4);

    // Calculate length of args for next blocking read.
    short len = argLength(recvData);

    // Read the rest of the arguments.
    blockingRead(len);

    currPos = 0;

    short responseLength = receiveHandler(len, recvData) + 3;

    for (int i = 0; i < responseLength; i++)
    {
        Serial.write(data[i]);
    }
}
