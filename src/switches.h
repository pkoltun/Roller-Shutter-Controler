#pragma once
#include <Arduino.h>
#undef min
#undef max
#include <vector>

class Switch
{
  typedef void(*FuncPtr)(int);
private:
  bool _pressed;
  unsigned long _lastMatch;
  unsigned long _firstMatch;
  unsigned long _debounceTime;
  std::vector<FuncPtr> _releaseEventHandlers;
protected:
  void set_Pressed ( bool pressed );
public:

  bool get_Pressed();

  Switch( unsigned long debounceTime);
  void AddReleasedHandler(FuncPtr handler);
  void RemoveReleasedHandler(FuncPtr handler);
  void virtual checkState() = 0;
};

class AnalogSwitch : public Switch
{
private:
  int _expectedVolatage;
  int _tolerance;
public:

  AnalogSwitch( int expectedVolatage, int tolerance, unsigned long debounceTime);
  void checkState();
};

class GPIOSwitch : public Switch
{
private:
  uint8_t _pinNumber;
public:

  GPIOSwitch(uint8_t pinNumber, unsigned long debounceTime);
  void checkState();
};
