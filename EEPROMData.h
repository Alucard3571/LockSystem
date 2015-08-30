// TODO: include the EEPROM header here.
#define MAX_DATA_SIZE 20

// Macros
#define CLEAR(s) memset(&(s), 0, sizeof(s))

static const unsigned int START_MARKER_SIGNATURE = 0xABCD;
static const unsigned int END_MARKER_SIGNATURE = 0xDCBA;

struct DataBlock
{
  unsigned int startMarker;
  unsigned int dataSize;
  char data[MAX_DATA_SIZE];
  unsigned int endMarker;
};

class EEPROMData
{

public:
   EEPROMData(unsigned int eepromStartAddress):
      _eepromStartAddress(eepromStartAddress)
   {
   }

   ~EEPROMData() 
   {
   }
      
   void writeData(String data)
   {
      data.trim();
      DataBlock structData;
      initializeDataBlock(structData);
      memcpy(&structData.data[0], &data[0], data.length());
      structData.dataSize = data.length();
      _EEPROM_write(_eepromStartAddress, structData);
   }

   bool readData(String& data)
   {
      DataBlock structData;
      initializeDataBlock(structData);
      _EEPROM_read(_eepromStartAddress, structData);
    
      if(
          structData.startMarker == START_MARKER_SIGNATURE &&
          structData.endMarker == END_MARKER_SIGNATURE &&
          data.length() >= structData.dataSize
        )
      {
        // clear passed String
        // Remove trailing characters.
        if(data.length() > structData.dataSize)
        {
           data.remove(structData.dataSize, (data.length() - structData.dataSize));
        }
        memcpy(&data[0], &structData.data[0], structData.dataSize);
        return true;
      }
      else
      {
        return false;
      }
   }

private:

  unsigned int _EEPROM_write(unsigned int address, const DataBlock& value)
  {
      const byte* ptr = (const byte*)(const void*)&value;
      unsigned int i;
      for (i = 0; i < sizeof(value); i++)
      {
         EEPROM.write(address++, *ptr++);
      }
      return i;
  }

  unsigned int _EEPROM_read(unsigned int address, DataBlock& value)
  {
      byte* ptr = (byte*)(void*)&value;
      unsigned int i;
      for (i = 0; i < sizeof(value); i++)
      {
         *ptr++ = EEPROM.read(address++);
      }
      return i;
  }

  void initializeDataBlock(DataBlock& data)
  {
      data.startMarker = START_MARKER_SIGNATURE;
      data.endMarker = END_MARKER_SIGNATURE;
      data.dataSize = 0;
      CLEAR(data.data);  
  }

  unsigned int _eepromStartAddress;
};

