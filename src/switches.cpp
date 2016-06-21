#include "switches.h"
#include <Arduino.h>
#include <algorithm>

GPIOSwitch::GPIOSwitch( uint8_t pinNumber,  unsigned long debounceTime)
  : Switch(debounceTime)
{
  _pinNumber = pinNumber;
  pinMode(_pinNumber, INPUT);
}

void GPIOSwitch::checkState()
{
  set_Pressed( digitalRead(_pinNumber) == 1 );
}

AnalogSwitch::AnalogSwitch( int expectedVolatage, int tolerance, unsigned long debounceTime)
  : Switch(debounceTime)
{
   _expectedVolatage = expectedVolatage;
   _tolerance = tolerance;
}

static int _lastADCRead = 0;
static int _lastADCReadTime = 0;

void AnalogSwitch::checkState()
{
  if (millis() - _lastADCRead > 10)
  {
    _lastADCRead = analogRead(A0);
    _lastADCReadTime = millis();
  }
  auto diff = _lastADCRead - _expectedVolatage;
  set_Pressed(abs(diff) <= _tolerance);
}

Switch::Switch( unsigned long debounceTime)
{
   _pressed = false;
   _lastMatch = 0;
   _firstMatch = 0;
   _debounceTime = debounceTime;
}

void Switch::AddReleasedHandler(FuncPtr handler)
{
  _releaseEventHandlers.push_back(handler);
}

void Switch::RemoveReleasedHandler(FuncPtr handler)
{
  _releaseEventHandlers.erase( std::remove( _releaseEventHandlers.begin(), _releaseEventHandlers.end(), handler ), _releaseEventHandlers.end() );
}

void Switch::set_Pressed(bool pressed)
{
  unsigned long currentMS = millis();

  if (pressed)
  {
    _lastMatch = currentMS;
    if (_firstMatch == 0)
    {
      _firstMatch = currentMS;
    }
  }
  else
  {
    if (_pressed == true)
    {
        for(auto releaseHandler: _releaseEventHandlers)
        {
          releaseHandler(currentMS - _firstMatch);
        }
        _firstMatch = 0;
        _lastMatch = 0;
    }

    _pressed = false;

  }

  if (currentMS - _firstMatch >= _debounceTime)
  {
    if (pressed)
    {
      _pressed = true;
    }
    else
    {
      // We didn't see pressed button for more then debounce time
      // or the state of button changed after debounce time
      // Rest all counters.
      _firstMatch = 0;
      _lastMatch = 0;
    }
  }
}
