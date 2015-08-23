// TODO: include the headers here.
#define EEPROM_START_ADDRESS 0x00
#define START_MARKER_SIGNATURE 0xABCD
#define END_MARKER_SIGNATURE 0xDCBA

class EEPROMData
{
public:
   EEPROMData(unsigned int dataSize):
      _dataSize(dataSize),
      _dataIsValid(false)
   {
   }

   ~EEPROMData() 
   {
   }
      
   void writeData(String data)
   {
      // TODO: add crc checks.
      _EEPROM_write(EEPROM_START_ADDRESS, data);
   }

   String readData()
   {
      String readData(20);
      _EEPROM_read(EEPROM_START_ADDRESS, readData);
      bool startSignature, endSignature;
      startSignature = memcmp(&_startMarkerSignature, &readData[0], sizeof(_startMarkerSignature));
      endSignature = memcmp(&_endMarkerSignature, &readData[sizeof(_startMarkerSignature)+_dataSize], sizeof(_endMarkerSignature));
      if(startSignature == 0 && endSignature == 0)
      {
        _dataIsValid = true;
      }
      else
      {
        _dataIsValid = false;
      }
      String data(_dataSize + 1);
      memcpy(&data[0], &readData[sizeof(_startMarkerSignature)], _dataSize);
      return data;
   }

   bool isValidData()
   {
      return _dataIsValid;
   }
private:
  unsigned int _EEPROM_write(int address, const String& value)
  {
      const byte* ptr = (const byte*)(const void*)&value;
      unsigned int i;
      for (i = 0; i < sizeof(value); i++)
      {
         EEPROM.write(address++, *ptr++);
      }
      return i;
  }

  unsigned int _EEPROM_read(int address, String& value)
  {
      byte* ptr = (byte*)(void*)&value;
      unsigned int i;
      for (i = 0; i < sizeof(value); i++)
      {
         *ptr++ = EEPROM.read(address++);
      }
      return i;
  }

  unsigned int _dataSize;
  String _password;
  const unsigned int _startMarkerSignature = START_MARKER_SIGNATURE;
  const unsigned int _endMarkerSignature = END_MARKER_SIGNATURE;
  bool _dataIsValid;
};

