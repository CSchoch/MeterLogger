/*

*/

#include <SMLParser.h>
#include <DebugUtils.h>
#include <sml_crc16.h>

// SMLParser

SMLParser::SMLParser(int uart_nr)
{
  if (0 > uart_nr || uart_nr > 2)
  {
    log_e("Serial number is invalid, please use 0, 1 or 2");
    return;
  }
  this->_uart_nr = uart_nr;
  this->stage = 0;
  this->stopIndex = 0;
  this->startIndex = 0;
}

void SMLParser::Begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin, bool invert)
{
  switch (_uart_nr)
  {
  case 0:
    Serial.begin(baud, config, rxPin, txPin, invert);
    this->MySerial = &Serial;
    break;
  case 1:
    Serial1.begin(baud, config, rxPin, txPin, invert);
    this->MySerial = &Serial1;
    break;
  case 2:
    Serial2.begin(baud, config, rxPin, txPin, invert);
    this->MySerial = &Serial2;
    break;
  }
}

SMLParser::~SMLParser()
{
  switch (_uart_nr)
  {
  case 0:
    Serial.end();
    break;
  case 1:
    Serial1.end();
    break;
  case 2:
    Serial2.end();
    break;
  }
}

void SMLParser::loop()
{
  switch (stage)
  {
  case 0:
    stopIndex = 0;
    findStartSequence();
    break;
  case 1:
    startIndex = 0;
    findStopSequence();
    break;
  case 2:
    publishMessage();
    break;
  }
}

double SMLParser::getTotalConsumption()
{
  return this->totalConsumption;
}

double SMLParser::getTarif1Consumption()
{
  return this->tarif1Consumption;
}

double SMLParser::getTarif2Consumption()
{
  return this->tarif2Consumption;
}

double SMLParser::getTotalSupply()
{
  return this->totalSupply;
}

double SMLParser::getTarif1Supply()
{
  return this->tarif1Supply;
}

double SMLParser::getTarif2Supply()
{
  return this->tarif2Supply;
}

long SMLParser::getActualPower()
{
  return this->actualPower;
}

bool SMLParser::getAvailable()
{
  return this->available;
}

void SMLParser::resetAvailable()
{
  this->available = false;
}

void SMLParser::findStartSequence()
{
  while (MySerial->available())
  {
    byte inByte = MySerial->read();
    int cycles = 0;

    DEBUGPRINTLNVERBOSE(inByte, HEX);

    while (cycles <= 1)
    {
      if (inByte == startSequence[startIndex])
      {
        smlMessage[startIndex] = inByte;
        startIndex++;
        cycles = 2;
        if (startIndex == sizeof(startSequence))
        {
          stage = 1;
          smlIndex = startIndex;
          startIndex = 0;
        }
      }
      else
      {
        startIndex = 0;
        cycles++;
      }
    }
  }
}

void SMLParser::findStopSequence()
{
  while (MySerial->available())
  {
    byte inByte = MySerial->read();
    int cycles = 0;

    DEBUGPRINTLNVERBOSE(inByte, HEX);

    smlMessage[smlIndex++] = inByte;

    while (cycles <= 1)
    {
      if (inByte == stopSequence[stopIndex])
      {
        stopIndex++;
        cycles = 2;
        if (stopIndex == sizeof(stopSequence))
        {
          stage = 2;
          stopIndex = 0;

          // after the stop sequence, ther are sill 3 bytes to come.
          // One for the amount of fillbytes plus two bytes for calculating CRC.
          for (int c = 0; c < 3; c++)
          {
            smlMessage[smlIndex++] = MySerial->read();
          }
          smlIndex--;
        }
      }
      else
      {
        stopIndex = 0;
        cycles++;
      }
    }
    if (smlIndex >= sizeof(smlMessage) - 3)
    {
      smlIndex = 0;
      stopIndex = 0;
      stage = 0;
      break;
    }
  }
}

int SMLParser::findSequence(const byte *message, const byte *sequence, int sizeOfSequence)
{
  int counter = 0;
  int offset = 0;
  int sequenceIndex = 0;

  while (counter <= smlIndex)
  {
    offset = 1;
    if (message[counter] == sequence[sequenceIndex])
    {
      sequenceIndex++;
      if (sequenceIndex == sizeOfSequence)
      {
        DEBUGPRINTLNVERBOSE("found sequence");
        sequenceIndex = 0;
        counter++;
        return counter;
      }
    }
    else
    {
      if (sequenceIndex > 0)
      {
        offset = 0;
        sequenceIndex = 0;
      }
    }
    counter = counter + offset;
  }
  return -1;
}

long SMLParser::getLong(const byte *message, int pos)
{
  long value;
  value = smlMessage[pos];
  for (int i = 1; i < 4; i++)
  {
    value <<= 8;
    value |= smlMessage[pos + i];
  }
  return value;
}

unsigned long SMLParser::getULong(const byte *message, int pos)
{
  unsigned long value;
  value = smlMessage[pos];
  for (int i = 1; i < 4; i++)
  {
    value <<= 8;
    value |= smlMessage[pos + i];
  }
  return value;
}

unsigned long long SMLParser::getULongLong(const byte *message, int pos)
{
  unsigned long long value;
  value = smlMessage[pos];
  for (int i = 1; i < 8; i++)
  {
    value <<= 8;
    value |= smlMessage[pos + i];
  }
  return value;
}

void SMLParser::publishMessage()
{
#if DEBUGLEVEL >= DEBUG
int arrSize = 2 * smlIndex + 1;
  char smlMessageAsString[arrSize];
  char *myPtr = &smlMessageAsString[0]; //or just myPtr=charArr; but the former described it better.
#endif
  int pos;
  word valueFound = 0;
  word compareValue = 0;
  unsigned long long totalConsumption = 0;
  unsigned long long tarif1Consumption = 0;
  unsigned long long tarif2Consumption = 0;
  unsigned long long totalSupply = 0;
  unsigned long long tarif1Supply = 0;
  unsigned long long tarif2Supply = 0;

  pos = findSequence(smlMessage, totalConsumptionSequence, sizeof(totalConsumptionSequence));
  if (pos != -1)
  {
    DEBUGPRINTDEBUG("Found total consumption sequence ");
    DEBUGPRINTLNDEBUG(pos);
    totalConsumption = getULongLong(smlMessage, pos + 11);
    valueFound |= totalConsumptionCheck;
  }

  pos = findSequence(smlMessage, tarif1ConsumptionSequence, sizeof(tarif1ConsumptionSequence));
  if (pos != -1)
  {
    DEBUGPRINTDEBUG("Found tarif 1 consumption sequence ");
    DEBUGPRINTLNDEBUG(pos);
    tarif1Consumption = getULongLong(smlMessage, pos + 7);
    valueFound |= tarif1ConsumptionCheck;
  }

  pos = findSequence(smlMessage, tarif2ConsumptionSequence, sizeof(tarif2ConsumptionSequence));
  if (pos != -1)
  {
    DEBUGPRINTDEBUG("Found tarif 2 consumption sequence ");
    DEBUGPRINTLNDEBUG(pos);
    tarif2Consumption = getULongLong(smlMessage, pos + 7);
    valueFound |= tarif2ConsumptionCheck;
  }

  compareValue = totalConsumptionCheck | tarif1ConsumptionCheck | tarif2ConsumptionCheck;
  if ((tarif1Consumption + tarif2Consumption == totalConsumption) && ((valueFound & compareValue) == compareValue))
  {
    double doubleValue = (totalConsumption / 10000.0);
    this->totalConsumption = doubleValue;
    doubleValue = (tarif1Consumption / 10000.0);
    this->tarif1Consumption = doubleValue;
    doubleValue = (tarif2Consumption / 10000.0);
    this->tarif2Consumption = doubleValue;
    DEBUGPRINTLNDEBUG("Consumption counters Ok");
  }
  else
  {
    valueFound &= ~(totalConsumptionCheck | tarif1ConsumptionCheck | tarif2ConsumptionCheck);
    DEBUGPRINTLNDEBUG("Consumption counters NOk");
  }

  pos = findSequence(smlMessage, totalSupplySequence, sizeof(totalSupplySequence));
  if (pos != -1)
  {
    DEBUGPRINTDEBUG("Found total supply sequence ");
    DEBUGPRINTLNDEBUG(pos);
    totalSupply = getULongLong(smlMessage, pos + 7);
    valueFound |= totalSupplyCheck;
  }

  pos = findSequence(smlMessage, tarif1SupplySequence, sizeof(tarif1SupplySequence));
  if (pos != -1)
  {
    DEBUGPRINTDEBUG("Found tarif 1 supply sequence ");
    DEBUGPRINTLNDEBUG(pos);
    tarif1Supply = getULongLong(smlMessage, pos + 7);
    valueFound |= tarif1SupplyCheck;
  }

  pos = findSequence(smlMessage, tarif2SupplySequence, sizeof(tarif2SupplySequence));
  if (pos != -1)
  {
    DEBUGPRINTDEBUG("Found tarif 2 supply sequence ");
    DEBUGPRINTLNDEBUG(pos);
    tarif2Supply = getULongLong(smlMessage, pos + 7);
    valueFound |= tarif2SupplyCheck;
  }

  compareValue = totalSupplyCheck | tarif1SupplyCheck | tarif2SupplyCheck;
  if ((tarif1Supply + tarif2Supply == totalSupply) && ((valueFound & compareValue) == compareValue))
  {
    double doubleValue = (totalSupply / 10000.0);
    this->totalSupply = doubleValue;
    doubleValue = (tarif1Supply / 10000.0);
    this->tarif1Supply = doubleValue;
    doubleValue = (tarif2Supply / 10000.0);
    this->tarif2Supply = doubleValue;
    DEBUGPRINTLNDEBUG("Supply counters Ok");
  }
  else
  {
    valueFound &= ~(totalSupplyCheck | tarif1SupplyCheck | tarif2SupplyCheck);
    DEBUGPRINTLNDEBUG("Supply counters NOk");
  }

  pos = findSequence(smlMessage, actualPowerSequence, sizeof(actualPowerSequence));
  if (pos != -1)
  {
    DEBUGPRINTDEBUG("Found actual power sequence ");
    DEBUGPRINTLNDEBUG(pos);
    long value = getLong(smlMessage, pos + 7);
    this->actualPower = value;
    DEBUGPRINTLNDEBUG(value);
    valueFound |= actualPowerCheck;
  }

#if DEBUGLEVEL >= DEBUG
  myPtr = &smlMessageAsString[0];
  for (int i = 0; i <= smlIndex; i++)
  {
    snprintf(myPtr, 3, "%02X", smlMessage[i]); //convert a byte to character string, and save 2 characters (+null) to charArr;
    myPtr += 2;                                //increment the pointer by two characters in charArr so that next time the null from the previous go is overwritten.
  }
#endif
  //DEBUGPRINTDEBUG(_uart_nr);
  //DEBUGPRINTDEBUG(" ");
  DEBUGPRINTLNDEBUG(smlMessageAsString); // for debuging
  DEBUGPRINTDEBUG(smlMessageAsString[smlIndex - 1], HEX);
  DEBUGPRINTLNDEBUG(smlMessageAsString[smlIndex - 0], HEX);
  DEBUGPRINTLNDEBUG(sml_crc16_calculate(smlMessage, smlIndex - 2), HEX);
  DEBUGPRINTLNDEBUG(smlIndex);

  smlIndex = 0;
  stage = 0; // start over
  compareValue = totalConsumptionCheck | tarif1ConsumptionCheck | tarif2ConsumptionCheck | totalSupplyCheck | tarif1SupplyCheck | tarif2SupplyCheck;
  this->available = ((valueFound & compareValue) == compareValue);
}
