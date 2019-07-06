/*
*/

#ifndef SMLParser_H
#define SMLParser_H
#define DEBUGLEVEL NONE

#include <Arduino.h>

class SMLParser
{
public:
    // The constructor for SMLParser requires no arguments
    SMLParser(int uart_nr);

    // Initialize SMLParser
    void Begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin, bool invert);
    void loop();
	
	double getTotalConsumption();
	double getTarif1Consumption();
	double getTarif2Consumption();
	double getTotalSupply();
	double getTarif1Supply();
	double getTarif2Supply();
	long getActualPower();
	bool getAvailable();
	void resetAvailable();
    
    // Destructor for SMLParser class
    ~SMLParser();
    
private:
	// Constants
	const byte startSequence[8] = { 0x1B, 0x1B, 0x1B, 0x1B, 0x01, 0x01, 0x01, 0x01 }; // see sml protocol
	const byte stopSequence[5]  = { 0x1B, 0x1B, 0x1B, 0x1B, 0x1A };
	const byte totalConsumptionSequence[7]  = { 0x07, 0x01, 0x00, 0x01, 0x08, 0x00, 0xFF };
	const byte tarif1ConsumptionSequence[7]  = { 0x07, 0x01, 0x00, 0x01, 0x08, 0x01, 0xFF };
	const byte tarif2ConsumptionSequence[7]  = { 0x07, 0x01, 0x00, 0x01, 0x08, 0x02, 0xFF };
	const byte totalSupplySequence[7]  = { 0x07, 0x01, 0x00, 0x02, 0x08, 0x00, 0xFF };
	const byte tarif1SupplySequence[7]  = { 0x07, 0x01, 0x00, 0x02, 0x08, 0x01, 0xFF };
	const byte tarif2SupplySequence[7]  = { 0x07, 0x01, 0x00, 0x02, 0x08, 0x02, 0xFF };
	const byte actualPowerSequence[7]  = { 0x07, 0x01, 0x00, 0x10, 0x07, 0x00, 0xFF };
	
	const word totalConsumptionCheck = 0x0001;
	const word tarif1ConsumptionCheck = 0x0002;
	const word tarif2ConsumptionCheck = 0x0004;
	const word totalSupplyCheck = 0x0005;
	const word tarif1SupplyCheck = 0x0010;
	const word tarif2SupplyCheck = 0x0020;
	const word actualPowerCheck = 0x0040;

    // Data
	int _uart_nr;
	byte smlMessage[700]; // for storing the the isolated message. Mine was 280 bytes, but may vary...
	int smlIndex;     // represents the actual position in smlMessage
	int startIndex;   // for counting startSequence hits
	int stopIndex;    // for counting stopSequence hits
	int stage;        // defines what to do next. 0 = searchStart, 1 = searchStop, 2 = publish message
	bool available;
	double totalConsumption;
	double tarif1Consumption;
	double tarif2Consumption;
	double totalSupply;
	double tarif1Supply;
	double tarif2Supply;
	long actualPower;
	HardwareSerial *MySerial;
	
	void findStartSequence();
	void findStopSequence();
	int findSequence(const byte * message, const byte * sequence, int sizeOfSequence);
	long getLong(const byte * message, int pos);
	unsigned long getULong(const byte * message, int pos);
	unsigned long long getULongLong(const byte * message, int pos);
	void publishMessage();
}; //-- SMLParser


#endif
