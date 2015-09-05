#ifndef i2ckeypadreader_h
#define i2ckeypadreader_h


class i2ckeypad {
public:
  i2ckeypad(int);
  i2ckeypad(int, int, int);
  char getkey();
  void init();
  
private:
  void pcf8574_write(int, int);
  int pcf8574_byte_read(int);
};

#endif
